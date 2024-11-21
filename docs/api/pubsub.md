# PubSub

Bristlemouth offers the ability to subscribe and publish to topics on the network.
A topic is a unique string that nodes on the network communicate over.
Pubsub is apart of the [Middleware Protocol](https://bristlemouth.notion.site/Middleware-Protocol-0f2bdf9abaca49a488fbe52e6a92cf96)
on the Bristlemouth stack.
Middleware is built on top of UDP
and the publish subscribe capabilities are associated with port 4321.

Topics are subscribed to,
and then added to the resource list in `bcmp/resource_discovery.c`,
which can be used by other nodes to determine the subscribed to topics of interest.
The following is an example of how to subscribe to a topic of interest:

```
#define sub_topic "sub/example"

static void example_sub_cb(uint64_t node_id, const char *topic,
                           uint16_t topic_len, const uint8_t *data,
                           uint16_t data_len, uint8_t type, uint8_t version)
{
    ...
}

int main(void) {

  ...

  bm_sub(sub_topic, example_sub_cb);

  ...

}
```

Multiple callbacks are able to be tied to the same subscription,
allowing multiple applications to utilize the same topic.
API is also available to unsubscribe to topics.

In order to use the API required by the pubsub module,
the following header must be included:

```
#include "pubsub.h"
```

---

## API

```{eval-rst}
.. cpp:function:: bool bm_sub(const char *topic, const BmPubSubCb callback);

  Subscribe to a specific string topic with callback

  :param topic: topic string to subscribe to
  :param callback: callback function to call when data is received on this topic

  :returns: BmOk if able to properly subscribe to topic, BmErr otherwise
```

```{eval-rst}
.. cpp:function:: bool bm_sub_wl(const char *topic, uint16_t topic_len, \
                                 const BmPubSubCb callback);

  Subscribe to a specific string topic with callback (while providing the topic
  length)

  :param topic: topic string to subscribe to
  :param topic_len: length of topic string
  :param callback: callback function to call when data is received on this topic

  :returns: BmOk if able to properly subscribe to topic, BmErr otherwise
```

```{eval-rst}
.. cpp:function:: bool bm_unsub(const char *topic, const BmPubSubCb callback);

  Unsubscribe to a specific string topic with callback

  :param topic: topic string to unsubscribe from
  :param callback: callback function to call when data is received on this topic

  :returns: BmOk if able to properly unsubscribe from topic, BmErr otherwise
```

```{eval-rst}
.. cpp:function:: bool bm_unsubwl(const char *topic, uint16_t topic_len, \
                                  const BmPubSubCb callback);

  Unsubscribe to a specific string topic with callback (while providing the topic
  length)

  :param topic: topic string to unsubscribe from
  :param topic_len: length of topic string
  :param callback: callback function to call when data is received on this topic

  :returns: BmOk if able to properly unsubscribe from topic, BmErr otherwise
```

```{eval-rst}
.. cpp:function:: bool bm_pub(const char *topic, const void *data, \
                              uint16_t len, const uint8_t type, \
                              uint8_t version);

  Publish data to specific topic

  :param topic: topic string to publish to
  :param data: data to publish
  :param len: length of data to publish
  :param type: type of data to publish, this is defined by the integrator (not currently used)
  :param version: version of the data to publish, this is defined by integrator (not currently used)

  :returns: BmOk if able to properly publish to topic, BmErr otherwise
```

```{eval-rst}
.. cpp:function:: bool bm_pubwl(const char *topic, uint16_t topic_len, \
                                const void *data, uint16_t len, \
                                const uint8_t type, uint8_t version);

  Publish data to specific topic (while providing the topic length)

  :param topic: topic string to publish to
  :param topic_len: length of topic string
  :param data: data to publish
  :param len: length of data to publish
  :param type: type of data to publish, this is defined by the integrator (not currently used)
  :param version: version of the data to publish, this is defined by integrator (not currently used)

  :returns: BmOk if able to properly publish to topic, BmErr otherwise
```
