#ifndef __ACTIONS_EXCEPTION_H
#define __ACTIONS_EXCEPTION_H

typedef void (*actions_exception_callback_t)(void);

typedef struct
{
    actions_exception_callback_t init_cb;
    actions_exception_callback_t run_cb;
} actions_exception_callback_routine_t;

#ifdef CONFIG_ACTIONS_EXCEPTION

int exception_register_callbacks(actions_exception_callback_routine_t *cb);

void exception_init(void);

void exception_run(void);

#else

int exception_register_callbacks(actions_exception_callback_routine_t *cb)
{
	return -ENOTSUP;
}

void exception_init(void)
{
	return;
}

void exception_run(void)
{
	return;
}
#endif

#endif
