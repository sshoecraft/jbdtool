
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <sys/types.h>
#ifdef __WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include <fcntl.h>
#include <ctype.h>
#include "mybmm.h"

#define DEFAULT_PORT 23

#ifdef __WIN32
typedef SOCKET socket_t;
#define SOCKET_CLOSE(s) closesocket(s);
#else
typedef int socket_t;
#define SOCKET_CLOSE(s) close(s)
#define INVALID_SOCKET -1
#endif

struct ip_session {
	socket_t sock;
	char target[64];
	int port;
};
typedef struct ip_session ip_session_t;

static int ip_init(mybmm_config_t *conf) {
#ifdef __WIN32
	WSADATA wsaData;
        int iResult;

        dprintf(1,"initializng winsock...\n");
        /* Initialize Winsock */
        iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
        if (iResult != 0) {
                log_write(LOG_SYSERR,"WSAStartup");
                return 1;
        }
#endif
	return 0;
}

static void *ip_new(mybmm_config_t *conf, ...) {
	ip_session_t *s;
	va_list ap;
	char *target;
	char *p;

	va_start(ap,conf);
	target = va_arg(ap,char *);
	va_end(ap);

	dprintf(1,"target: %s\n", target);

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("ip_new: malloc");
		return 0;
	}
	s->sock = INVALID_SOCKET;
	p = strchr((char *)target,':');
	if (p) *p = 0;
	strncat(s->target,(char *)target,sizeof(s->target)-1);
	if (p) {
		p++;
		s->port = atoi(p);
	}
	if (!s->port) s->port = DEFAULT_PORT;
	dprintf(5,"target: %s, port: %d\n", s->target, s->port);
	return s;
}

static int ip_open(void *handle) {
	ip_session_t *s = handle;
	struct sockaddr_in addr;
	socklen_t sin_size;
	struct hostent *he;
	char temp[64];
	uint8_t *ptr;

	dprintf(1,"s->sock: %p\n", s->sock);
	if (s->sock != INVALID_SOCKET) return 0;

	dprintf(1,"creating socket...\n");
	s->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s->sock == INVALID_SOCKET) {
		log_write(LOG_SYSERR,"socket");
		perror("socket");
		return 1;
	}

	/* Try to resolve the target */
	he = (struct hostent *) 0;
	if (!is_ip(s->target)) {
		he = gethostbyname(s->target);
		dprintf(6,"he: %p\n", he);
		if (he) {
			ptr = (unsigned char *) he->h_addr;
			sprintf(temp,"%d.%d.%d.%d",ptr[0],ptr[1],ptr[2],ptr[3]);
		}
	}
	if (!he) strcpy(temp,s->target);
	dprintf(3,"temp: %s\n",temp);

	memset(&addr,0,sizeof(addr));
	sin_size = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(temp);
	addr.sin_port = htons(s->port);
	dprintf(3,"connecting...\n");
	if (connect(s->sock,(struct sockaddr *)&addr,sin_size) < 0) {
		lprintf(LOG_SYSERR,"connect to %s", s->target);
		return 1;
	}
	return 0;
}

static int ip_read(void *handle, ...) {
	ip_session_t *s = handle;
	uint8_t *buf;
	int buflen, bytes, bidx, num;
	va_list ap;
	struct timeval tv;
	fd_set rdset;

	va_start(ap,handle);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);

	dprintf(5,"buf: %p, buflen: %d\n", buf, buflen);

	tv.tv_usec = 0;
	tv.tv_sec = 1;

	FD_ZERO(&rdset);

	if (s->sock == INVALID_SOCKET) return -1;

	dprintf(5,"reading...\n");
	bidx=0;
	while(1) {
		FD_SET(s->sock,&rdset);
		num = select(s->sock+1,&rdset,0,0,&tv);
		dprintf(5,"num: %d\n", num);
		if (!num) break;
		dprintf(5,"buf: %p, bufen: %d\n", buf, buflen);
		bytes = recv(s->sock, buf, buflen, 0);
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 0) {
			if (errno == EAGAIN) continue;
			bidx = -1;
			break;
		} else if (bytes == 0) {
			break;
		} else if (bytes > 0) {
			buf += bytes;
			bidx += bytes;
			buflen -= bytes;
			if (buflen < 1) break;
		}
	}
	if (debug >= 5) bindump("ip_read",buf,bidx);
	dprintf(5,"returning: %d\n", bidx);
	return bidx;
}

static int ip_write(void *handle, ...) {
	ip_session_t *s = handle;
	uint8_t *buf;
	int bytes,buflen;
	va_list ap;

	va_start(ap,handle);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);

	dprintf(4,"s->sock: %p\n", s->sock);

	if (s->sock == INVALID_SOCKET) return -1;
	if (debug >= 5) bindump("ip_write",buf,buflen);
	bytes = send(s->sock, buf, buflen, 0);
	dprintf(4,"bytes: %d\n", bytes);
	return bytes;
}


static int ip_close(void *handle) {
	ip_session_t *s = handle;

	if (s->sock != INVALID_SOCKET) {
		dprintf(5,"closing...\n");
		SOCKET_CLOSE(s->sock);
		s->sock = INVALID_SOCKET;
	}
	return 0;
}

mybmm_module_t ip_module = {
	MYBMM_MODTYPE_TRANSPORT,
	"ip",
	0,
	ip_init,
	ip_new,
	ip_open,
	ip_read,
	ip_write,
	ip_close
};
