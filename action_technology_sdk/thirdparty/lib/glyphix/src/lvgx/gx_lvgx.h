#pragma once

#include "lvgl/lvgl.h"
#if LV_VERSION_CHECK(9, 0, 0)
#include "lvgl/src/lvgl_private.h"
#endif
#include "msg_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LvglGlyphix lvgx_t;

/**
 * @brief Key code definitions.
 * @see lvgx_post_key_event()
 */
typedef enum lvgx_key_code {
  LVGX_KEY_POWER = 0, //!< The power key of the watch.
  LVGX_KEY_FUNC = 1   //!< The function key of the watch.
} lvgx_key_code_t;

/**
 * @brief Initializes the Glyphix framework in LVGL.
 *
 * This function should be called during system initialization to set up the
 * Glyphix system and its services. It has a relatively high stack requirement,
 * therefore it is recommended to be executed within the UI thread.
 */
void lvgx_init(void);
/**
 * @brief Deinitializes the Glyphix framework in LVGL.
 *
 * This function should be called during system deinitialization to clean up the
 * Glyphix system and its services.
 */
void lvgx_deinit(void);
/**
 * Attaches the Glyphix UI to an LVGL object, causing it to be displayed within
 * that object.
 * @param parent The parent LVGL object, typically a screen.
 * @note This is an internal function and usually should not be called manually,
 * instead, use lvgx_applet_view_enter().
 * @see lvgx_applet_view_enter()
 */
void lvgx_attach_window(lv_obj_t *parent);
/**
 * Process the Glyphix event queue. This function should be called within the
 * main event loop. It must be invoked even when the display is off or in a
 * low-power state.
 */
void lvgx_process_events(void);
/**
 * Launches a Glyphix applet.
 * @param package The package name of the Glyphix applet to start.
 * @return Returns 0 if the launch is successful.
 * @note This function should not be called directly; instead, use
 * lvgx_applet_view_enter().
 * @see lvgx_applet_view_enter()
 */
int lvgx_launch_applet(const char *package);
/**
 * Displays a list of Glyphix applet within an LVGL interface.
 * This function serves as a demonstration or debugging feature.
 * @param parent The parent LVGL object where the applet list will be shown.
 */
void lvgx_applet_list(lv_obj_t *parent);
/**
 * @brief Update applet Glyphix list display.
 * Call this function during the applet installation or uninstallation, etc.
 */
void lvgx_applet_list_update();
//! Checks whether there is an active Glyphix application.
bool lvgx_applet_actived(void);
/**
 * Posts a Glyphix entity key event to the event queue.
 * @param code The key code representing the key being pressed.
 * @param state The state of the key press: true indicates a press, false
 * indicates a release.
 */
void lvgx_post_key_event(lvgx_key_code_t code, bool state);

/**
 * Initializes the Glyphix heap. This is an internal function and should not be
 * called manually.
 */
void lvgx_heap_init(void);
/**
 * This port function is invoked at the first stage of lvgx_deinit()
 */
void lvgx_port_post_init(void);
/**
 * This port function is invoked at the final stage of lvgx_init(),
 * intended for performing any post-initialization tasks.
 */
void lvgx_port_post_deinit(void);
/**
 * Registers the ID for the GLYPHIX_APPLET_VIEW. This function should be called
 * during system initialization.
 * @param id The ID for the GLYPHIX_APPLET_VIEW.
 */
void lvgx_set_applet_view(int id);
/**
 * Display the Applet View for the LVGX applets, which is an internal only API.
 */
void lvgx_applet_view_show();
/**
 * Switches all Glyphix applications to the inactivate state, and exiting the
 * GLYPHIX_APPLET_VIEW.
 */
void lvgx_applets_inactivate(void);
/**
 * Starts or navigates to a Glyphix applet, displaying it within the
 * GLYPHIX_APPLET_VIEW and bringing it to the forefront, overlaying other views.
 * @param package The package name of the application to start or navigate to.
 */
void lvgx_applet_view_enter(const char *package);

int lvgx_view_system_init(void);

int lvgx_view_system_deinit(void);

bool lvgx_send_message_to_ui_thread_async(uint16_t view_id, uint8_t msg_id, uint32_t msg_data, MSG_CALLBAK msg_cb);

#ifdef __cplusplus
}
#endif
