#ifndef __BM_ADIN2111_H__
#define __BM_ADIN2111_H__

#include "adin2111.h"
#include "network_device.h"
#include "util.h"

#define ADIN2111_PORT_MASK (3U)

#ifdef __cplusplus
extern "C" {
#endif

BmErr adin2111_init(void);
NetworkDevice adin2111_network_device(void);

#ifdef __cplusplus
}
#endif

#endif // __BM_ADIN2111_H__
