[English version](README_EN.md)

# Servidores concurrentes con el API de sockets

### Material de soporte
-  On-line man pages: socket(2), socket(7), send(2), recv(2), read(2), write(2), setsockopt(2), fcntl(2), select(2), tcp(7), ip(7).
-  Guide to using sockets by Brian "Beej" Hall
-  Tcpdump manual
-  Chapters 6, 7 y 8 of "Linux Socket Programming" by Sean Walton, Sams Publishing Co. 2001
-  Fichero cheat sheet en proyecto anterior

### Prácticas con sockets
Descripción de las prácticas de sockets Las prácticas de sockets se dividen en tres partes:
1. Servidores secuenciales (cliente y servidor de eco, opciones de sockets, análisis con tcpdump, servidores de ficheros)
2. Servidores concurrentes (procesos, hilos).
3. **Entrada/Salida I/O (manejadores de señales, mecanismos de polling, select)**

## Entrada/salida bloqueante y no bloqueante

Las operaciones de entrada/salida son por lo general bloqueantes y esto también ocurre cuando se utilizan con sockets. 
Esto implica que cuando se realiza alguna de estas operaciones sobre un socket, el proceso pasa al estado dormido, esperando que se satisfaga alguna condición que permita que se complete la operación. Así: 

* Si realizamos operaciones de entrada (`read`, `recv`, `recvfrom`...) sobre un socket TCP y no hay datos disponibles en el buffer de recepción del socket, el proceso pasa a estado dormido hasta que lleguen datos.
* Si realizamos operaciones de salida (`write`, `send`, `sendto`...) sobre un socket TCP, cuando nosotros realizamos esta llamada, el kernel copia los datos del buffer de la aplicación en el buffer de envío del socket; si no queda espacio en este buffer, el proceso se bloquea hasta que haya suficiente espacio.

En muchas ocasiones es recomendable emplear algún mecanismo que nos permita realizar estas operaciones de forma no bloqueante y así poder realizar otras tareas en vez de esperar a que los datos estén disponibles. En esta práctica vamos a ver algunos de los mecanismos que existen para poder realizar operaciones de entrada/salida no bloqueantes sobre sockets, en concreto, los mecanismos de polling y los mecanismos asíncronos.

Para esta práctica **usaremos al comienzo los ficheros de código de la práctica [sockets2]((https://gitlab.gast.it.uc3m.es/aptel/sockets2_concurrent_servers))** pero hay ciertos ficheros nuevos que podrás descargar usando el siguiente comando:

 ```
 git clone https://gitlab.gast.it.uc3m.es/aptel/sockets3_concurrent_servers_polling_select.git
 ```

### La función `fcntl()` y los mecanismos de polling
La función `fcntl()` es una función de control que nos permite realizar diferentes operaciones sobre descriptores (de ficheros, de sockets,...) en general. El prototipo de la función es el siguiente:

```c
#include <fcntl.h>
int fcntl(int fd, int cmd, /* int arg*/);
```

Cada descriptor tiene asociado un conjunto de flags que nos permiten saber o conocer ciertas características del descriptor. Para obtener el valor de estos flags, se realiza una llamada a `fcntl()` con el parámetro `cmd` al valor `F_GETFL`. De un modo similar cuando queremos modificar el valor de los flags, se utiliza el valor `F_SETFL`. 

Se recomienda ver detalladamente el uso de esta función leyendo la página del manual fcntl(2). Para indicar que las operaciones de entrada y salida sobre un socket no sean bloqueantes, es necesario activar el flag `O_NONBLOCK` en el descriptor del socket. El código necesario para ello es el siguiente: 

```c
 // sd es el descriptor del socket
 if (fcntl(sd, F_SETFL, O_NONBLOCK) < 0)
  perror("fcntl: no se puede fijar operaciones no bloqueantes");
```

De esta forma ya sabemos cómo activar que las operaciones de lectura y escritura no sean bloqueantes, pero *¿cómo sabemos cuando están los datos disponibles?*: 

> Cuando el socket no es bloqueante, si al realizar una operación de lectura o escritura ésta no se puede completar, la llamada devuelve un error (`-1`) y le asignará a la variable `errno` el valor `EWOULDBLOCK` (de todas formas, recordad, que es necesario comprobar el número de bytes que devuelven estas llamadas, porque no siempre coincide con el número de bytes que queríamos leer o escribir). Así, para saber cuando existen datos disponibles se suele utilizar un mecanismo de "encuesta", denominado polling, en el que se consulta continuamente cuándo existen datos disponibles y si no los hay, se realizan otras tareas.

### 1. Utilizando el código de la [práctica 3](https://gitlab.gast.it.uc3m.es/aptel/sockets2_concurrent_servers) modifica el cliente para que se puedan realizar operaciones de entrada/salida no bloqueantes, y emplea un mecanismo de polling para saber si existen datos disponibles cuando se realizan operaciones de lectura.

Para ver este comportamiento, haz que se imprima en pantalla un contador, que se incremente cada vez que el programa tiene que esperar por los datos que devuelve el servidor (operación de lectura). Compila el código y ejecútalo utilizando el servidor de eco concurrente de la práctica anterior.

> para ver mejor la diferencia con el caso de sockets no bloqueantes, elimina del cliente anterior la llamada a fcntl() y comprueba que no se incrementa el contador antes de recibir los datos.

## Mecanismos asíncronos utilizando señales

La señal `SIGIO` se genera cuando cambia el estado de un socket, por ejemplo:

* Cuando existen nuevos datos disponibles en el buffer de recepción o se ha liberado espacio en el buffer de envío y, por lo tanto, podemos realizar nuevas operaciones de escritura.
* Cuando existen nuevos clientes que se quieren conectar.

Para que se genere la señal de `SIGIO` tenemos que realizar las siguientes llamadas a `fcntl()` sobre el socket correspondiente (`sd` en el ejemplo):

```c
 if ( fcntl(sd, F_SETFL, O_ASYNC | O_NONBLOCK) < 0 ) {
 perror("fcntl error");
 }
 if ( fcntl(sd, F_SETOWN, getpid()) < 0 ) {
 perror("fcntl error");
```

Los mecanismos asíncronos utilizan esta señal para saber cuando están listos los datos en un socket y de esta forma poder realizar otras tareas mientras no se reciben datos.

Como ya hemos visto en la práctica anterior, cuando se genera una señal podemos hacer que se ejecute un manejador en el que codificamos las acciones que se deben llevar a cabo cuando la señal se produce. En el caso de la señal `SIGIO` realizaremos operaciones del tipo: leer datos que se encuentran disponibles en el buffer de recepción del socket, enviar los datos que tiene pendientes la aplicación, y/o aceptar nuevos clientes que quieren establecer conexiones.  

### 2. El código demand-accept.c es un ejemplo sencillo de un servidor que utiliza un manejador de SIGIO para detectar cuándo se conectan nuevos clientes. 

Así, cuando se genera la señal, el manejador se encarga de: aceptar la conexión con el nuevo cliente, enviar un mensaje, y cerrar la conexión. 

Compílalo y pruébalo con un cliente telnet. Para ello ejecutad en un terminal, una vez arrancado el servidor: 
```
telnet localhost 9999
```
## Control de varios descriptores usando la llamada `select`

Normalmente a un programa servidor se conectan varios clientes simultáneamente y, por ello, nuestros programas deben estar preparados para esta circunstancia. Para implementar esto tenemos dos posibles opciones:

1. Crear un nuevo proceso o hilo por cada cliente que llegue, que es lo que hemos visto en la práctica anterior de sockets.
2. Utilizar la llamada select(), que vamos a ver ahora en detalle. 

La llamada `select()` nos permite comprobar el estado de varios sockets al mismo tiempo. Con ella podemos saber qué sockets de los que maneja nuestro programa están listos para leer datos, para escribir datos, cuáles reciben conexiones, cuáles generan excepciones... 

El prototipo de la función select() es el siguiente:

```c
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
 int select(int numfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout);
```

Los parámetros de la función son los siguientes:

* `numfds`: es el valor del descriptor de socket más alto que queremos tratar más uno. Cada vez que abrimos un fichero, socket o similar, se nos da un descriptor de fichero que es un número entero. Estos descriptores suelen tener valores consecutivos.
* `readfds`: es un puntero a los descriptores de los que nos interesa saber si hay algún dato disponible para leer (es decir, sobre los que queremos que nos avisen cuando haya datos). También se nos avisará cuando haya un nuevo cliente o cuando un cliente cierre la conexión.
* `writefds`: es un puntero a los descriptores de los que nos interesa saber si podemos escribir en ellos sin peligro. Si en el otro lado han cerrado la conexión e intentamos escribir, se nos enviará una señal `SIGPIPE`.
* `exceptfds`: es un puntero a los descriptores de los que nos interesa saber si ha ocurrido alguna excepción.
* `timeout`: es el tiempo que queremos esperar como máximo. Si pasamos `NULL`, nos quedaremos bloqueados en la llamada a `select()` hasta que suceda algo en alguno de los descriptores. Se puede poner un tiempo cero si únicamente queremos saber si hay algo en algún descriptor, sin quedarnos bloqueados.

La función `select()` nos devuelve: `-1` en caso de error (ver `errno`), `0` si venció el temporizador, y un número mayor que cero (el número de descriptores en los conjuntos de descriptores) en caso de éxito. 

`fd_set` es el tipo de los conjuntos de descriptores, y las variables de este tipo se manipulan con macros. Si suponemos que hemos definido un conjunto `fd_set` set;
* `FD_ZERO(&set)` inicializa (y borra) el conjunto de descriptores.
* `FD_SET(fd, &set)` añade un nuevo descriptor al conjunto.
* `FD_CLR(fd,&set)` quita un descriptor del conjunto.
* `FD_ISSET(fd,&set)` devuelve mayor que cero si el descriptor fd se encuentra en el conjunto. Es la función que utilizamos para saber si después de una llamada a `select()` hay datos listos en el descriptor `fd`. 

### 3. En el fichero smart-select.c se encuentra el código de un servidor que utiliza `select()` para atender a los clientes. 

El servidor pre-lanza cinco procesos (MAXPROCESSES) de eco (puede verse con el comando `ps x`), de forma que todos ellos aceptan conexiones de clientes de forma simultánea en el mismo socket. 
Cada servidor tiene sus propios clientes de eco, y la forma de diferenciar si es una nueva petición o si llegan datos de alguno de los clientes se hace a través de `select()` y de la macro `FD_ISSET()`.

Compila el ejemplo y pruébalo con varios clientes.

1. ¿Cuántos clientes atiende cada proceso? ¿Cuántos clientes se podrían atender de forma concurrente?
1. Decrementa el número de procesos (`MAXPROCESSES`) que pre-lanza el servidor, por ejemplo a 2. Compila smart-select.c nuevamente, lanza 5 clientes, ¿Qué ocurre con el quinto?

