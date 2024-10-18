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
