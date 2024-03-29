/*
 * C<Net> is nonblocking, autoflushing communication endpoint. 
 *
 * C<Net::INET> is connection over TCP/IP network, C<Net::UNIX> is for
 * unix socket based communication and C<Net::SSL> is TCP/IP using
 * OpenSSL layer.
 *
 * Copyright (c) 2003 Martin Sarfy <xsarfy@fi.muni.cz> */

#ifndef HEADER_NET_H_INCLUDED
#define HEADER_NET_H_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif


#define NET_PEEK_ERROR -2
#define NET_PEEK_CLOSED	-1
#define NET_PEEK_IDLE	 0
//#define NET_PEEK_READY	 1

#ifndef null
#define null 0L
#endif

#ifndef Bool
typedef int Bool;
#endif
#ifndef false
#define false 0
#endif
#ifndef true
#define true  1
#endif

struct NetStruct {
	int 	sock;
	char*	peer;
	char*	auth;
	Bool	blocking;

	char	*buffer;
	int		buff_len; // mallocated length
	int		real_len; // really readed length

	void	*data;	  // unix/inet/ssl private data

	int  sys_type; // NET_INET, NET_SSL, NET_UNIX, NET_BFISH
	void* (*sys_accept)(void *net); // returns new Net object
	int  (*sys_read)(void *net,void *buffer,int len);
	int  (*sys_write)(void *net,void *buffer,int len);
	void (*sys_block)(void *net,Bool block);
	void (*sys_close)(void *net);

};

#ifdef Net
#undef Net
#endif
typedef struct NetStruct * Net;

// constructors:

Bool net_init();
void net_free(void *buffer);

// 		server:
Net net_server_inet(int port);
Net net_server_ssl(int port);
Net net_server_unix(const char *file);

// 		connect:
Net net_connect_inet(const char *host,int port);
Net net_connect_ssl(const char *host,int port);
Net net_connect_unix(const char *file);
// Net *connect_ssl(const char *host,int port,const char *certfile);

//		socketpair:
Bool net_socketpair_unix(Net net[2]);
// (Net a,Net b) net_socketpair_unix()

Net   net_accept(Net net,Bool wait);
int   net_peek(Net net);
void* net_read(Net net,int *len);
int   net_write(Net net,void *chunk,int len);
char* net_peer(Net net);
char* net_auth(Net net);
void  net_close(Net net);

#define NET_SYS_INET 1
#define NET_SYS_UNIX 2
#define NET_SYS_SSL  4


#ifdef __cplusplus
}
#endif


#endif

