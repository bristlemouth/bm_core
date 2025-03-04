# Middleware

Bristlemouth offers a flexible,
easy-to-use layer on the network stack to provide an easy way to build applications.
The middleware provides a simple publish/subscribe method
that allows nodes to communicate over certain topics
(comparable to other middleware schemes such as MQTT or DDS).
On top of the pub/sub messaging,
services are able to be built utilizing request/reply communication.
This allows for bi-directional communication between nodes.
Please view the following [page](https://bristlemouth.notion.site/Middleware-Protocol-0f2bdf9abaca49a488fbe52e6a92cf96)
for further detailed information on the middleware provided by Bristlemouth,

Within this section of the codebase,
the following diagram shows how these components work together:

```{image} middleware.png
:align: center
:width: 1000
:class: no-scaled-link
```

(supported_services)=

## Supported Services

These services offer a request/reply form of communication.
This allows for applications to publish to certain topics when information is requested or replied to.
Please view the [Bristlemouth Specification](https://bristlemouth.notion.site/The-Bristlemouth-Standard-Specification-f5449080f5c940cabbd0512b4d2aeb82)
for further information on services.
The following services are supported on Bristlemouth:

- config_cbor_map
- sys_info
- power_info

### Config CBOR Map Service
The replier to this service will generate a key-pair table of all configuration values on the system
and report it to the requestor.
When requesting a config CBOR map,
the request is sent to a specific node
as well as the configuration partition that the map is to be reported as (system, user or hardware).
This message is cbor enceded before sending over the wire,
for both the requestor and the replier.
An example of how these key pairs look when decoded is as follows:
```
"sampleIntervalMs": 30000,
"sampleDurationMs": 310000,
"subsampleIntervalMs": 60000,
"subsampleDurationMs": 30000,
"subsampleEnabled": 0,
"bridgePowerControllerEnabled": 0,
"ticksSamplingEnabled": 0,
"samplesPerReport": 2,
"transmitAggregations": 1,
"currentReadingPeriodMs": 60000,
"softReadingPeriodMs": 500,
"rbrCodaReadingPeriodMs": 500,
"turbidityReadingPeriodMs": 1000
```

### System Information Service
When requested,
the system information provided from a node is as follows:

- The application name of the node
- The GIT SHA of the node
- The node's ID
- The crc of the device's config CBOR map (see above)

### Power Information Timing Service
A service to provide a node information total time the bus power will be on,
the time remaining for the bus power to be on,
and the upcoming time the bus power will be off.
This service is available on the following topic: `bus_power_controller/timing`.
Power information timing is reported in the following format when requested:

```
    {
      "total_on_s": 310,
      "remaining_on_s": 121,
      ”upcoming_off_s”: 1500
    }
```

This information is useful to ensure that critical operations on a node will be finalized before the power turns off.
If one of the values are undefined,
it is standard to indicate this by utilizing the macro `POWER_SERVICE_UNDEFINED`.
When an undefined value is given,
it is the responsibility of the requestor to handle this accordingly.
For example,
if `upcoming_off_s` is equivalent to `POWER_SERVICE_UNDEFINED`,
then the requestor should prepare for the power to potentially never return.
Another example of this is if the power is on indefinitely for the time being,
`POWER_SERVICE_UNDEFINED` shall be used to indicate that there is a potential for the bus power timing to change.
In this use case,
the requestor should continue to request the service to ensure this value does not change.
