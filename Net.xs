#define PERL_NO_GET_CONTEXT
#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

#include<stdio.h>
#include "libnet/net.h"


MODULE = Net	PACKAGE = Net

bool
init(pkg,certfile,keyfile,password)
	char*	pkg
	char*	certfile
	char*	keyfile
	char*	password
CODE:
	RETVAL = net_init(certfile,keyfile,password);
OUTPUT:
	RETVAL

Net
server(pkg,sys_type,...)
	char*	pkg
	char*	sys_type
PREINIT:
	int	port;
CODE:
	if(!strcmp(sys_type,"INET")) {
		if(items == 3) port = SvIV(ST(2));
				 else croak("Usage: Net->server('INET',port)");
		RETVAL = net_server_inet(port);
	} else 
	if(!strcmp(sys_type,"UNIX")) {
		croak("named pipes not implemented");
	} else
	if(!strcmp(sys_type,"SSL")) {
		if(items == 3) port = SvIV(ST(2));
				 else croak("Usage: Net->server('SSL',port)");
		RETVAL = net_server_ssl(port);
	} else
	croak("unsupported sys_type: %s",sys_type);
OUTPUT:
	RETVAL

Net
accept(net,wait=1)
	Net net
	bool wait
CODE:
	RETVAL = net_accept(net,wait);
OUTPUT:
	RETVAL

Net
connect(pkg,sys_type,...)
	char*	pkg
	char*	sys_type
PREINIT:
	char*	host;
	int		port;
CODE:
	if(!strcmp(sys_type,"INET")) {
		if(items == 4) {
			host = SvPV(ST(2),PL_na);
			port = SvIV(ST(3));
		} else croak("Usage: Net->connect('INET',host,port)");
		RETVAL = net_connect_inet(host,port);
	} else
	if(!strcmp(sys_type,"UNIX")) {
		croak("named pipes not implemented");
	} else 
	if(!strcmp(sys_type,"SSL")) {
		if(items == 4) {
			host = SvPV(ST(2),PL_na);
			port = SvIV(ST(3));
		} else croak("Usage: Net->connect('SSL',host,port)");
		RETVAL = net_connect_ssl(host,port);
	} else
	croak("unsupported sys_type: %s",sys_type);
OUTPUT:
	RETVAL


void
socketpair(pkg)
	char*	pkg
PREINIT:
	Net		net[2];
	SV*		sv0;
	SV*		sv1;
PPCODE:
	if(!net_socketpair_unix(net)) XSRETURN_UNDEF;
	/* im not sure whether i'm doing this well */
	EXTEND(SP, 2);
	sv0 = sv_newmortal();
	sv_setref_pv(sv0,"Net",(void*)net[0]);
	PUSHs(sv0);
	sv1 = sv_newmortal();
	sv_setref_pv(sv1,"Net",(void*)net[1]);
	PUSHs(sv1);


int
peek(net)
	Net net
CODE:
	RETVAL = net_peek(net);
OUTPUT:
	RETVAL

int
write(net,chunk)
	Net	net
	SV*	chunk
PREINIT:
	void*	buffer;
	int		len;
CODE:
	buffer = (void*)SvPV(chunk,len);
	RETVAL = net_write(net,buffer,len);
OUTPUT:
	RETVAL

SV*
read(net)
	Net net
PREINIT:
	int	len;
	void* buffer;
CODE:
	buffer = (void*)net_read(net,&len);
	if(!buffer) {
		if(len < 0) { XSRETURN_UNDEF; }    /* error - undef */
			   else { RETVAL = newSV(0); } /* idle  - ""    */
	} else {
		RETVAL = newSVpv((char*)buffer,len);
		net_free(buffer);
	}
OUTPUT:
	RETVAL

char*
peer(net)
	Net net
CODE:
	RETVAL = net_peer(net);
OUTPUT:
	RETVAL

char*
auth(net)
	Net net
CODE:
	RETVAL = net_auth(net);
OUTPUT:
	RETVAL

void
DESTROY(net)
	Net net
CODE:
	net_close(net);




