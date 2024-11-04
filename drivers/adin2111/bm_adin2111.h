#ifndef __BM_ADIN2111_H__
#define __BM_ADIN2111_H__

#include "adin2111.h"
#include "network_device.h"
#include "util.h"

#define ADIN2111_PORT_MASK (3U)

typedef struct {
  void *device_handle;
  NetworkDeviceCallbacks const *callbacks;
} Adin2111;

#ifdef __cplusplus
extern "C" {
#endif

BmErr adin2111_init(Adin2111 *self);
NetworkDevice create_adin2111_network_device(Adin2111 *self);

#ifdef __cplusplus
}
#endif

#endif // __BM_ADIN2111_H__
