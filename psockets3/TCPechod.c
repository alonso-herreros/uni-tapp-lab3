/* TCPechod.c - main, TCPechod */ 

		  
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define	QLEN		   5	/* maximum connection queue length	*/
#define	BUFSIZE		4096    /* maximum data length	*/

extern int	errno;

void	reaper(int);
int	errexit(const char *format, ...);


/*------------------------------------------------------------------------
 * main - Concurrent TCP server for ECHO service
 *------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
	char	*port = "";	/* service name or port number	*/
	
	int	alen;			/* length of client's address	*/
	
	struct sockaddr_in sin;	/* an Internet endpoint address		*/	
	struct	sockaddr_in fsin;	/* the address of a client	*/

	int	psock;			/* passive server socket - for listening to incomming connections */
	int	asock;			/* active server socket	- for communicating with the client */
	
	/* variable to print information */
	char str[INET_ADDRSTRLEN];

	/* buffer to receive client data */
	char	buf[BUFSIZ];
	int	cc;

	switch (argc) {
	case	1:
		break;
	case	2:
		port = argv[1];
		break;
	default:
		errexit("usage: TCPechod [port]\n");
	}

	/* Passive socket initialization */

	/* First thing to do, erase the structure sin filling it up with zeroes */
	memset(&sin, 0, sizeof(sin));
	/* Let's configure the passive socket so the server can listen for incoming 
	 * connections. Since machines may have several network interfaces, we need 
	 * to specify one to be used */	

	/* AF_INET is the address family that is used for the socket we are creating
	 * (in this case an Internet Protocol address) */
	sin.sin_family = AF_INET;

	/* When INADDR_ANY is specified in the bind call, the socket will be bound to 
         * all local interfaces */
	sin.sin_addr.s_addr = INADDR_ANY;
	
	/* We need to specify the port in which the server will listen to incomming 
         * connections */
	sin.sin_port = htons((u_short)atoi(port));

	/* now, we can allocate a socket! 
	 * The socket allocation requires to identify:
	 * - The protocol family : PF_INET
         * - The type of procotocol: STREAM or DATAGRAM (Stream in this case : SOCK_STREAM)
         * - Which STREAM or Datagram protocol should be used (TCP in this case: IPPROTO_TCP)
	 */
	psock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	/* Bind the socket: assigns the socket to the requested port */
	if (bind(psock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		errexit("can't bind to %s port: %s\n", port, strerror(errno));
	/* start listening to incoming connections */	
	if (listen(psock, QLEN) < 0)
		errexit("can't listen on %s port: %s\n", port,
			strerror(errno));

	/* SIGNAL HANDLER */
	(void) signal(SIGCHLD, reaper);

	while (1) { // This loop will run forever (1==true)
		
		
		alen = sizeof(fsin);
		/* when the execution flow arrives here, it will enter accept and will wait
		 * sleeping until a new connection arrives. When that happens, the information
		 * about the client that is connecting will be copied to fsin.
		 * We will get an active socket to communicate with the client.
		 */		
		printf("Parent: Waiting for incomming connections at port %s\n", port);			
		asock = accept(psock, (struct sockaddr *)&fsin, &alen);
		inet_ntop(AF_INET, &(fsin.sin_addr), str, INET_ADDRSTRLEN);
		printf("Parent: Incomming connection from %s remote port %d\n", str, ntohs(fsin.sin_port));		
		if (asock < 0) {
			if (errno == EINTR)
				continue;
			errexit("accept: %s\n", strerror(errno));
		}
		/* Let's create a new process to answer the incoming connection */

		switch (fork()) {
			case 0:	/* child */

				/* since the child process do not need a passive socket 
				 * and that passive socket belongs to the parent process
				 * we can close it without closing the real passive socket
				 * that is in parent's memory space -- REMEMBER */
				(void) close(psock);
				/* echo protocol implementation:
				 * upon reception of a message from the client, the server should copy
				 * back the same information. We use the active socket. */

				/* Read from incoming stream using the active socket. cc is the received length */
				while (cc = read(asock, buf, sizeof(buf))) {
					if (cc < 0)
						errexit("echo read: %s\n", strerror(errno));
					/* print to console */	
					printf("\tchild %d : receiving data: \n",getpid());	
					write(1,buf,cc); /* stdout is the 1 file descriptor */
					/* write back to the client */		
					if (write(asock, buf, cc) < 0)
						errexit("echo write: %s\n", strerror(errno));
				}				
				exit(0);
			default: /* parent */

				/* since the parent process do not need a active socket 
				 * we can close it without closing the real active socket
				 */
				(void) close(asock);
				/* go back listening to incoming connections */
				break;
			case -1:
				errexit("fork: %s\n", strerror(errno));
		}

	
	}
}

/*------------------------------------------------------------------------
 * reaper - clean up zombie children
 *------------------------------------------------------------------------
 */
/*ARGSUSED*/

void reaper(int sig)
{
	int	status;

	while (wait3(&status, WNOHANG, (struct rusage *)0) >= 0)
				/* empty */;
}

