#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "net.h"
#include "sys.h"

// htonl functions
#ifdef WIN32
#include<winsock.h>
#else
#include <netinet/in.h>
#endif

#define PERROR(fx) { perror(fx); exit(1); }

#define BUFF_SIZE_INITIAL	8192
#define BUFF_SIZE_INC		4096

//#define DEBUG(format,args...)
#define QUIET 
#define DEBUG 
#define PRINT printf

//typedef unsigned int uint32_t;

// base constructor
Net create_net(int sys_type,int sock,char *peer,char* auth,void *data) {
	Net net;

	net = calloc(1,sizeof(*net));
	net->sock = sock;
	net->blocking = 1;

	net->peer = peer ? strdup(peer) : NULL;
	net->auth = auth ? strdup(auth) : NULL;
	net->data = data;

	// empty, but existing buffer
	// we will realloc() it later
	net->buff_len = BUFF_SIZE_INITIAL;
	net->buffer = malloc(net->buff_len);

	DEBUG("creating NET(sock=%d)\n",net->sock);

	switch(sys_type) {

	case NET_SYS_INET:
		net->sys_read   = (void*)inet_read;
		net->sys_write  = (void*)inet_write;
		net->sys_close  = (void*)inet_close;
		net->sys_block  = (void*)inet_block;
		net->sys_accept = (void*)inet_accept;
		break;

	case NET_SYS_UNIX:
		net->sys_read   = (void*)unix_read;
		net->sys_write  = (void*)unix_write;
		net->sys_close  = (void*)unix_close;
		net->sys_block  = (void*)unix_block;
		net->sys_accept = NULL;
		break;
	
	case NET_SYS_SSL:
		net->sys_read   = (void*)ssl_read;
		net->sys_write  = (void*)ssl_write;
		net->sys_close  = (void*)ssl_close;
		net->sys_block  = (void*)ssl_block;
		net->sys_accept = (void*)ssl_accept;
		break;

	default:
		exit(6); // !!!
	}
	return net;
}

Bool net_block(Net net,Bool block) {
	if(block != net->blocking) {
		DEBUG("blocking(%d->%d on sock=%d)\n",net->blocking,block,net->sock);
		net->sys_block(net,block);
		net->blocking = block ? 1 : 0;
	}
	return net->blocking;
}


// 'constructor'
// returns NULL on error
Net net_accept(Net net,Bool wait) {
	net_block(net,wait);
	if(!net->sys_accept) return NULL; // can't do accept on this type
	return net->sys_accept(net);
}

#ifndef uint32_t
typedef unsigned int uint32_t;
#endif

// returns 0 on error or length on success
// fix: it dies on error instead of returing 0
int net_write(Net net,void *chunk,int len) {
	int			err;
	uint32_t	len32;

	net_block(net,1);

	if(!len) exit(3); // fix: write EINVAL

	len32 	= htonl(len);
	err = net->sys_write(net,&len32,sizeof(len32));
	if(err == -1) PERROR("write32()");

	DEBUG("writing %d bytes\n",len);
	err = net->sys_write(net,chunk,len);
	if(err == -1) PERROR("write()");

	return len;
	return 0;
}



// return chunk
// huh, not in-site, too much malloc/memcpy/free :-(
// returns 
// NULL and 0 in *len on idle, 
// NULL and negative number (NET_PEEK_* constant) in *len on error
// data/length on success, caller must free() data
void* net_read(Net net,int *len) {
	void* buffer;

	if(!len) exit(3); // be polite, write something!! (EINVAL)

	*len = net_peek(net);

	// error, closed or idle
	if(*len <= 0) return null;

	buffer = malloc(*len);
	memcpy(buffer, net->buffer+4, (*len));

	// shrink buffer
	net->real_len -= (4 + (*len));
	memmove(net->buffer, net->buffer+4+(*len), net->real_len);

	DEBUG("%d bytes read\n",(*len));
	return buffer;
}

char* net_peer(Net net) { return net->peer; }
char* net_auth(Net net) { return net->auth; }

// returns error -2, closed -1, idle 0, ready $msglen
// => msg can NOT have 0 bytes length! 
// actually it's net_peek who is doing real sys_read()
int net_peek(Net net) {
	uint32_t header;
	int len;

	net_block(net,0);

	if(!net->buffer) return NET_PEEK_CLOSED;

	// ensure we have enough space 
	if(net->buff_len - net->real_len < BUFF_SIZE_INC)
		net->buffer = realloc(net->buffer, 
							  net->buff_len += BUFF_SIZE_INC );

	// read to the end of buffer
	len = net->sys_read(net,net->buffer   + net->real_len,
						    net->buff_len - net->real_len );
	net->real_len += (len < 0) ? 0 : len;

	DEBUG("peek: len=%d, buff_len=%d, real_len=%d",
					len,net->buff_len,net->real_len);

	if(net->real_len < 4) return (len < 0) ? NET_PEEK_ERROR : NET_PEEK_IDLE;

	header = *((uint32_t*)(void*)net->buffer);
	header = ntohl(header);

	// null-sized messages not supported
	if(!header) return NET_PEEK_ERROR;

	if(4 + header <= (unsigned)net->real_len) return header;

	return (len < 0) ? NET_PEEK_ERROR : NET_PEEK_IDLE;
/*	if(len < 0) {
		// unexcepted end, fix: errno==EIO ??
		if(len != -1) return NET_PEEK_ERROR;
		if(net->real_len == 0) return NET_PEEK_CLOSED;
		if(net->real_len <  4) return NET_PEEK_ERROR;
		// if(4 + header < net->real_len) 
//		return NET_PEEK_ERROR;
	}

	return (4 + header > net->real_len) ? NET_PEEK_IDLE : header; */
}


void net_close(Net net) {
	net->sys_close(net);
	DEBUG("closing sock=%d\n",net->sock);
	if(net->buffer) free(net->buffer);
	net->buffer	= null; // fuse
	net->sock	= 0;	// fuse 
	if(net->peer) free(net->peer);
	if(net->auth) free(net->auth);
	free(net);
}

Bool net_init(char *certfile,char* keyfile,char* password) {
	if(!inet_init()) return 0;
	if(!ssl_init(certfile,keyfile,password)) return 0;
	return 1;
}

// sigfault on m$ - malloc/free seems to diff in net.c and Net.xs
void net_free(void *buffer) { free(buffer); }


