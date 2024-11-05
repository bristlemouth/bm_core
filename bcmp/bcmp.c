#include "bcmp.h"
#include "bm_config.h"
#include "bm_ip.h"
#include "bm_os.h"
#include "dfu.h"
#include "messages/config.h"
#include "messages/heartbeat.h"
#include "messages/info.h"
#include "messages/neighbors.h"
#include "messages/ping.h"
#include "messages/resource_discovery.h"
#include "messages/time.h"
#include "messages/topology.h"
#include "packet.h"
#include "util.h"
#include <string.h>

#define bcmp_evt_queue_len 32

// Send heartbeats every 10 seconds (and check for expired links)
#define bcmp_heartbeat_s 10

typedef struct {
  BmQueue queue;
  BmTimer heartbeat_timer;
} BcmpContext;

static BcmpContext CTX;

/*!
  @brief Timer handler for sending out heartbeats

  @details No work is done in the timer handler, but instead an event is
           queued up to be handled in the BCMP task.

  @param tmr unused
*/
static void heartbeat_timer_handler(BmTimer tmr) {
  (void)tmr;

  BcmpQueueItem item = {BcmpEventHeartbeat, NULL, 0};

  bm_queue_send(CTX.queue, &item, 0);
}

/*!
  @brief BCMP task. All BCMP events are handled here.

  @param parameters unused
*/
static void bcmp_thread(void *parameters) {
  (void)parameters;

  CTX.heartbeat_timer =
      bm_timer_create("bcmp_heartbeat", bcmp_heartbeat_s * 1000, true, NULL,
                      heartbeat_timer_handler);

  // TODO - send out heartbeats on link change
  for (;;) {
    BcmpQueueItem item;

    BmErr err = bm_queue_receive(CTX.queue, &item, UINT32_MAX);

    if (err == BmOK) {
      switch (item.type) {
      case BcmpEventRx: {
        process_received_message(item.data, item.size - sizeof(BcmpHeader));
        break;
      }

      case BcmpEventHeartbeat: {
        // Should we check neighbors on a differnt timer?
        // Check neighbor status to see if any dropped
        bcmp_check_neighbors();

        // Send out heartbeats
        bcmp_send_heartbeat(bcmp_heartbeat_s);
        break;
      }

      default: {
        break;
      }
      }

      bm_ip_rx_cleanup(item.data);
    }
  }
}

/*!
  @brief BCMP link change event callback

  @param port - system port in which the link change occurred
  @param state - 0 for down 1 for up
*/
void bcmp_link_change(uint8_t port, bool state) {
  (void)port; // Not using the port for now
  if (state) {
    // Send heartbeat since we just connected to someone and (re)start the
    // heartbeat timer
    bcmp_send_heartbeat(bcmp_heartbeat_s);
    bm_timer_start(CTX.heartbeat_timer, 10);
  }
}

/*!
  @brief BCMP initialization

  @param *netif lwip network interface to use
  @return none
*/
BmErr bcmp_init(NetworkDevice network_device) {
  CTX.queue = bm_queue_create(bcmp_evt_queue_len, sizeof(BcmpQueueItem));
  network_device.callbacks->link_change = bcmp_link_change;

  bcmp_heartbeat_init();
  ping_init();
  time_init();
  bm_dfu_init();
  bcmp_config_init();
  bcmp_topology_init();
  bcmp_device_info_init();
  bcmp_resource_discovery_init();

  return bm_task_create(bcmp_thread, "BCMP", 1024, NULL, bcmp_task_priority,
                        NULL);
}

void *bcmp_get_queue(void) { return CTX.queue; }

/*!
  @brief BCMP packet transmit function. Header and checksum added and computer within

  @param *dst destination ip
  @param type message type
  @param *data message buffer
  @param size message length in bytes
  @param seq_num The sequence number of the message
  @param reply_cb A callback function to handle reply messages

  @return BmOK on success
  @return BmErr on failure
*/
BmErr bcmp_tx(const void *dst, BcmpMessageType type, uint8_t *data,
              uint16_t size, uint32_t seq_num,
              BmErr (*reply_cb)(uint8_t *payload)) {
  BmErr err = BmEINVAL;
  void *buf = NULL;

  if (dst && (uint32_t)size + sizeof(BcmpHeartbeat) <= max_payload_len) {
    buf = bm_ip_tx_new(dst, size + sizeof(BcmpHeader));
    if (buf) {
      err = serialize(buf, data, size, type, seq_num, reply_cb);

      if (err == BmOK) {
        err = bm_ip_tx_perform(buf, NULL);
        if (err != BmOK) {
          bm_debug("Error sending BMCP packet %d\n", err);
        }
      } else {
        bm_debug("Could not properly serialize message\n");
      }

      bm_ip_tx_cleanup(buf);
    } else {
      err = BmENOMEM;
      bm_debug("Could not allocate memory for bcmp message\n");
    }
  }

  return err;
}

/*!
  @brief Forward the payload to all ports other than the ingress port.

  @details See section 5.4.4.2 of the Bristlemouth spec for details.

  @param header header message buffer to forward
  @param payload payload to be forwarded
  @param size payload size
  @param ingress_port Port on which the packet was received.

  @return BmOK on success
  @return BmErr on failure
*/
BmErr bcmp_ll_forward(BcmpHeader *header, void *payload, uint32_t size,
                      uint8_t ingress_port) {
  uint8_t port_specific_dst[sizeof(multicast_ll_addr)];
  BmErr err = BmEINVAL;
  void *forward = NULL;
  // TODO: Make more generic. This is specifically for a 2-port device.
  uint8_t egress_port = ingress_port == 1 ? 2 : 1;

  if (header && payload) {
    memcpy(port_specific_dst, &multicast_ll_addr, sizeof(multicast_ll_addr));
    ((uint32_t *)port_specific_dst)[3] = 0x1000000 | (egress_port << 8);

    // L2 will clear the egress port from the destination address,
    // so calculate the checksum on the link-local multicast address.
    forward = bm_ip_tx_new(&multicast_ll_addr, size + sizeof(BcmpHeader));
    if (forward) {
      header->checksum = 0;
      header->checksum = packet_checksum(forward, size + sizeof(BcmpHeader));

      // Copy data to be forwarded
      bm_ip_tx_copy(forward, header, sizeof(BcmpHeader), 0);
      bm_ip_tx_copy(forward, payload, size, sizeof(BcmpHeader));

      err = bm_ip_tx_perform(forward, &port_specific_dst);
      if (err != BmOK) {
        bm_debug("Error forwarding BMCP packet link-locally: %d\n", err);
      }
      bm_ip_tx_cleanup(forward);
    } else {
      err = BmENOMEM;
    }
  }
  return err;
}
