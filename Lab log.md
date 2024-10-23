## Telematic Applications

# Lab 3: Concurrent servers with polling and select

*Academic year 2024-2025*

---

## 1. Implement non-blocking IO operations

The client's code was edited to include the requested calls and
functionalities. It was tested, and the counter kept increasing quickly without
ever exiting the loop or seemingly reading any of the server's echo. The output
can be checked in the logs.

## 2. Test demand-accept.c server

As expected, the server starts, sets some options, opens a listening socket on
port 9999, and sets up the handler. When any client tries to connect to that
socket, the server's `sigio` handler accepts the connection, sends a message to
the client, and promptly closes it. This is all done in the handler. This
operation can be repeated as many times as desired. 

Upon requesting a connection, the client receives the `SINACK` segment. When
the server receives the client's `ACK`, it sends the message followed by a
`FIN` segment.

## 3. Test smart-select.c server

As we can see, all processes are able to accept connections, up to 2 at a time
each. When there are 5 servlets running, up to 10 clients can connect at a
time. We can see the limitation more clearly if we lower the amount of servlets
to 2. At this level, on ly 4 connections could be served at a time, as the logs
show.

When trying to connect 5 clients at once, the last one got connected (triple
handshake) performed, but the server was not able to serve it, so it didn't get
echo. Strange behavior was observed when clients were killed with Ctrl+C,
however: the server displayed some unreadable characters. The server's output
was not fully logged due to a bug in the server code. Even when clients were
killed, the connection stayed up, with neither side sending `FIN` segments, so
the slots occupied by the clients were not freed up and the fifth client was never able to connect.

All connections were closed by the server when killed.

