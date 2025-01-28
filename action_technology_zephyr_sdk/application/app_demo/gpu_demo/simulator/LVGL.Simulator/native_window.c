/**
 * @file native_window.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_drivers/win32drv/win32drv.h"
#include "resource.h"

#include "native_window.h"

/*********************
 *      DEFINES
 *********************/

#define VSYNC_TIMER_ID 1
#define CLOCK_TIMER_ID 2

#define VSYNC_TIMER_PERIOD 16
#define CLOCK_TIMER_PERIOD 100

/**********************
 *      TYPEDEFS
 **********************/
typedef struct _native_window {
    native_window_callback_t callback[NWIN_NUM_CBS];
    void * user_data[NWIN_NUM_CBS];
} native_window_t;

/**********************
*  GLOBAL PROTOTYPES
**********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void CALLBACK _vsync_period_callback(HWND hWnd, UINT nMsg, UINT nTimerid, DWORD dwTime);
static void CALLBACK _clock_period_callback(HWND hWnd, UINT nMsg, UINT nTimerid, DWORD dwTime);

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
static native_window_t nwin_data;

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

int native_window_init(void)
{
    SetTimer(NULL, VSYNC_TIMER_ID, VSYNC_TIMER_PERIOD, _vsync_period_callback);
    SetTimer(NULL, CLOCK_TIMER_ID, CLOCK_TIMER_PERIOD, _clock_period_callback);

    return !lv_win32_init(
        GetModuleHandleW(NULL), SW_SHOW,
        CONFIG_PANEL_HOR_RES, CONFIG_PANEL_VER_RES,
        LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDI_LVGL)));
}

int native_window_handle_message_loop(void)
{
    int ret = lv_win32_handle_message_loop();

    KillTimer(NULL, VSYNC_TIMER_ID);
    KillTimer(NULL, CLOCK_TIMER_ID);

    return ret;
}

bool native_window_is_closed(void)
{
    return lv_win32_quit_signal;
}

void * native_window_get_framebuffer(void)
{
    return lv_win32_get_frame_buffer();
}

void native_window_flush_framebuffer(
    int16_t x, int16_t y, int16_t w, int16_t h)
{
    lv_win32_flush_frame_buffer(x, y, w, h);
}

bool native_window_get_pointer_state(input_dev_data_t* data)
{
    lv_coord_t x, y;
    bool pressed = false;

    lv_win32_get_pointer_state(&pressed, &x, &y);

    if (x < 0) {
        x = 0;
    } else if (x > CONFIG_PANEL_HOR_RES - 1) {
        x = CONFIG_PANEL_HOR_RES - 1;
    }

    if (y < 0) {
        y = 0;
    } else if (y > CONFIG_PANEL_VER_RES - 1) {
        y = CONFIG_PANEL_VER_RES - 1;
    }

    data->point.x = x;
    data->point.y = y;
    data->state = pressed ? INPUT_DEV_STATE_PR : INPUT_DEV_STATE_REL;
    data->gesture = 0;

    return true;
}

bool native_window_get_keypad_state(input_dev_data_t* data)
{
    bool pressed = false;
    uint32_t key_val = 0;

    lv_win32_get_keypad_state(&pressed, &key_val);

    data->state = pressed ? INPUT_DEV_STATE_PR : INPUT_DEV_STATE_REL;

    /* TODO: translate Windows key value VK_x */
    switch (key_val) {
    case VK_HOME:
        data->key = KEY_POWER;
        break;
    //case VK_HOME:
    case VK_END:
    default:
        data->key = 0;
        break;
    }

    return true;
}

int native_window_register_callback(unsigned int type, native_window_callback_t cb, void* user_data)
{
    if (type >= NWIN_NUM_CBS)
        return -EINVAL;

    if (nwin_data.callback[type])
        return -EBUSY;

    nwin_data.user_data[type] = user_data;
    nwin_data.callback[type] = cb;

    return 0;
}

int native_window_unregister_callback(unsigned int type)
{
    if (type >= NWIN_NUM_CBS)
        return -EINVAL;

    if (nwin_data.callback[type]) {
        nwin_data.callback[type] = NULL;
        nwin_data.user_data[type] = NULL;
    }

    return 0;
}

void native_window_get_local_time(struct rtc_time* tm)
{
    SYSTEMTIME lt;

    GetLocalTime(&lt);

    tm->tm_year = lt.wYear;
    tm->tm_mon = lt.wMonth;
    tm->tm_mday = lt.wDay;
    tm->tm_wday = lt.wDayOfWeek;
    tm->tm_hour = lt.wHour;
    tm->tm_min = lt.wMinute;
    tm->tm_sec = lt.wSecond;
    tm->tm_ms = lt.wMilliseconds;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static void CALLBACK _vsync_period_callback(HWND hWnd, UINT nMsg, UINT nTimerid, DWORD dwTime)
{
    if (nwin_data.callback[NWIN_CB_VSYNC]) {
        nwin_data.callback[NWIN_CB_VSYNC](nwin_data.user_data[NWIN_CB_VSYNC]);
    }
}

static void CALLBACK _clock_period_callback(HWND hWnd, UINT nMsg, UINT nTimerid, DWORD dwTime)
{
    if (nwin_data.callback[NWIN_CB_CLOCK]) {
        nwin_data.callback[NWIN_CB_CLOCK](nwin_data.user_data[NWIN_CB_VSYNC]);
    }
}
