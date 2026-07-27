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
- metrics

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

### Metrics Service

The metrics service reports diagnostic counters and gauges from a node.
Rather than a fixed message,
the reply is a generic envelope that carries any number of independent components,
each contributing its own set of flat key/value fields.
This allows new metric producers (network PHYs, sensors, subsystems)
to be added without changing the message or the requestor.
The service is available on the topic `<node_id>/metrics`.

The reply contains three metadata fields (`version`, `node_id`, `uptime_ms`)
and a `data` map keyed by component name,
where each component's value is a map of that component's fields.
For example, the ADIN2111 driver provides an `adin_port_stats` component,
which a two-port node decodes as:

    {
      "version": 1,
      "node_id": "a4bf32db19ba188c",
      "uptime_ms": 92238,
      "data": {
        "adin_port_stats": {
          "num_ports": 2,
          "sqi_1": 7, "mse_1": 32, "lq_1": 2, "rxe_1": 0,
          "sye_1": 0, "fc_1": 0, "len_1": 0, "algn_1": 0,
          "sqi_2": 7, "mse_2": 0, "lq_2": 2, "rxe_2": 0,
          "sye_2": 0, "fc_2": 0, "len_2": 0, "algn_2": 0
        }
      }
    }

Here the component reports its fields per port,
flattened with a `_<port>` suffix (ports are 1-indexed). The set of fields and their semantics are defined by that component, not the service.
