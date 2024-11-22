# Services
Bristlemouth offers a service API that can be used to build request/response services.

The two main components of the service API:
1. services
2. service requests

Both the services and service requests are built on-top of the
Bristlemouth Pub/Sub API and subsequently on-top of the middleware.
Therefore services and service requests are processed in the middleware thread.

## Services
The services component (`service.h/.c`) is responsible for registering services and
forwarding incoming service requests to the appropriate service handler callback.
Services most commonly will be implemented to provide a response/reply to a request.

### Registering Services

To register a service, you must provide a service ID and a service handler callback.
The service ID is a string that uniquely identifies the service.
The service handler callback is user defined and is invoked when a service request is received.
Registering a service will add the service to the list of available services and the service will
appear in the subscription list as `node_id/serivce ID/req`.
This is the topic that the service will listen on for incoming requests.

When a request is made to the service, the service handler callback will be invoked with the
request data and the service handler callback should respond with the appropriate response data.
The response data will be sent back to the requester on the topic `node_id/service ID/rep`.
This topic will then appear in the publication list as `node_id/service ID/rep`.
This is the topic that the requester will listen on for the response.

```{eval-rst}
.. cpp:function:: bool bm_service_register(size_t service_strlen, const char *service, \
                                           BmServiceHandler service_handler)

  Register a service.

  :param service_strlen: The length of the service string
  :param service: The service string
  :param service_handler: The callback to call when a request is received

  :returns: true if the service was registered, false otherwise
```

### Unregistering Services

To unregister a service, you must provide the service ID of the service you wish to unregister.
Unregistering a service will remove the service from the list of available services and service
requests will no longer be forwarded to the service handler callback.

```{eval-rst}
.. cpp:function:: bool bm_service_unregister(size_t service_strlen, const char *service)

  Unregister a service.

  :param service_strlen: The length of the service string
  :param service: The service string

  :returns: true if the service was unregistered, false otherwise
```

## Service Requests

The service requests (`service_request.h\.c`) is responsible for tracking outbound service requests
and invoking the appropriate service response callback when a response is received.
Outbound requests each should have a unique ID, callback, and timeout.
If the request times out, the service request handler will invoke the callback with a "fail".
If the request is successful, the service request handler will invoke the callback with a "success".

When a service request is made, the request data will be sent to the service on the topic
`node_id/service ID/req`, the topic that the recipient should be subscribed to.
This topic will then appear in the requesters's publication list.
Likewise, the requester will subscribe to the topic `node_id/service ID/rep` to receive the response.
This topic will appear in the requester's subscription list.

### Sending Service Requests

To send a service request, you must provide the service ID of the service you wish to request,
the data that may be sent as part of the request, a callback to be invoked when the reply is received,
and a timeout to expire the request if no reply is received.

```{eval-rst}
.. cpp:function:: bool bm_service_request(size_t service_strlen, const char *service, \
                                          size_t data_len, const uint8_t *data, \
                                          BmServiceReplyCb reply_cb, uint32_t timeout_s)

  Send a service request.

  :param service_strlen: The length of the service string
  :param service: The service string
  :param data_len: The length of the data
  :param data: The data to send as part of the request
  :param reply_cb: The callback to call when a reply is received
  :param timeout_s: The timeout in seconds

  :returns: true if the request was sent, false otherwise
```

## Examples
Check out the [middleware reference](../reference/middleware.md#supported-services) for more example services.
