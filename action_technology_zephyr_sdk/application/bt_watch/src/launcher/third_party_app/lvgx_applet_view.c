/**
 * @file third_party_app view
 */

#include "app_ui.h"
#include "system_app.h"
#include "third_party_app_ui.h"
#include "third_party_app_view.h"
#include <view_stack.h>


#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
#include "dvfs.h"
#endif

#include "gx_lvgx.h"

static int _glyphix_applet_view_preload(view_data_t *view_data) {
  ARG_UNUSED(view_data);
  ui_view_update(GLYPHIX_APPLET_VIEW);
  return 0;
}

static int _glyphix_applet_view_layout_update(view_data_t *view_data,
                                              bool first_layout) {
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
  dvfs_set_level(DVFS_LEVEL_HIGH_PERFORMANCE, "glyphix");
#endif
  lvgx_attach_window(lv_display_get_screen_active(view_data->display));
  return 0;
}

static int _glyphix_applet_view_layout(view_data_t *view_data) {
  int ret = _glyphix_applet_view_layout_update(view_data, 1);
  if (ret < 0)
    return ret;

  lv_refr_now(view_data->display);
  SYS_LOG_INF("_third_party_app_view_layout");

  return 0;
}

static int _glyphix_applet_view_delete(view_data_t *view_data) {
  printk("_glyphix_enter_view_delete closed lv_default_font \n");
#ifdef CONFIG_ACTS_DVFS_DYNAMIC_LEVEL
  dvfs_unset_level(DVFS_LEVEL_HIGH_PERFORMANCE, "glyphix");
#endif
  return 0;
}

static int _glyphix_applet_view_proc_key(lv_obj_t *scr,
                                         ui_key_msg_data_t *key_data) {
  if (lvgx_applet_actived()) {
    switch (key_data->event) {
    case (KEY_POWER | KEY_TYPE_SHORT_UP):
      lvgx_post_key_event(LVGX_KEY_POWER, false);
      break;
    case (KEY_POWER | KEY_TYPE_SHORT_DOWN):
      lvgx_post_key_event(LVGX_KEY_POWER, true);
      break;
    case (KEY_TBD | KEY_TYPE_SHORT_UP):
      lvgx_post_key_event(LVGX_KEY_FUNC, false);
      break;
    case (KEY_TBD | KEY_TYPE_SHORT_DOWN):
      lvgx_post_key_event(LVGX_KEY_FUNC, true);
      break;
    }
  }
  if (KEY_VALUE(key_data->event) == KEY_GESTURE_RIGHT ||
      key_data->event == (KEY_POWER | KEY_TYPE_SHORT_UP)) {
    key_data->done = true; /* ignore gesture */
  }

  return 0;
}

static int _glyphix_applet_view_handler(uint16_t view_id,
                                        view_data_t *view_data, uint8_t msg_id,
                                        void *msg_data) {
  switch (msg_id) {
  case MSG_VIEW_PRELOAD:
    return _glyphix_applet_view_preload(view_data);
  case MSG_VIEW_LAYOUT:
    return _glyphix_applet_view_layout(view_data);
  case MSG_VIEW_DELETE:
    return _glyphix_applet_view_delete(view_data);
  case MSG_VIEW_KEY:
    return _glyphix_applet_view_proc_key(
        lv_display_get_screen_active(view_data->display), msg_data);
  default:
    return 0;
  }
  return 0;
}

VIEW_DEFINE2(glyphix_applet_view, _glyphix_applet_view_handler, NULL, NULL,
             GLYPHIX_APPLET_VIEW, NORMAL_ORDER, UI_VIEW_LVGL, DEF_UI_WIDTH,
             DEF_UI_HEIGHT);
