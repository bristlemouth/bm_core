#include "bristlemouth.h"
#include "bcmp.h"
#include "bm_adin2111.h"
#include "bm_config.h"
#include "bm_ip.h"
#include "bm_service.h"
#include "l2.h"
#include "middleware.h"
#include "topology.h"

BmErr bristlemouth_init(NetworkDevicePowerCallback net_power_cb) {
  NetworkDevice network_device = adin2111_network_device();
  network_device.callbacks->power = net_power_cb;

  BmErr err = BmOK;
  bm_err_check(err, adin2111_init());
  bm_err_check(err, bm_l2_init(network_device));
  bm_err_check(err, bm_ip_init());
  bm_err_check(err, bcmp_init(network_device));
  bm_err_check(err, topology_init(network_device.trait->num_ports()));
  bm_err_check(err, bm_service_init());
  bm_err_check(err, bm_pubsub_init());
  bm_err_check(err, bm_middleware_init());
  return err;
}

NetworkDevice bristlemouth_network_device(void) {
  return adin2111_network_device();
}
