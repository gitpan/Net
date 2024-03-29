/* sys.h - libnet's private header file
 * Copyright (c) 2003 Martin Sarfy <xsarfy@fi.muni.cz> */

#ifndef HEADER_SYS_H_INCLUDED
#define HEADER_SYS_H_INCLUDED


// microsoft
#ifdef WIN32
#define snprintf _snprintf
#endif

#define QUIET 
#define DEBUG 
#define PRINT printf
void NETERROR(char *fx);


/* create_net - defined in net.c */
Net create_net(int sys_type,int sock,char* peek,char* auth,void* data);
/* net_block - maybe we could move it to net.h */
Bool net_block(Net net,Bool block);


/*  Net::INET - bsd socket */
Bool inet_init();
Net  inet_accept(Net net);
int  inet_write(Net net,void *buffer,int len);
int  inet_read(Net net,void *buffer,int len);
void inet_block(Net net,Bool block);
void inet_close(Net net);

/*  Net::UNIX - local communication */
#define unix_read  inet_read
#define unix_write inet_write
#define unix_block inet_block
#define unix_close inet_close

/* tcp.c: socket manipulation */

int  tcp_listen(int port,void **p_sin);
int  tcp_accept(int lsock,void *sin_data,char *peer,int peer_size);
int  tcp_connect(const char *host,int port);
void tcp_block(int sock,Bool block);

/*	Net::SSL - OpenSSL support */
Bool ssl_init(const char* certfile,const char* keyfile,const char* password);
Net  ssl_accept(Net net);
int  ssl_write(Net net,void *buffer,int len);
int  ssl_read(Net net,void *buffer,int len);
void ssl_block(Net net,Bool block);
void ssl_close(Net net);



#endif

