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

#include<openssl/ssl.h>
#include<openssl/err.h>

#include "net.h"
#include "sys.h"

// global SSL context
SSL_CTX *ctx;

static void STRERROR(const char* str) {
	fprintf(stderr,"error: %s\n",str);
	exit(1);
}

static void SSLERROR(SSL *ssl,char* fx,int err) {
	int code;
	char *str = NULL;
	if(err > 0) return; // no error

	code = SSL_get_error(ssl,err);
	if(code == SSL_ERROR_NONE) 				str = "SSL_ERROR_NONE";
	if(code == SSL_ERROR_ZERO_RETURN)		str = "SSL_ERROR_ZERO_RETURN";
	if(code == SSL_ERROR_WANT_READ) 		str = "SSL_ERROR_WANT_READ";
	if(code == SSL_ERROR_WANT_WRITE) 		str = "SSL_ERROR_WANT_WRITE";
	if(code == SSL_ERROR_WANT_CONNECT) 		str = "SSL_ERROR_WANT_CONNECT";
//	if(code == SSL_ERROR_WANT_ACCEPT) 		str = "SSL_ERROR_WANT_ACCEPT";
	if(code == SSL_ERROR_WANT_X509_LOOKUP)	str = "SSL_ERROR_WANT_X509_LOOKUP";
	if(code == SSL_ERROR_SYSCALL) 			str = "SSL_ERROR_SYSCALL";
	if(code == SSL_ERROR_SSL) 				str = "SSL_ERROR_SSL";

	ERR_print_errors_fp(stderr); 

	if(!str) fprintf(stderr,"error: %s: error %d\n",fx,code);
		else fprintf(stderr,"error: %s: %s\n",fx,str);
	exit(2);
}


char* g_password = NULL; /*The password code is not thread safe*/
static int password_cb(char *buf,size_t num,int rwflag,void *userdata) {
	if(!g_password || num<strlen(g_password)+1) return 0;
	strcpy(buf,g_password);
	return strlen(g_password);
}
static void sigpipe_handle(int x) {
	fprintf(stderr,"SIGPIPE received, ignoring\n");
}



Bool ssl_init(const char* certfile,const char* keyfile,const char* password) {
	if(ctx) exit(6); // already called

	SSL_library_init();
	SSL_load_error_strings();
	ERR_load_crypto_strings();

	ctx = SSL_CTX_new(SSLv23_method());
	if(!ctx) STRERROR("SSL_CTX_new()");

	if(!SSL_CTX_use_certificate_chain_file(ctx,keyfile)) 
		STRERROR("Can't read certificate file");

	if(password) {
		g_password = strdup(password);
		SSL_CTX_set_default_passwd_cb(ctx,password_cb);
	}

	if(!SSL_CTX_use_RSAPrivateKey_file(ctx,keyfile,SSL_FILETYPE_PEM)) {
		ERR_print_errors_fp(stderr); 
		STRERROR("Can't read key file");
	}

	if(!SSL_CTX_load_verify_locations(ctx,certfile,0))
		STRERROR("Can't read CA list");

	SSL_CTX_set_verify(ctx,SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT|
						   SSL_VERIFY_CLIENT_ONCE,NULL);

#if(OPENSSL_VERSION_NUMBER < 0x00090600fL)
	SSL_CTX_set_verify_depth(ctx,1);
#endif

	return true;
}

static void get_auth(SSL *ssl,char *auth,int auth_size) {
	X509	*x509 = SSL_get_peer_certificate(ssl);
	if(!x509) STRERROR("No certificate was presented by the peer");

	auth[0] = '\0';
	X509_NAME_get_text_by_NID(X509_get_subject_name(x509),
							  NID_commonName, auth, auth_size);
}


Net net_connect_ssl(const char *host,int port) {
	char 	peer[256],auth[512];
	int	 	sock,err;
	BIO		*sbio;
	SSL		*ssl;

	if(!ctx) exit(7);

	// tcp
	sock = tcp_connect(host,port);
	snprintf(peer,256,"%s:%d",host,port);

	// ssl
	ssl = SSL_new(ctx);
	sbio = BIO_new_socket(sock,BIO_NOCLOSE);
	SSL_set_bio(ssl,sbio,sbio);

	err = SSL_connect(ssl);
	if(err <= 0) SSLERROR(ssl,"SSL_connect()",err);

	err = SSL_get_verify_result(ssl);
	if(err != X509_V_OK) {
		fprintf(stderr,"SSL_get_verify_result() %d returned\n",err);
		exit(2);
	}

	get_auth(ssl,auth,512);

	return create_net(NET_SYS_SSL,sock,peer,auth,(void*)ssl);
}

Net net_server_ssl(int port) {
	char	peer[10];
	void	*sin;
	int		sock;

	sock = tcp_listen(port,&sin);
	snprintf(peer,10,":%d",port);	// todo: return valid auth
	return create_net(NET_SYS_SSL,sock,peer,(char*)NULL,(void*)sin);
}

// returns NULL on error
Net ssl_accept(Net net) {
	char peer[256],auth[512];
	int	 sock,err;
	BIO	 *sbio;
	SSL	 *ssl;

	sock = tcp_accept(net->sock,net->data,peer,256);
	if(!sock) return NULL;
	DEBUG("connection from %s\n",peer);

	sbio=BIO_new_socket(sock,BIO_NOCLOSE);
	ssl = SSL_new(ctx);
	SSL_set_bio(ssl,sbio,sbio);

	err = SSL_accept(ssl);
	if(err <= 0) SSLERROR(ssl,"SSL_accept()",err);

	get_auth(ssl,auth,512);

	return create_net(NET_SYS_SSL,sock,peer,auth,(void*)ssl);
}



// ==========================================================


// read as many data as you can
// returns -1 on EOF (connection closed), 0 on idle, 1+ as datalength
// -2+ on serious error
int ssl_read(Net net,void *buffer,int len) {
	SSL	 *ssl;

	ssl = (SSL*)net->data;
	len = SSL_read(ssl,buffer,len);
	if(len > 0) return len;

	switch(SSL_get_error(ssl,len)) {
	case SSL_ERROR_WANT_READ: return 0;
	case SSL_ERROR_SYSCALL:   return -1;
	default: SSLERROR(ssl,"SSL_read()",len);
	}

	return -2;
}

static sum = 0;

// SHOULD return 0 on error!
int ssl_write(Net net,void *chunk,int len) {
	SSL	 *ssl = (SSL*)net->data;
	int  err;

	err = SSL_write(ssl,chunk,len);

	if(err <= 0) SSLERROR(ssl,"SSL_write()",err);
	if(err != len) STRERROR("len != SSL_write(len)");

	return err;
}

// set non-blocking IO
void ssl_block(Net net,Bool block) {
	tcp_block(net->sock,block);
}

void ssl_close(Net net) {
//	SSL *ssl = (SSL*)net->data;
//	int ret;

/*	If we called SSL_shutdown() first then we always get return value of '0'. 
	In this case, try again, but first send a TCP FIN to trigger the other 
	side's close_notify */

//	ssl_block(net,1);
//	ret = SSL_shutdown(ssl);
/*	if(!ret) {
		shutdown(net->sock,1);
		SSL_shutdown(ssl);
	}*/
//	SSL_free(ssl);

	shutdown(net->sock,1);
#ifdef WIN32
	closesocket(net->sock);
#else
	close(net->sock);
#endif
}

