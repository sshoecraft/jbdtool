
#ifndef __WORKER_H
#define __WORKER_H

typedef void (*worker_func_t)(void *);

struct worker_info;
typedef struct worker_info worker_info_t;

struct worker_pool {
	worker_info_t *workers;
	int count;
};
typedef struct worker_pool worker_pool_t;

extern worker_pool_t *worker_create_pool(int);
extern int worker_destroy_pool(worker_pool_t *,int);
extern int worker_exec(worker_pool_t *,worker_func_t func,void *arg);
extern void worker_wait(worker_pool_t *,int);
extern void worker_killbusy(worker_pool_t *);
extern void worker_kill(worker_pool_t *);
extern void worker_finish(worker_pool_t *,int);

#endif /* __WORKER_H */
