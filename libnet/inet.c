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


/*	inet_read, inet_write, inet_close and inet_block 	*
 *	are shared with NET::UNIX - unix.c!					*/


// read as many data as you can
// returns -1 on EOF (connection closed), 0 on idle, 1+ as datalength
// -2+ on serious error
int inet_read(Net net,void *buffer,int len) {
	len = recv(net->sock,buffer,len,0);
/*	len = read(net->sock,buffer,len);*/
	if(len == -1) {
#ifdef WIN32
		if(WSAGetLastError() == WSAEWOULDBLOCK) return 0;
#else
		if(errno == EAGAIN) return 0;
#endif;
		NETERROR("recv()");
		return -2;
	}
	if(!len) return -1;
	return len;
}


// SHOULD return 0 on error!
int inet_write(Net net,void *chunk,int len) {
	int err = send(net->sock,chunk,len,0);
	if(err == -1) NETERROR("write()");
	return err; // length
}


// set non-blocking IO
void inet_block(Net net,Bool block) { 
	tcp_block(net->sock,block); 
}


void inet_close(Net net) {
	DEBUG("(inet|unix)_close: closing sock=%d\n",net->sock);
#ifdef WIN32
	closesocket(net->sock);
#else
	close(net->sock);
#endif
}


// returns NULL on error
Net inet_accept(Net net) {
	char peer[256];
	int sock;

	sock = tcp_accept(net->sock,net->data,peer,256);
	if(!sock) return NULL;
	DEBUG("connection from %s\n",peer);
	return create_net(NET_SYS_INET,sock,peer,NULL,NULL);
}


// constructor
Net net_server_inet(int port) {
	void *sin = NULL;
	char peer[10];
	int  sock;

	sock = tcp_listen(port,&sin);
	snprintf(peer,10,":%d",port);
	return create_net(NET_SYS_INET,sock,peer,(char*)NULL,(void*)sin);
}

// constructor
Net net_connect_inet(const char *host,int port) {
	char peer[256];
	int sock;

	sock = tcp_connect(host,port);
	snprintf(peer,256,"%s:%d",host,port);
	return create_net(NET_SYS_INET,sock,peer,NULL,NULL);
}


Bool inet_init() {
#ifdef WIN32
	WORD wVersionRequested = MAKEWORD(1,1);
	WSADATA wsaData;
	int nRet;

	// Initialize WinSock and check the version
	nRet = WSAStartup(wVersionRequested, &wsaData);
	if(wsaData.wVersion != wVersionRequested) {	
		fprintf(stderr,"winsock: wrong version\n");
		return false;
	}
#endif
	return true;
}


/* todo: WSACleanup() */

