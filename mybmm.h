
#ifndef __MYBMS_H__
#define __MYBMS_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>

#include "utils.h"
#include "list.h"
#include "worker.h"
#include "debug.h"

//typedef unsigned char uint8_t;

#define MYBMM_MAX_INVERTERS	32
#define MYBMM_MAX_PACKS		32
#define MYBMM_PACK_NAME_LEN	32
#define MYBMM_PACK_TYPE_LEN	8
#define MYBMM_MODULE_NAME_LEN	32
//#define MYBMM_INSTANCE_NAME_LEN	32
//#define MYBMM_TRANSPORT_NAME_LEN 8

#include "config.h"
#include "module.h"
#include "pack.h"

#define MYBMM_TARGET_LEN 32

/* States */
#define MYBMM_SHUTDOWN		0x0001		/* Emergency Shutdown */
#define MYBMM_GEN_ACTIVE	0x0002		/* Generator Active */
#define MYBMM_CHARGING		0x0004		/* Charging */
#define MYBMM_CRIT_CELLVOLT	0x0008		/* A cell voltage is critical */
#define MYBMM_FORCE_SOC		0x0010		/* Force State of Charge */
#define MYBMM_CONFIG_DIRTY	0x0020		/* Config has been updated */
#define MYBMM_GRID_ENABLE	0x0040		/* Grid is enabled */
#define MYBMM_GEN_ENABLE	0x0080		/* Generator is enabled */
#define MYBMM_GRID_ACTIVE	0x0100		/* Grid is active */

/* State Macros */
#define mybmm_set_state(c,v)	(c->state |= (v))
#define mybmm_clear_state(c,v)	(c->state &= (~v))
#define mybmm_check_state(c,v)	((c->state & v) != 0)
#define mybmm_set_cap(c,v)	(c->capabilities |= (v))
#define mybmm_clear_cap(c,v)	(c->capabilities &= (~v))
#define mybmm_check_cap(c,v)	((c->capabilities & v) != 0)
#define mybmm_is_shutdown(c)	mybmm_check_state(c,MYBMM_SHUTDOWN)
#define mybmm_is_gen(c) 	mybmm_check_state(c,MYBMM_GEN_ACTIVE)
#define mybmm_is_charging(c)	mybmm_check_state(c,MYBMM_CHARGING)
#define mybmm_is_critvolt(c)	mybmm_check_state(c,MYBMM_CRIT_CELLVOLT)

#define mybmm_force_soc(c,v)	{ c->soc = v; mybmm_set_state(c,MYBMM_FORCE_SOC); }

#endif
