/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.

Bluetooth transport

This was tested against the MLT-BT05 TTL-to-Bluetooth module (HM-10 compat) on the Pi3B+

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
//#include <errno.h>
//#include <fcntl.h>
//#include <termios.h>
//#include <sys/ioctl.h>
#include "gattlib.h"
#include "mybmm.h"

struct bt_session {
	gatt_connection_t *c;
	uuid_t uuid;
	char target[32];
	char data[2048];
	int len;
	int cbcnt;
};
typedef struct bt_session bt_session_t;

static void notification_cb(const uuid_t* uuid, const uint8_t* data, size_t data_length, void* user_data) {
	bt_session_t *bt = (bt_session_t *) user_data;

	/* Really should check for overrun here */
	dprintf(1,"data: %p, data_length: %d\n", data, data_length);
	memcpy(&bt->data[bt->len],data,data_length);
	bt->len+=data_length;
	dprintf(1,"bt->len: %d\n", bt->len);
	bt->cbcnt++;
	dprintf(1,"bt->cbcnt: %d\n", bt->cbcnt);
}

static int open_bt(bt_session_t *bt) {
	uint16_t on = 0x0001;

#define UUID "0xffe1"
	if (gattlib_string_to_uuid(UUID, strlen(UUID)+1, &bt->uuid) < 0) {
		fprintf(stderr, "Fail to convert string to UUID\n");
		return 1;
	}

	dprintf(1,"connecting to: %s\n", bt->target);
	bt->c = gattlib_connect(NULL, bt->target, GATTLIB_CONNECTION_OPTIONS_LEGACY_DEFAULT);
	if (!bt->c) {
		fprintf(stderr, "Fail to connect to the bluetooth device.\n");
		return 1;
	}

	/* yes, its hardcoded. deal. */
	dprintf(1,"reg not\n");
	gattlib_write_char_by_handle(bt->c, 0x0026, &on, sizeof(on));
	gattlib_register_notification(bt->c, notification_cb, bt);
	if (gattlib_notification_start(bt->c, &bt->uuid)) {
		fprintf(stderr, "Fail to start notification.\n");
		gattlib_disconnect(bt->c);
		return 1;
	}

	return 0;
}

#if 0
static int get_bt(bt_session_t *bt, unsigned char *cmd, int cmd_len) {
	int retries;

	bt->len = 0;
	bt->cbcnt = 0;
	retries=3;
	do {
		dprintf(1,"retries: %d\n", retries);
		if (gattlib_write_char_by_uuid(bt->c, &bt->uuid, cmd, cmd_len)) return 1;
		sleep(1);
	} while(retries-- && bt->cbcnt == 0);
	if (retries < 1) return 1;
//	bindump("get_bt",bt->data,bt->len);
	return 0;
}
#endif

static int bt_init(mybmm_config_t *conf) {
	return 0;
}

static void *bt_new(mybmm_config_t *conf, ...) {
	bt_session_t *s;
	va_list ap;
	char *target;

	va_start(ap,conf);
	target = va_arg(ap,char *);
	va_end(ap);
	dprintf(5,"target: %s\n", target);

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("bt_new: malloc");
		return 0;
	}
	strcpy(s->target,target);
	return s;
}

static int bt_open(void *handle) {
	bt_session_t *bt = handle;

	return (open_bt(bt));
}

static int bt_read(void *handle,...) {
	bt_session_t *bt = handle;
	uint8_t *buf;
	int buflen, len, retries;
	va_list ap;

	if (!bt->c) bt_open(handle);

	va_start(ap,handle);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);

	retries=3;
	while(1) {
		dprintf(1,"bt->len: %d\n", bt->len);
		if (!bt->len) {
			if (!--retries) return 0;
			sleep(1);
			continue;
		}
		len = (bt->len > buflen ? buflen : bt->len);
		dprintf(1,"len: %d\n", len);
		memcpy(buf,bt->data,len);
		break;
	}

#if 0
	dprintf(1,"cmd: %p, cmdlen: %d, buf: %p, buflen: %d\n", cmd, cmdlen, buf, buflen);
	if (get_bt(bt, cmd, cmdlen)) return -1;
	dprintf(1,"buflen: %d, len: %d\n", buflen,len);
#endif
	return len;
}

static int bt_write(void *handle,...) {
	bt_session_t *bt = handle;
	uint8_t *buf;
	int buflen;
	va_list ap;

	if (!bt->c) bt_open(handle);

	va_start(ap,handle);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);

	dprintf(1,"buf: %p, buflen: %d\n", buf, buflen);

	bt->len = 0;
	bt->cbcnt = 0;
	if (gattlib_write_char_by_uuid(bt->c, &bt->uuid, buf, buflen)) return -1;

#if 0
	retries=3;
	do {
		dprintf(1,"retries: %d\n", retries);
		sleep(1);
	} while(retries-- && bt->cbcnt == 0);
	if (retries < 1) return 1;
//	bindump("get_bt",bt->data,bt->len);
	return len;
#endif
	return buflen;
}


static int bt_close(void *handle) {
	bt_session_t *bt = handle;

//	gattlib_notification_stop(bt->c, &bt->uuid);
        gattlib_disconnect(bt->c);
	return 0;
}

mybmm_module_t bt_module = {
	MYBMM_MODTYPE_TRANSPORT,
	"bt",
	0,
	bt_init,
	bt_new,
	bt_open,
	bt_read,
	bt_write,
	bt_close
};
