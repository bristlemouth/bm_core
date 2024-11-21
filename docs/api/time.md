# Time
Bristlemouth offers a time API that can be used to obtain or set the current time on a device.

Time messages are sent over the Bristlemouth Control Messaging Protocol (BCMP).

The following header file path must be included when calling BCMP time API:

```
#include "messages/time.h"
```

Public API supported by the BCMP time module is as follows:

```{eval-rst}
.. cpp:function:: BmErr bcmp_time_get_time(uint64_t target_node_id)

  Get the current time from a target node.

  :param target_node_id: Target node id to obtain the current time from

  :returns: BmOK if the get message was sent successfully, BmErr otherwise
```

```{eval-rst}
.. cpp:function:: BmErr bcmp_time_set_time(uint64_t target_node_id, uint64_t utc_us)

  Set the current time on a target node.

  :param target_node_id: Target node id to set the current time on
  :param utc_us: Time to set on the target node represented in microseconds since epoch

  :returns: BmOK if the set message was sent successfully, BmErr otherwise
```
