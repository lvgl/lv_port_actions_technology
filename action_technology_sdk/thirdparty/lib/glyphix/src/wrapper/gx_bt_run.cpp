
#include "gx_application.h"
#include "gx_logger.h"
#include "gx_threadpool.h"
#include "mas.h"
#include <zephyr.h>

using namespace gx;
static struct k_thread mas_rpc_thread;
static K_KERNEL_STACK_DEFINE(mas_stack, 4 * 1024);

static struct k_thread thpool_thread;
static K_KERNEL_STACK_DEFINE(thpool_stack, 6 * 1024);

struct MasThreadPool : ThreadPool {
    static void __entry(void *p1, void *p2, void *p3) {
        MasThreadPool *mas_thread_pool = (MasThreadPool *)p1;
        while (mas_thread_pool->loop.waitEvents(), mas_thread_pool->loop.processEvents()) {}
    }

    MasThreadPool() {
        for (int i = 0; i < 1; i++) {
            //rt_thread_t self_thread = rt_thread_create("thpool", __entry, this, 1024 * 2, 12, 20);
           // rt_thread_startup(self_thread);
			k_thread_create(&thpool_thread, thpool_stack, K_THREAD_STACK_SIZEOF(thpool_stack), __entry, this, NULL, NULL, 12, 0, K_NO_WAIT);
			k_thread_name_set(&thpool_thread, "thpool");
			k_thread_start(&thpool_thread);
        }
    }
    virtual bool enqueue(TaskEvent *task) {
        loop.postEvent(task);
        return true;
    }

    EventLoop loop;
};

static void mas_rpc_run_test(void *p1, void *p2, void *p3) { mas_rpc_run(); }

static void init_mas() {
    mas_init();

    MasThreadPool *masThreadPool = new MasThreadPool();
    mas_proto_set_req_pool(masThreadPool);

    k_thread_create(&mas_rpc_thread, mas_stack, K_THREAD_STACK_SIZEOF(mas_stack), mas_rpc_run_test, NULL, NULL, NULL, 0, 0, K_NO_WAIT);
    k_thread_name_set(&mas_rpc_thread, "mas_rpc_run");
    k_thread_start(&mas_rpc_thread);
}

extern "C" int mas_start() {
    LogInfo() << "glyphix_startglyphix_start";

	int bluetooth_register(void);
    bluetooth_register();
    
    init_mas();

    return 0;
}
