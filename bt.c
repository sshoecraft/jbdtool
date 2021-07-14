
#if defined(BLUETOOTH) && !defined(__WIN32) && !defined(__WIN64)
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.

Bluetooth transport

This was tested against the MLT-BT05 TTL-to-Bluetooth module (HM-10 compat) on the Pi3B+

*/

#undef DEBUG_MEM

#include "mybmm.h"
#include "gattlib.h"

struct bt_session {
	gatt_connection_t *c;
	uuid_t uuid;
	char target[32];
	char topts[32];
	char data[4096];
	int not;
	int len;
	int cbcnt;
	int have_char;
};
typedef struct bt_session bt_session_t;

static int bt_init(mybmm_config_t *conf) {
	return 0;
}

static void *bt_new(mybmm_config_t *conf, ...) {
	bt_session_t *s;
	va_list ap;
	char *target,*topts;

	va_start(ap,conf);
	target = va_arg(ap,char *);
	topts = va_arg(ap,char *);
	va_end(ap);

	dprintf(5,"target: %s, topts: %s\n", target, topts);
	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("bt_new: malloc");
		return 0;
	}
	strncat(s->target,(char *)target,sizeof(s->target)-1);
	if (topts) {
		if (strlen((char *)topts) && strncmp((char *)topts,"0x",2) != 0 && !strchr((char *)topts,'-'))
			sprintf(s->topts,"0x%s",(char *)topts);
		else
			strncat(s->topts,(char *)topts,sizeof(s->topts)-1);
	}
	dprintf(5,"target: %s, topts: %s\n", s->target, s->topts);
	return s;
}

static void notification_cb(const uuid_t* uuid, const uint8_t* data, size_t data_length, void* user_data) {
	bt_session_t *s = (bt_session_t *) user_data;

	/* Really should check for overrun here */
	dprintf(7,"s->len: %d, data: %p, data_length: %d\n", s->len, data, (int)data_length);
	if (s->len + data_length > sizeof(s->data)) data_length = sizeof(s->data) - s->len;
	memcpy(&s->data[s->len],data,data_length);
	s->len+=data_length;
	dprintf(7,"s->len: %d\n", s->len);
	s->cbcnt++;
	dprintf(7,"s->cbcnt: %d\n", s->cbcnt);
}

static int bt_open(void *handle) {
	bt_session_t *s = handle;
	gattlib_characteristic_t *cp;
	int count,ret,i,found;
	char uuid_str[40];

	if (s->c) return 0;

	dprintf(1,"connecting to: %s\n", s->target);
	s->c = gattlib_connect(NULL, s->target, GATTLIB_CONNECTION_OPTIONS_LEGACY_DEFAULT);
	if (!s->c) {
		log_write(LOG_ERROR,"Fail to connect to the bluetooth device.\n");
		return 1;
	}
	dprintf(1,"s->c: %p\n", s->c);

	ret = gattlib_discover_char(s->c, &cp, &count);
	if (ret) {
		log_write(LOG_ERROR,"Failed to discover characteristics.\n");
		return 1;
	}

	if (!s->have_char) {
		found = 0;
		dprintf(1,"topts: %s\n", s->topts);
		if (strlen(s->topts)) {
			for(i=0; i < count; i++) {
				gattlib_uuid_to_string(&cp[i].uuid, uuid_str, sizeof(uuid_str));
				dprintf(1,"uuid: %s, value_handle: %04x\n", uuid_str, cp[i].value_handle);
				if (strcmp(uuid_str,s->topts) == 0) {
					memcpy(&s->uuid,&cp[i].uuid,sizeof(s->uuid));
					found = 1;
					break;
				}
			}
		} else if (count > 0) {
			gattlib_uuid_to_string(&cp[0].uuid, uuid_str, sizeof(uuid_str));
			dprintf(1,"uuid: %s, value_handle: 0x%04x\n", uuid_str, cp[0].value_handle);
			memcpy(&s->uuid,&cp[0].uuid,sizeof(s->uuid));
			dprintf(1,"using characteristic: %s\n", uuid_str);
			found = 1;
		}
		free(cp);
		if (!found) {
			/* just force use of ffe1 */
			dprintf(1,"not found, using 0xffe1\n");
			strcpy(s->topts,"0xffe1");
			gattlib_string_to_uuid(s->topts, strlen(s->topts)+1, &s->uuid);
		}
		s->have_char = 1;
	}

	gattlib_register_notification(s->c, notification_cb, s);
	if (gattlib_notification_start(s->c, &s->uuid)) {
		dprintf(1,"error: failed to start bluetooth notification.\n");
		gattlib_disconnect(s->c);
		return 1;
	} else {
		s->not = 1;
	}

	dprintf(1,"s->c: %p\n", s->c);
	return 0;
}

static int bt_read(void *handle,...) {
	bt_session_t *s = handle;
	uint8_t *buf;
	int buflen, len, retries;
	va_list ap;

	if (!s->c) return -1;

	va_start(ap,handle);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);

	dprintf(1,"buf: %p, buflen: %d\n", buf, buflen);

	retries=3;
	while(1) {
		dprintf(1,"s->len: %d\n", s->len);
		if (!s->len) {
			if (!--retries) return 0;
			sleep(1);
			continue;
		}
		len = (s->len > buflen ? buflen : s->len);
		dprintf(1,"len: %d\n", len);
		memcpy(buf,s->data,len);
		s->len = 0;
		break;
	}

	return len;
}

static int bt_write(void *handle,...) {
	bt_session_t *s = handle;
	uint8_t *buf;
	int buflen;
	va_list ap;

	if (!s->c) return -1;

	va_start(ap,handle);
	buf = va_arg(ap, uint8_t *);
	buflen = va_arg(ap, int);
	va_end(ap);

	dprintf(1,"buf: %p, buflen: %d\n", buf, buflen);

	s->len = 0;
	s->cbcnt = 0;
	dprintf(1,"s->c: %p\n", s->c);
	if (gattlib_write_char_by_uuid(s->c, &s->uuid, buf, buflen)) return -1;
	bindump("bt write",buf,buflen);

	return buflen;
}


static int bt_close(void *handle) {
	bt_session_t *s = handle;

	dprintf(1,"s->c: %p\n",s->c);
	if (s->c) {
		if (s->not) {
			dprintf(1,"stopping notifications\n");
			gattlib_notification_stop(s->c, &s->uuid);
			s->not = 0;
		}
		dprintf(1,"disconnecting...\n");
		gattlib_disconnect(s->c);
		s->c = 0;
	}
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
#endif
