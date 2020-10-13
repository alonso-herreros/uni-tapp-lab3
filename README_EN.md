## Concurrent Servers with the sockets API

### Support Materials 
* On-line man pages: socket(2), socket(7), send(2), recv(2), read(2), write(2), setsockopt(2), fcntl(2), select(2), tcp(7), ip(7).
* Guide to using sockets by Brian "Beej" Hall
* Tcpdump manual
* Chapters 6, 7 y 8 of "Linux Socket Programming" by Sean Walton, Sams Publishing Co. 2001
* Cheat Sheet file within this project

### Problem statement

The sockets assignment is divided into the following three parts:
1. Sequential servers (echo client and server, socket options, analysis with tcpdump, file servers) 
2. Concurrent servers (processes, threads).
3. **Input/output (signal handlers, poll, select)**

## Blocking and non-blocking input/output

This assignement relies on the [previous assignment sockets2]((https://gitlab.pervasive.it.uc3m.es/aptel/sockets2_concurrent_servers)), but there are some new code that will be used later. The new code can be downloaded this way:
 ```
 git -c http.sslVerify=false clone https://gitlab.pervasive.it.uc3m.es/aptel/sockets3_concurrent_servers_polling_select.git
 ```


As is usual for input/output operations, input/output operations on sockets are generally blocking. That is, when a process performs an input/output operation on a socket, it goes into a sleeping state waiting for a condition to be satisfied before the operation can be completed. Thus:

* If a process performs an input operation (read, recv, recvfrom,...) on a TCP socket and there is no data in the input buffer of that socket, the process goes to sleep until some data becomes available.
* If a process performs an output operation (write, send, sendto,...) on a TCP socket - whereupon the kernel attempts to copy the output data from the application buffer to the output buffer - and there is no space in the output buffer of that socket, the process goes to sleep until sufficient space becomes available.

It is often useful to employ some mechanism that allows us to perform these operations in a non-blocking manner, enabling other tasks to be carried out instead of waiting for data, or space, to become available. In this lab session, we will look at some of the mechanisms that can be used to perform non-blocking input/output operations on sockets, in particular, polling and asynchronous mechanisms.

## The fcntl() function and the polling mechanism

The `fcntl()` function is a control function that enables us to perform different operations on file descriptors, socket descriptors etc. The prototype of this function is as follows:

```c
  #include <fcntl.h>
  int fcntl(int fd, int cmd, /* int arg*/);
```

Associated to each descriptor is a set of flags that provide information about it. The value of these flags can be obtained via a call to `fcntl()` with the `cmd` parameter set to `F_GETFL`. Similarly, the value of the flags can be modified via a call to `fcntl()` with the `cmd` parameter set to `F_SETFL`.

We recommend you study the details of the use of this function given on the fcntl(2) man page.

The `O_NONBLOCK` flag is used to indicate that the input/output operations on a given socket descriptor are non-blocking. This flag is set as follows:

```c
  int flags;
  // sd is the socket descriptor
  if ( fcntl(sd, F_SETFL, O_NONBLOCK) < 0 ) 
    perror("fcntl: could not set non-blocking operations");
```

We now know how to ensure that input/output operations are non-blocking but how can we know when data, or space, is available?

> When an input/output operation on a non-blocking socket cannot be completed, the call to the input/output function returns an error code (-1) and assigns the value EWOULDBLOCK to the variable errno (recall that it is always necessary to check the number of bytes returned by these calls, since it does not always coincide with the number of bytes that we wanted to read or write). 
> In order to find out when there is data available to be read, a polling mechanism, by which the socket is regularly checked to see if data has arrived, is often used. Each time the socket is polled, if no data is available other tasks can be carried out.

### 1.	Using the code provided for the [third sockets lab exercise](https://gitlab.pervasive.it.uc3m.es/aptel/sockets2_concurrent_servers) modify the client so that it can perform non-blocking input/output operations and so that it uses a polling mechanism to check the availability of data when performing read operations.

In order to manifest this behaviour, make the client print the value of a counter to the screen, where this counter is incremented each time the program has to wait for data to be returned by the server (that is, each time it performs a read operation). Compile the code and execute it using the echo server of the previous lab exercise.

> in order to see more clearly the difference between the blocking and non-blocking cases, once you have seen the above-described behaviour, comment the call to fcntl and check that the counter is not incremented before receiving the data.

## Asynchronous mechanisms using signals

The `SIGIO` signal is generated when the state of a socket changes, for example:

New data is available in the input buffer, so that new read operations can now be performed, or space has been freed in the output buffer, so that new write operations can now be performed.

A new client wishes to connect.
In order for the `SIGIO` signal to be generated, the following calls to the `fcntl` function must be made on the corresponding socket (`sd` in the following):

```c
   if ( fcntl(sd, F_SETFL, O_ASYNC | O_NONBLOCK) < 0 ) {
    perror("fcntl error");
  }

  if ( fcntl(sd, F_SETOWN, getpid()) < 0 ) {
    perror("fcntl error");
  }
```

Asynchronous mechanisms use the occurrence of this signal to know when data is available, enabling other tasks to be carried out while no data is received.

As we saw in the previous lab session, a signal handler for a given signal is a function specifying the actions to be carried out when that signal occurs. The `SIGIO` signal handler will perform operations of the type: read any data available in the input buffer of the socket, send any data that the application is waiting to send via the socket, accept any new clients that wish to establish to establish a connection.

### 2.	The code demand-accept.c is a simple example of a server that uses a `SIGIO` handler to detect when new clients connect. When the signal is generated, the handler accepts the connection of a new client, sends a message to this client and then closes the connection. Compile and test this code with a telnet client:

execute 
```
telnet localhost 9999 in a shell
```

## Controlling several descriptors using the select call

Usually, various clients connect to a server program simultaneously; To enable our server program to handle simultaneous connections, we have two options:

1 Create a new process or thread for each new connection as we saw in the last lab session.
2 Use the `select()` function, which we now look at in detail.

The `select()` function allows several sockets to be checked at the same time. It is used to know which of the sockets being handled by our program are ready to read or write data, which have received a new connection, which have generated an exception, etc.

The prototype of the `select()` function is as follows:

```c
#include <sys/time.h> 
  #include <sys/types.h> 
  #include <unistd.h> 

  int select(int numfds, fd_set *readfds, fd_set *writefds,
             fd_set *exceptfds, struct timeval *timeout);
```

where:
* `numfds` is the value of the highest value socket descriptor plus one. Recall that a descriptor is an integer returned when a file, socket, or similar is opened. In the usual case, the values assigned to these descriptors are consecutive.
* `readfds` is a pointer to the set of descriptors for which we wish to be notified when data is available to be read. We will also be notified when there is a new client or when a client closes the connection.
* `writefds` is a pointer to the set of descriptors for which we wish to be notified when we can write to them without any problem. If the connection has been closed by the other side and we try to write to it, we will receive the SIGPIPE signal.
* `exceptfds` is a pointer to the set of descriptors for which we wish to be notified when an exception has occurred.
* `timeout` is the maximum time that we wish to wait. A NULL value indicates that the call to select() is to remain blocked until something occurs on one of the descriptors. A zero value indicates that we wish to know if something has occurred on one of the descriptors without blocking.

`select()` returns -1 if an error occurs (see `errno`), 0 if the timer expires and a number greater than zero (the number of descriptors in the descriptor set) if the call succeeds.

`fd_set` is the type of the descriptor set; variables of this type are manipulated using macros. Suppose we have defined the descriptor set as `set`

```c
fd_set set;
```

then:
* `FD_ZERO(&set)` initialises (and deletes) the set of descriptors.
* `FD_SET(fd, &set)` adds a new descriptor to the set
* `FD_CLR(fd, &set)` removes a descriptor from the set.
* `FD_ISSET(fd, &set)` returns a number greater than zero if the descriptor fd is a member of the set. This macro can be used to find out if there is data available on fd after a call to select().

### 3.	The server in smart-select.c uses `select()` to handle client connections. Five (`MAXPROCESSES`) echo server processes are started (this can be observed using ps x) before any connections are received; all of them accept connections from clients simultaneously on the same socket. Each server process has its own echo clients; new connection requests are distinguished from the arrival of new data by using the select() function and the FD_ISSET() macro. Compile the example and test it using multiple clients.

How many clients are served by each process? How many clients could be served concurrently?

Decrement the numbers of processes (`MAXPROCESSES`) pre-launched by the server (e.g. to 2). Compile smart-select again. Launch 5 clients. What happens to the fifth one?






