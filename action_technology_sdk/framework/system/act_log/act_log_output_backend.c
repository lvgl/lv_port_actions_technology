#include "act_log_inner.h"

#ifdef CONFIG_ACTLOG_OUTPUT_USER
static actlog_backend_callback_t user_backend_cb;
static void *user_backend_data;
#endif

int act_log_backend_init(void)
{
    int ret_val = 0;

    uint32_t output_mode = 0;

    if (IS_ENABLED(CONFIG_ACTLOG_OUTPUT_FLOW)) {
        ret_val |= act_log_output_flow_init();
        output_mode |= ACTLOG_OUTPUT_MODE_FLOW;
    }

    if (IS_ENABLED(CONFIG_ACTLOG_OUTPUT_BINARY)) {
        ret_val |= act_log_output_binary_init();
        output_mode |= ACTLOG_OUTPUT_MODE_BINARY;
    }

    if (IS_ENABLED(CONFIG_ACTLOG_OUTPUT_FILE)) {
        ret_val |= act_log_output_textfile_init();
        output_mode |= ACTLOG_OUTPUT_MODE_TEXT;
    }

    actlog_ctrl.output_mode = output_mode;

    return ret_val;
}

int act_log_backend_register(uint8_t mode, actlog_backend_callback_t callback, void *user_data)
{
    if (mode != ACTLOG_OUTPUT_MODE_USER) {
        return -EINVAL;
    }

#ifdef CONFIG_ACTLOG_OUTPUT_USER
    user_backend_data = user_data;
    user_backend_cb = callback;
    return 0;
#else
    return -ENOSYS;
#endif
}

int act_log_backend_unregister(uint8_t mode)
{
    if (mode != ACTLOG_OUTPUT_MODE_USER) {
        return -EINVAL;
    }

#ifdef CONFIG_ACTLOG_OUTPUT_USER
    user_backend_cb = NULL;
    user_backend_data = NULL;
#endif
    return 0;
}

size_t actlog_strnlen(const char *s, size_t maxlen)
{
	size_t n = 0;

	while (*s != '\0' && n < maxlen) {
		s++;
		n++;
	}

	return n;
}

int act_log_backend_output(log_message_t *log_msg, uint8_t *data, uint32_t len)
{
    int ret_val = 0;

    uint32_t output_mode = actlog_ctrl.output_mode;

    if (output_mode & ACTLOG_OUTPUT_MODE_FLOW) {
        if (IS_ENABLED(CONFIG_ACTLOG_OUTPUT_FLOW)) {
            //Actions serial tools cannot output a string with a '\0', and must truncate it with strlen
            ret_val |= act_log_output_flow_write(log_msg, data, actlog_strnlen(data, len));
        } else {
            return -EIO;
        }
    }

    if (output_mode & ACTLOG_OUTPUT_MODE_BINARY) {
        if (IS_ENABLED(CONFIG_ACTLOG_OUTPUT_BINARY)) {
            ret_val |= act_log_output_binary_write(log_msg, data, len);
        } else {
            return -EIO;
        }
    }

    if (output_mode & ACTLOG_OUTPUT_MODE_TEXT) {
        if (IS_ENABLED(CONFIG_ACTLOG_OUTPUT_FILE)) {
            ret_val |= act_log_output_textfile_write(log_msg, data, actlog_strnlen(data, len));
        } else {
            return -EIO;
        }
    }

    if (output_mode & ACTLOG_OUTPUT_MODE_USER) {
#ifdef CONFIG_ACTLOG_OUTPUT_USER
        if (user_backend_cb) {
            user_backend_cb(data, len, user_backend_data);
        }
#endif
    }

    return ret_val;
}

int act_log_backend_get_used_size(int file_id)
{
    uint32_t output_mode = actlog_ctrl.output_mode;

    if (output_mode & ACTLOG_OUTPUT_MODE_BINARY) {
        if (IS_ENABLED(CONFIG_ACTLOG_OUTPUT_BINARY)) {
            return act_log_output_binary_get_used_size(file_id);
        } else {
            return -EIO;
        }
    }

    if (output_mode & ACTLOG_OUTPUT_MODE_TEXT) {
        if (IS_ENABLED(CONFIG_ACTLOG_OUTPUT_FILE)) {
            return act_log_output_textfile_get_used_size(file_id);
        } else {
            return -EIO;
        }
    }

    return 0;
}

int act_log_backend_flush(void)
{
    uint32_t output_mode = actlog_ctrl.output_mode;

    if (output_mode & ACTLOG_OUTPUT_MODE_BINARY) {
        if (IS_ENABLED(CONFIG_ACTLOG_OUTPUT_BINARY)) {
            return act_log_output_binary_flush();
        } else {
            return -EIO;
        }
    }

    if (output_mode & ACTLOG_OUTPUT_MODE_TEXT) {
        if (IS_ENABLED(CONFIG_ACTLOG_OUTPUT_FILE)) {
            return act_log_output_textfile_flush();
        } else {
            return -EIO;
        }
    }

    return 0;
}

int act_log_backend_traverse(int file_id, int (*traverse_cb)(uint8_t *data, uint32_t max_len), uint8_t *buf,
        uint32_t len)
{
    uint32_t output_mode = actlog_ctrl.output_mode;

    if (output_mode & ACTLOG_OUTPUT_MODE_BINARY) {
        if (IS_ENABLED(CONFIG_ACTLOG_OUTPUT_BINARY)) {
            return act_log_output_binary_traverse(file_id, traverse_cb, buf, len);
        } else {
            return -EIO;
        }
    }

    if (output_mode & ACTLOG_OUTPUT_MODE_TEXT) {
        if (IS_ENABLED(CONFIG_ACTLOG_OUTPUT_FILE)) {
            return act_log_output_textfile_traverse(file_id, traverse_cb, buf, len);
        } else {
            return -EIO;
        }
    }

    return 0;
}

int act_log_backend_clear(int file_id)
{
    uint32_t output_mode = actlog_ctrl.output_mode;

    if (output_mode & ACTLOG_OUTPUT_MODE_BINARY) {
        if (IS_ENABLED(CONFIG_ACTLOG_OUTPUT_BINARY)) {
            return act_log_output_binary_clear(file_id);
        } else {
            return -EIO;
        }
    }

    if (output_mode & ACTLOG_OUTPUT_MODE_TEXT) {
        if (IS_ENABLED(CONFIG_ACTLOG_OUTPUT_FILE)) {
            return act_log_output_textfile_clear(file_id);
        } else {
            return -EIO;
        }
    }

    return 0;
}

