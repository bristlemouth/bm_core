#include "bristlemouth.h"
#include "bcmp.h"
#include "bm_ip.h"
#include "bm_service.h"
#include "l2.h"
#include "middleware.h"

#define BM_MIDDLEWARE_PORT 4321

BmErr bristlemouth_init(NetworkDevicePowerCallback net_power_cb) {
  NetworkDevice network_device = adin2111_network_device();
  network_device.callbacks->power = net_power_cb;

  BmErr err = BmOK;
  bm_err_check(err, adin2111_init());
  bm_err_check(err, bm_l2_init(network_device));
  bm_err_check(err, bm_ip_init());
  bm_err_check(err, bcmp_init(network_device));

  if (err == BmOK) {
    bm_service_init();
  }

  bm_err_check(err, bm_middleware_init(BM_MIDDLEWARE_PORT));
  return err;
}
