
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#define _GNU_SOURCE
#ifndef __WIN32
#include <dlfcn.h>
#endif
#include "mybmm.h"

extern mybmm_module_t jbd_module;
#if !defined(__WIN32) && !defined(__WIN64)
extern mybmm_module_t bt_module;
extern mybmm_module_t can_module;
#endif
extern mybmm_module_t ip_module;
extern mybmm_module_t serial_module;

mybmm_module_t *mybmm_get_module(mybmm_config_t *conf, char *name, int type) {
	mybmm_module_t *mp;

	dprintf(3,"name: %s, type: %d\n", name, type);
	list_reset(conf->modules);
	while((mp = list_get_next(conf->modules)) != 0) {
		dprintf(3,"mp->name: %s, mp->type: %d\n", mp->name, mp->type);
		if (strcmp(mp->name,name)==0 && mp->type == type) {
			dprintf(3,"found.\n");
			return mp;
		}
	}
	dprintf(3,"NOT found.\n");
	return 0;
}

mybmm_module_t *mybmm_load_module(mybmm_config_t *conf, char *name, int type) {
	mybmm_module_t *mp;

	list_reset(conf->modules);
	while((mp = list_get_next(conf->modules)) != 0) {
		dprintf(3,"mp->name: %s, mp->type: %d\n", mp->name, mp->type);
		if (strcmp(mp->name,name)==0 && mp->type == type) {
			dprintf(3,"found.\n");
			return mp;
		}
	}
	dprintf(3,"NOT found.\n");

	 if (strcmp(name,"jbd")==0) {
		mp = &jbd_module;
#if !defined(__WIN32) && !defined(__WIN64)
#if defined(BLUETOOTH)
	} else if (strcmp(name,"bt")==0) {
		mp = &bt_module;
#endif
	} else if (strcmp(name,"can")==0) {
		mp = &can_module;
#endif
	} else if (strcmp(name,"ip")==0) {
		mp = &ip_module;
	} else if (strcmp(name,"serial")==0) {
		mp = &serial_module;
	} else {
		return 0;
	}

	/* Call the init function */
	dprintf(3,"init: %p\n", mp->init);
	if (mp->init && mp->init(conf)) {
		printf("%s init function returned error, aborting.\n",mp->name);
		return 0;
	}

	/* Add this module to our list */
	dprintf(3,"adding module: %s\n", mp->name);
	list_add(conf->modules,mp,0);

	return mp;
}
