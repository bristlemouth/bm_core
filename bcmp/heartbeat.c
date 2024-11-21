#include "messages/heartbeat.h"
#include "bcmp.h"
#include "bm_config.h"
#include "bm_os.h"
#include "messages/info.h"
#include "messages/neighbors.h"
#include "packet.h"
#include <inttypes.h>

/*!
  @brief Send heartbeat to neighbors

  @param lease_duration_s - liveliness lease duration

  @return BmOk on success
  @return BmErr on failure
*/
BmErr bcmp_send_heartbeat(uint32_t lease_duration_s) {
  BcmpHeartbeat heartbeat = {.time_since_boot_us =
                                 bm_ticks_to_ms(bm_get_tick_count()) * 1000,
                             .liveliness_lease_dur_s = lease_duration_s};
  return bcmp_tx(&multicast_ll_addr, BcmpHeartbeatMessage,
                 (uint8_t *)&heartbeat, sizeof(heartbeat), 0, NULL);
}

/*!
  @brief Process Incoming Heartbeat

  @details Update neighbor tables if necessary, if a device is not in our
           neighbor tables, add it, and request it's info

  @param *data - heartbeat data

  @return BmOK on success
  @return BmErr on failure
*/
static BmErr bcmp_process_heartbeat(BcmpProcessData data) {
  BmErr err = BmEINVAL;
  BcmpHeartbeat *heartbeat = (BcmpHeartbeat *)data.payload;

  BcmpNeighbor *neighbor =
      bcmp_update_neighbor(ip_to_nodeid(data.src), data.ingress_port);
  if (neighbor) {
    err = BmOK;

    // Neighbor restarted, let's get additional info
    if (heartbeat->time_since_boot_us < neighbor->last_time_since_boot_us) {
      bcmp_neighbor_invoke_discovery_cb(true, neighbor);
      bm_debug("🏘📡 Updating neighbor info! %016" PRIx64 "\n",
               neighbor->node_id);
      bcmp_request_info(neighbor->info.node_id, &multicast_ll_addr, NULL);
    }

    // Update times
    neighbor->last_time_since_boot_us = heartbeat->time_since_boot_us;
    neighbor->heartbeat_period_s = heartbeat->liveliness_lease_dur_s;
    neighbor->last_heartbeat_ticks = bm_get_tick_count();
    neighbor->online = true;
  }

  return err;
}

/*!
  @brief Initialize Heartbeat Module

  @return BmOK on success
  @return BmErr on failure
*/
BmErr bcmp_heartbeat_init(void) {
  BcmpPacketCfg heartbeat_packet = {
      false,
      false,
      bcmp_process_heartbeat,
  };

  return packet_add(&heartbeat_packet, BcmpHeartbeatMessage);
}
