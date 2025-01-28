#include "gx_cdef.h"
#include "mas_rpc.h"
#include "mas_utils.h"
#include "mas_link.h"
#include "mas_link_char.h"

typedef int (*mas_module_fn)(void);

/* MAS module object */
typedef struct mas_module {
    mas_module_fn init;
    mas_module_fn deinit;
} mas_module_t;

#define MAS_MODULE_EXPORT(name, init, deinit, level)                                               \
    const mas_module_t mas_module_obj_##name = {                                                   \
        (mas_module_fn)init,                                                                       \
        (mas_module_fn)deinit,                                                                     \
    };                                                                                             \
    MOBILE_ABILITY_VAR_EXPORT(mas_module_obj_##name)

typedef enum mas_status_type {
    MAS_STATUS_SERVER_LINK_UP,
    /* link up */
    MAS_STATUS_SERVER_LINK_DOWN,
    /* link down */
} mas_status_t;

enum mas_network_status_type { MAS_NETWORK_STATUS_LINK_UP, MAS_NETWORK_STATUS_LINK_DOWN };

struct mas_callback;
typedef struct mas_callback mas_status_cb;

typedef void (*mas_status_callback_fn)(mas_status_cb *cb, mas_status_t type);

struct mas_callback {
    gx_list_t node;

    union mas_callback_function {
        mas_status_callback_fn status_cb;
    } type;
};

/* low level API */
void mas_low_level_post_status_changed_event(mas_status_t type);
int mas_link_char_register(struct mas_link_char *link_char, bool using_crc,
                           const struct mas_link_char_port_ops *ops);

GX_INLINE int mas_link_bluetooth_register(struct mas_link_char *link_char,
                                          const struct mas_link_char_port_ops *ops) {
    return mas_link_char_register(link_char, true, ops);
}

GX_INLINE int mas_link_socket_register(struct mas_link_char *link_char,
                                       const struct mas_link_char_port_ops *ops) {
    return mas_link_char_register(link_char, false, ops);
}

void mas_link_char_upload_recv_data(struct mas_link_char *link_char, const void *buffer,
                                    size_t buf_len);