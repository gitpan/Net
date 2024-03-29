#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#ifdef WIN32
#include<winsock.h>
#else
#include <netdb.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#endif

#include "net.h"
#include "sys.h"


void NETERROR(char *fx) {
#ifdef WIN32
	fprintf(stderr,"%s: %d\n",fx,WSAGetLastError());
	exit(1);
#else
	perror(fx);
	exit(1);
#endif
}


// returns NULL on error
int tcp_accept(int lsock,void *sin_data,char *peer,int peer_size) {
	int sock;

#ifdef WIN32
	SOCKADDR_IN *sin;
	int sin_len = sizeof(SOCKADDR_IN);
	sin = (SOCKADDR_IN *)sin_data;
#else
	struct sockaddr_in *sin;
	int sin_len = sizeof(struct sockaddr_in);
	sin = (struct sockaddr_in *)sin_data;
#endif

	if(!sin) exit(5); // doing accept on non-server socket

	DEBUG("listening on port %d\n",ntohs(sin->sin_port));

	sock = accept(lsock,(void*)sin,&sin_len);

	if(sock == -1) {
#ifdef WIN32
		if(WSAGetLastError() == WSAEWOULDBLOCK) return 0;
#else
		if(errno == EAGAIN) return 0;
#endif;
		   else NETERROR("accept()");
	}

	snprintf(peer,peer_size,"%s:%d",
			 inet_ntoa(sin->sin_addr),ntohs(sin->sin_port));
	return sock;
}



int tcp_listen(int port,void **p_sin) {
#ifdef WIN32
	SOCKADDR_IN *sin;
#else
	struct sockaddr_in *sin;
#endif
	int  sock, yes=1;

	sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock == -1) NETERROR("socket()");

	sin = malloc(sizeof(*sin));
	sin->sin_family = AF_INET;
	sin->sin_addr.s_addr = INADDR_ANY;
	sin->sin_port = htons((short)port);

	if(bind(sock,(struct sockaddr*)sin,sizeof(*sin))) NETERROR("bind()");

	// SOMAXCONN = 5 on m$
	if(listen(sock,SOMAXCONN)) NETERROR("listen()");

#ifndef WIN32
	if(setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)))
			NETERROR("setsockopt()");
#endif

	if(p_sin) *p_sin = sin;
	return sock;
}

int tcp_connect(const char *host,int port) {
#ifdef WIN32
	LPHOSTENT hp;
	SOCKADDR_IN sin;
#else
	struct	hostent *hp;
	struct sockaddr_in sin;
#endif
	int sock;

	sock = socket(AF_INET,SOCK_STREAM,0);
	if(sock == -1) NETERROR("socket()");

	if(!(hp = gethostbyname(host))) {
#ifdef WIN32
		fprintf(stderr,"gethostbyname(): %d\n",WSAGetLastError());
#else
		herror("gethostbyname");
#endif
		exit(1);
	}

	sin.sin_family = AF_INET;
	sin.sin_addr = **((struct in_addr **)hp->h_addr_list);
	sin.sin_port = htons((short)port);

	if(connect(sock,(struct sockaddr*)&sin,sizeof(sin))) {
		fprintf(stderr,"connect('%s',%d)\n",host,port);
		NETERROR("connect()");
	}

	DEBUG("connected to %s:%d\n",host,port);

	return sock;
}


// set non-blocking IO
void tcp_block(int sock,Bool block) {
#ifdef WIN32
    u_long arg = !block;
    if(ioctlsocket(sock,FIONBIO,&arg)) NETERROR("ioctlsocket()");
#else
	int  flags;
	flags = fcntl(sock,F_GETFL);
	if(flags == -1) NETERROR("fcntp()");
	flags = block ? (flags & (~O_NONBLOCK)) : (flags | O_NONBLOCK);
	flags = fcntl(sock,F_SETFL,flags);
	if(flags == -1) NETERROR("fcntl()");
#endif
}

