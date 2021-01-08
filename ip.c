
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include "mybmm.h"

#define DEFAULT_PORT 23

struct ip_session {
	int fd;
	char address[MYBMM_TARGET_LEN+1];
	int port;
};
typedef struct ip_session ip_session_t;

static int ip_init(mybmm_config_t *conf) {
	return 0;
}

static void *ip_new(mybmm_config_t *conf, ...) {
	ip_session_t *s;
	va_list ap;
	char *target;

	va_start(ap,conf);
	target = va_arg(ap,char *);
	va_end(ap);
	dprintf(5,"target: %s\n", target);

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("ip_new: malloc");
		return 0;
	}
	s->address[0] = 0;
	strncat(s->address,strele(0,":",target),MYBMM_TARGET_LEN);
	s->port = atoi(strele(1,":",target));
	if (!s->port) s->port = DEFAULT_PORT;
	dprintf(5,"address: %s, port: %d\n", s->address, s->port);
	return s;
}

static int ip_open(void *handle) {
	ip_session_t *s = handle;
	struct sockaddr_in addr;
	socklen_t sin_size;
//	struct hostent *hp;
	struct timeval tv;
//	int flags;

	s->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s->fd < 0) {
		perror("ip_open: socket");
		return 1;
	}

	/* Try to resolve the address */
//	hp = gethostbyname(host);

	memset(&addr,0,sizeof(addr));
	sin_size = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(s->address);
	addr.sin_port = htons(s->port);
	if (connect(s->fd,(struct sockaddr *)&addr,sin_size) < 0) {
		if (debug > 2) perror("ip_open: connect");
		goto ip_open_error;
	}

//	dprintf(1,"setting timeout\n");
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(s->fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) < 0) {
		perror("socket_open: setsockopt SO_RCVTIMEO");
		goto ip_open_error;
	}

	return 0;
ip_open_error:
	close(s->fd);
	s->fd = -1;
	return 1;
}

static int ip_read(void *handle, ...) {
	ip_session_t *s = handle;
	uint8_t *buf, ch;
	int buflen, bytes, bidx;
	va_list ap;

	va_start(ap,handle);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);

	dprintf(5,"buf: %p, buflen: %d\n", buf, buflen);

	bidx=0;
	dprintf(5,"reading...\n");
	while(1) {
		bytes = read(s->fd, &ch, 1);
		dprintf(6,"bytes: %d\n", bytes);
		if (bytes < 0) {
			if (errno != EAGAIN) bidx = -1;
			break;
		} else if (bytes == 0) {
			break;
		} else if (bytes > 0) {
//			dprintf(6,"ch: %02x\n", ch);
			buf[bidx++] = ch;
			if (bidx >= buflen) break;
		}
//		usleep(100);
	}
//	bindump("ip",buf,bidx);

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

	dprintf(5,"buf: %p, buflen: %d\n", buf, buflen);

	if (debug >= 3) bindump("TO BMS",buf,buflen);
	bytes = write(s->fd,buf,buflen);
	dprintf(5,"bytes: %d\n", bytes);
	usleep ((buflen + 25) * 100);
	return bytes;
}


static int ip_close(void *handle) {
	ip_session_t *s = handle;

	close(s->fd);
	return 0;
}

EXPORT_API mybmm_module_t ip_module = {
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
