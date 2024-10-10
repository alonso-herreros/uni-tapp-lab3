/* TCPecho.c - main, TCPecho */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <netinet/tcp.h>

#ifndef	INADDR_NONE
#define	INADDR_NONE	0xffffffff
#endif	/* INADDR_NONE */

extern int	errno;

int	errexit(const char *format, ...);

#define	LINELEN		1500

/*------------------------------------------------------------------------
 * main - TCP client for ECHO service
 *------------------------------------------------------------------------
 */
int
main(int argc, char *argv[])
{
	char	*host = "localhost";	/* host to use if none supplied	*/
	char	*port = "echo";	/* default service name		*/
	int	s, n;			/* socket descriptor, read count*/
	struct sockaddr_in sin;	/* an Internet endpoint address		*/
	struct hostent	*phe;	/* pointer to host information entry	*/
	int	outchars, inchars;	/* characters sent and received	*/
	char	buf[LINELEN+1];		/* buffer for one line of text	*/
//	int no_nagle= 1;

	switch (argc) {
	case 1:
		host = "localhost";
		break;
	case 3:
		port = argv[2];
		/* FALL THROUGH */
	case 2:
		host = argv[1];
		break;
	default:
		fprintf(stderr, "usage: TCPecho [host [port]]\n");
		exit(1);
	}

	/* First thing to do, erase the structure sin filling it up with zeroes */
	memset(&sin, 0, sizeof(sin));
	/* AF_INET is the address family that is used for the socket we are creating
	 * (in this case an Internet Protocol address) */
	sin.sin_family = AF_INET;

        /* Assign port number to connect to */
	sin.sin_port = htons((u_short)atoi(port));
		
	/* Map host name to IP address, allowing for dotted decimal
         * using this way to assign the IP address, we allow to use
	 * either the IP address or the host name. gethostbyname will
	 * make a DNS request if necessary to find out the IP address */
	if ( phe = gethostbyname(host) )
		memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
	else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
		errexit("can't get \"%s\" host entry\n", host);

   	/* now, we can allocate a socket! 
	 * The socket allocation requires to identify:
	 * - The protocol family : PF_INET
         * - The type of procotocol: STREAM or DATAGRAM (Stream in this case : SOCK_STREAM)
         * - Which STREAM or Datagram protocol should be used (TCP in this case: IPPROTO_TCP)
	 */  
	s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s < 0)
		errexit("can't create socket: %s\n", strerror(errno));

//	if (setsockopt(s,SOL_TCP,TCP_NODELAY,&no_nagle, sizeof(no_nagle)) != 0)
//		errexit("setsockopt: no pude deshabilitar Nagle");

        /* Connect the socket */
	if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
		errexit("can't connect to %s.%s: %s\n", host, port,
			strerror(errno));
	/* don't even think of using other function than fgets for this */
	while (fgets(buf, LINELEN+1, stdin)) {
		buf[LINELEN] = '\0';	/* insure line null-terminated	*/
		outchars = strlen(buf);
		/* write to socket */
		(void) write(s, buf, outchars);

		/* read it back */
		for (inchars = 0; inchars < outchars; inchars+=n ) {
		  n = read(s, &buf[inchars], outchars - inchars);
			if (n < 0)
				errexit("socket read failed: %s\n",
					strerror(errno));
		}
		/* write answer to the console */
		fputs(buf, stdout);
	}
	
	exit(0);
}


