# Services
Bristlemouth offers a service API that can be used to build request/response services.

The two main components of the service API are the service manager and
the service request handler.

The service manager (`service.h`) is responsible for registering services and
forwarding incoming service requests to the appropriate service request callback.

The service request handler (`service_request.h`) is responsible for tracking outbound service requests
and invoking the appropriate service response callback when a response is received.
Outbound requests each should have a unique ID, callback, and timeout.
If the request times out, the service request handler will invoke the callback with a "fail".
If the request is successful, the service request handler will invoke the callback with a "success".

Both the service manager and service request handler are built on-top of the
Bristlemouth Pub/Sub API and subsequently on-top of the middleware.
Therefore the middleware is responsible for processing service callbacks.

## Services
Services are registered with the service manager by providing a service ID and a service request callback.
The service ID is a string that uniquely identifies the service.
The service request callback is invoked when a service request is received.

<!-- TODO - service handler is the name of the service callback so that should be changed here
in the docs to reflect that and make it less confusing -->


## Service Requests
