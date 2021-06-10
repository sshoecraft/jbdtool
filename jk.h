
#ifndef __JK_H
#define __JK_H

#include "module.h"
#include "pack.h"

struct jk_session {
	mybmm_pack_t *pp;		/* Our pack info */
	mybmm_module_t *tp;		/* Our transport */
	void *tp_handle;		/* Our transport handle */
};
typedef struct jk_session jk_session_t;

#endif
