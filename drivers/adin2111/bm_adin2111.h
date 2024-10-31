#include "adin2111.h"
#include "network_interface.h"
#include "util.h"

#define ADIN2111_PORT_MASK (3U)

typedef struct {
  void *device_handle;
  NetworkInterfaceCallbacks const *callbacks;
} Adin2111;

#ifdef __cplusplus
extern "C" {
#endif

BmErr adin2111_init(Adin2111 *self);
NetworkInterface prep_adin2111_netif(Adin2111 *self);

#ifdef __cplusplus
}
#endif
