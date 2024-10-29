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

## Supported Services

These services offer a request/reply form of communication.
This allows for applications to publish to certain topics when information is requested or replied
and obtain information from a specific node ID.
The following services are supported on Bristlemouth:

- config_cbor_map
- sys_info

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
