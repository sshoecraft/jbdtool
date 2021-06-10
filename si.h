
#ifndef __MYBMM_SI_H
#define __MYBMM_SI_H

struct si_session {
	mybmm_config_t *conf;
	mybmm_inverter_t *inv;
	pthread_t th;
	uint32_t bitmap;
	uint8_t messages[16][8];
	mybmm_module_t *tp;
	void *tp_handle;
	int stop;
	int open;
	int (*get_data)(struct si_session *, int id, uint8_t *data, int len);
};
typedef struct si_session si_session_t;

#endif /* __MYBMM_SI_H */
