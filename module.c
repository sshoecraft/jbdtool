
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#define _GNU_SOURCE
#include <dlfcn.h>
#include "mybmm.h"

#if 0
extern mybmm_module_t si_module;
extern mybmm_module_t jbd_module;
extern mybmm_module_t ip_module;
extern mybmm_module_t can_module;
extern mybmm_module_t canproxy_module;
extern mybmm_module_t preh_module;
#endif

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
	char temp[128];
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

#if 0
	sprintf(temp,"modules/%s.so", p);
	dprintf(3,"temp: %s\n", temp);
	h = dlopen(temp,RTLD_NOW);

	/* If a debug sym is present in the mod, set it to same as ours */
	mod_debug = dlsym(conf->dlsym_handle, "debug");
	if (mod_debug) *mod_debug = debug;
#endif

#if 1
	/* Get the module symbol */
	sprintf(temp,"%s_module",name);
//	mp = dlsym(conf->dlsym_handle, temp);
	mp = dlsym(RTLD_DEFAULT, temp);
	dprintf(3,"module: %p\n", mp);
	if (!mp) {
		printf("error: cannot find symbol: %s_module: %s\n",name,dlerror());
		return 0;
	}
#else
	/* Static config */
	if (strcmp(name,"si")==0) {
		mp = &si_module;
	} else if (strcmp(name,"jbd")==0) {
		mp = &jbd_module;
	} else if (strcmp(name,"canproxy")==0) {
		mp = &canproxy_module;
	} else if (strcmp(name,"preh")==0) {
		mp = &preh_module;
	} else if (strcmp(name,"ip")==0) {
		mp = &preh_module;
	} else {
		return 0;
	}
#endif

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
