#include "middleware.h"
#include "bcmp.h"
#include "bm_config.h"
#include "bm_ip.h"
#include "bm_os.h"
#include "l2.h"
#include "ll.h"
#include <string.h>

#define middleware_task_size 512
#define net_queue_len 64
#define udp_header_size 8
#define max_payload_len_udp (max_payload_len - udp_header_size)

typedef struct {
  void *pcb;
  uint16_t port;
  BmMiddlewareRxCb rx_cb;
  BmMiddlewareRoutingCb routing_cb;
  BmIpAddr dest;
} MiddlewareApplication;

typedef struct {
  LL applications;
  BmQueue net_queue;
} MiddlewareCtx;

typedef struct {
  void *buf;
  uint64_t node_id;
  uint32_t size;
  uint16_t port;
} NetQueueItem;

static MiddlewareCtx CTX = {0};

static bool handle_middleware_routing(uint8_t ingress_port,
                                      uint16_t *egress_ports, BmIpAddr *src,
                                      const BmIpAddr *dest) {
  bool should_handle = true;

  CTX.applications.cursor = CTX.applications.head;
  while (CTX.applications.cursor) {
    const MiddlewareApplication *application =
        (MiddlewareApplication *)CTX.applications.cursor->data;

    if (memcmp(&application->dest, dest, sizeof(BmIpAddr)) != 0) {
      CTX.applications.cursor = CTX.applications.cursor->next;
      continue;
    }

    if (application->routing_cb) {
      should_handle = application->routing_cb(ingress_port, egress_ports, src);
    }
    break;
  }

  return should_handle;
}

/*!
  @brief Middleware Receiving Callback Bound To UDP Interface

  @param port the UDP port the message was received on
  @param buf buffer to interperet
  @param node_id node id to queue buffer for
  @param size size of buf in bytes

  @return BmOK on success
  @return BmErr on failure
*/
BmErr bm_middleware_rx(uint16_t port, void *buf, uint64_t node_id,
                       uint32_t size) {
  BmErr err = BmEINVAL;
  NetQueueItem queue_item;

  if (buf) {
    queue_item.buf = buf;
    queue_item.node_id = node_id;
    queue_item.size = size;
    queue_item.port = port;

    if ((err = bm_queue_send(CTX.net_queue, &queue_item, 0)) != BmOK) {
      bm_udp_cleanup(buf);
    }
  } else {
    bm_debug("Error. Message has no payload buf\n");
  }

  return err;
}

/*!
  @brief Middleware network processing task

  @details Will receive middleware packets in queue from
           middleware receive callback and process them

  @param *arg unused

  @return None
*/
static void middleware_net_task(void *arg) {
  (void)arg;
  NetQueueItem item = {0};
  BmErr err = BmOK;

  for (;;) {
    if (item.buf) {
      bm_udp_cleanup(item.buf);
      item.buf = NULL;
    }

    err = bm_queue_receive(CTX.net_queue, &item, UINT32_MAX);
    if (err != BmOK || !item.buf) {
      continue;
    }

    MiddlewareApplication *application = NULL;
    err = ll_get_item(&CTX.applications, item.port, (void **)&application);
    if (err != BmOK) {
      continue;
    }

    application->rx_cb(item.node_id, item.buf, item.size);
  }
}

/*!
  @brief Initialize Middleware 

  @param port UDP port to use for middleware
*/
BmErr bm_middleware_init(void) {
  BmErr err = BmEINVAL;

  err = bm_l2_register_link_local_routing_callback(handle_middleware_routing);
  if (err != BmOK) {
    return err;
  }

  err = BmENOMEM;
  CTX.net_queue = bm_queue_create(net_queue_len, sizeof(NetQueueItem));

  if (CTX.net_queue) {
    err = bm_task_create(middleware_net_task, "Middleware Task",
                         // TODO - verify stack size
                         middleware_task_size, NULL,
                         middleware_net_task_priority, NULL);
  }

  return err;
}

/*!
 @brief Add a middleware application

 @details This function adds an application to a linked list, it
          will bind the port to a UDP socket, set up the dest address 
          to send messages to when required. Received messages invoke
          the rx_cb. The port bound to the application is used as the
          linked list's ID.

 @param port UDP pot to bind to the application
 @param dest Destination address to send messages and receive messages on
 @param rx_cb Reception callback to handle received messages

 @return BmOK on success
         BmErr on failure
 */
BmErr bm_middleware_add_application(uint16_t port, BmIpAddr dest,
                                    BmMiddlewareRxCb rx_cb,
                                    BmMiddlewareRoutingCb routing_cb) {
  LLItem *item = NULL;

  void *pcb = bm_udp_bind_port(&dest, port, bm_middleware_rx);
  if (!pcb) {
    return BmENOMEM;
  }

  MiddlewareApplication application = {
      .port = port,
      .dest = dest,
      .rx_cb = rx_cb,
      .routing_cb = routing_cb,
      .pcb = pcb,
  };
  item = ll_create_item(item, &application, sizeof(application), port);
  if (!item) {
    return BmENOMEM;
  }

  return ll_item_add(&CTX.applications, item);
}

/*!
  @brief Middleware network transmit function

  @param[in] *buf data to send over UDP
  
  @return BmOK on success
  @return BmErr on failure
*/
BmErr bm_middleware_net_tx(uint16_t port, void *buf, uint32_t size) {
  BmErr err;
  MiddlewareApplication *application = NULL;

  err = ll_get_item(&CTX.applications, port, (void **)&application);
  if (err != BmOK) {
    return err;
  }

  // Don't try to transmit if the payload is too big
  err = BmEINVAL;
  if (buf && size <= max_payload_len_udp) {
    err =
        bm_udp_tx_perform(application->pcb, buf, size,
                          (const void *)&application->dest, application->port);
  }

  return err;
}
