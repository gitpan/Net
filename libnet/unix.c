#include<stdio.h>
#include"net.h"
#include"sys.h"

//#define DEBUG(format,args...)
#define QUIET 
#define DEBUG 
#define PRINT printf
#define PERROR(fx) { perror(fx); exit(1); }

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#else
#include<windows.h>
#include<winsock.h>

// microsoft
#define snprintf _snprintf

static void NETERROR(char* fx) {
	fprintf(stderr,"%s: %d\n",fx,WSAGetLastError());
	exit(11);
}

// code stolen from perl's util.c
static emulate_socket_pair(int fd[2]) {
    int listener  = -1;
    int connector = -1;
    int acceptor  = -1;
    SOCKADDR_IN listen_addr;
    SOCKADDR_IN connect_addr;
    int size;
	int family = AF_INET;

	listener = socket(family,SOCK_STREAM,0);
	if(listener == -1) NETERROR("socket()");

	listen_addr.sin_family = family;
	listen_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	listen_addr.sin_port = 0;

	if(bind(listener,(SOCKADDR*)&listen_addr,
				 sizeof(listen_addr))) NETERROR("bind()");

	if(listen(listener,1)) NETERROR("listen()");

	connector = socket(family,SOCK_STREAM,0);
	if(connector == -1) NETERROR("socket()");

	size = sizeof(connect_addr);
	if(getsockname(listener,(SOCKADDR*)&connect_addr,
				&size) == -1) NETERROR("getsockname()");

	connect_addr.sin_family = family;
	connect_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
//	connect_addr.sin_port = listen_addr.sin_port;

	if(connect(connector,(SOCKADDR*)&connect_addr,
				sizeof(connect_addr)) == -1) NETERROR("connect()");

	size = sizeof(listen_addr);
	acceptor = accept(listener,(SOCKADDR*)&listen_addr,&size);
	if(acceptor == -1) NETERROR("accept()");
	DEBUG("connected/accepted port: %d, %d <-> %d\n",
			connect_addr.sin_port,listener,connector);

	if(getsockname(connector,(SOCKADDR*)&connect_addr,
				&size) == -1) NETERROR("getsockname()");

	if (size != sizeof (connect_addr)
        || listen_addr.sin_family != connect_addr.sin_family
        || listen_addr.sin_addr.s_addr != connect_addr.sin_addr.s_addr
        || listen_addr.sin_port != connect_addr.sin_port) {
			closesocket(connector);
			closesocket(listener);
			return false;
	}

	fd[0] = connector;
	fd[1] = acceptor;

	return true;
}

#endif


/*	unix_read, unix_write, unix_close and unix_block 	*
 *	are shared with Net::INET, #defined in sys.h		*/

// the only constructor
Bool net_socketpair_unix(Net net[2]) {
	int  fd[2];
	char peer[64];
	unsigned int  pid;

	if(!net) exit(7);

#ifdef WIN32
	pid = (int)GetCurrentProcessId();
	if(!emulate_socket_pair(fd)) exit(10);
#else
	pid = getpid();
	if(socketpair(AF_UNIX, SOCK_STREAM, 0, fd)) PERROR("socketpair()");
#endif

	snprintf(peer,64,"%u/%d",pid,fd[0]);
	net[0] = create_net(NET_SYS_UNIX,fd[0],peer,NULL,NULL);
	snprintf(peer,64,"%u/%d",pid,fd[1]);
	net[1] = create_net(NET_SYS_UNIX,fd[1],peer,NULL,NULL);

	return 1;
}

