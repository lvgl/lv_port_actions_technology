#include "gx_applet.h"
#include "gx_appletkit.h"
#include "gx_briefmanifest.h"
#include "gx_lvgx.h"
#include "gx_packagedatabase.h"
#include <cstddef>
#include <lvgl/lvgl_freetype_font.h>

#include "gx_logger.h"

extern "C" {
#include "view_stack.h"

extern lv_font_t lv_default_font;
}

using namespace gx;

struct AppletInfo {
  String package;
  String icon;
};

static void event_handler(lv_event_t *event) {
  auto *info = static_cast<AppletInfo *>(event->user_data);
  if (event->code == LV_EVENT_CLICKED) {
    AppletKit::instance()->launch(info->package);
  } else if (event->code == LV_EVENT_DELETE) {
    delete info;
  }
}

static lv_obj_t *applet_list_obj = nullptr;

static void applet_list_delete_event(lv_event_t *event) {
  if (event->code == LV_EVENT_DELETE)
    applet_list_obj = nullptr;
}

void lvgx_applet_list(lv_obj_t *parent) {
  lv_obj_t *cont = lv_obj_create(parent);
  lv_obj_set_pos(cont, 50, 50);
  lv_obj_set_size(cont, 360, 360);
  lv_obj_set_style_bg_color(cont, lv_color_hex(0x3B3B3B), LV_PART_MAIN);
  lv_obj_center(cont);
  lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN_REVERSE);
  lv_obj_add_event_cb(cont, applet_list_delete_event, LV_EVENT_DELETE, nullptr);
  applet_list_obj = cont;
  lvgx_applet_list_update();
}

void lvgx_applet_list_update() {
  lv_obj_t *cont = applet_list_obj;
  if (!cont)
    return;
  lv_obj_clean(cont); // delete all children.

  PackageDatabase *db = AppletKit::instance()->database(ADBT_Applet);
  Vector<String> applets = db->keys();
  for (const auto &name : applets) {
    lv_obj_t *btn = lv_btn_create(cont);
    auto manifest = AppletKit::instance()->queryPackage(name);
    auto *info = new AppletInfo;
    info->package = manifest.package();
    info->icon = "/NAND:/data/icons/" + manifest.package() + ".png";

    lv_obj_add_event_cb(btn, event_handler, LV_EVENT_ALL, info);
    lv_obj_set_size(btn, 360, 80);

    // lv_obj_set_style_bg_color(btn, lv_color_hex(0x3B3B3B), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(btn, 255, LV_PART_MAIN);
    lv_obj_set_style_pad_all(btn, 1, LV_PART_MAIN);
    lv_obj_set_style_border_width(btn, 2, LV_PART_MAIN);
    lv_obj_set_style_radius(btn, 20, LV_PART_MAIN);

    lv_obj_t *label = lv_label_create(btn);
    lv_obj_set_style_text_font(label, &lv_default_font, 0); // set ttf font
    lv_label_set_text(label, manifest.name().c_str());
    lv_obj_center(label);

    lv_obj_t *img = lv_img_create(btn);
    lv_img_set_src(img, info->icon.c_str());
    lv_obj_set_size(img, 64, 64);
    lv_obj_align(img, LV_ALIGN_LEFT_MID, 0, 0);
  }
  lv_obj_invalidate(lv_scr_act());
}

static int applet_view_id = 0;

void lvgx_set_applet_view(int id) { applet_view_id = id; }

void lvgx_applet_view_show() {
#ifdef CONFIG_UI_SERVICE
  if (applet_view_id && view_stack_get_top() != applet_view_id) {
    view_stack_push_view(applet_view_id, NULL);
  }
#endif
}

void lvgx_applets_inactivate() {
#ifdef CONFIG_UI_SERVICE
  if (view_stack_get_top() == applet_view_id)
    view_stack_pop();
#endif
}
