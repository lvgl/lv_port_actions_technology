#include "gx_appletkit.h"
#include "gx_application.h"
#include "gx_briefmanifest.h"
#include "gx_dir.h"
#include "gx_eventloop.h"
#include "gx_file.h"
#include "gx_lvgx.h"
#include "gx_packagedatabase.h"
#include <msg_manager.h>
#include "gx_event.h"

extern "C" {
extern lv_font_t lv_default_font;
static K_SEM_DEFINE(ui_event_quit, 0, 1);

void wait_vsync_sem(void);
int ui_message_send_async2(uint16_t view_id, uint8_t msg_id, uint32_t msg_data,
                           MSG_CALLBAK msg_cb);

extern const lv_obj_class_t lvgx_widget_class;
}

using namespace gx;

static void uiEventsForwardCallback(app_msg *, int, void *) {
  App() && App()->processEvents(EventLoop::ExcludePaintEvents |
                                EventLoop::ExcludeInputEvents);
}

static void uiEventsForwardEntry(void *, void *, void *) {
  Application *app = App();
  while (app->eventLoop()->waitEvents()) {
    // Several related header files cannot be included.
    lvgx_send_message_to_ui_thread_async(0, 0, 0, uiEventsForwardCallback);
  }
  k_sem_give(&ui_event_quit);
}

static void uiEventsForward() {
  static struct k_thread thread;
  static K_KERNEL_STACK_DEFINE(stack, 1024);

  k_thread_create(&thread, stack, K_THREAD_STACK_SIZEOF(stack),
                  uiEventsForwardEntry, nullptr, nullptr, NULL, -2, 0,
                  K_NO_WAIT);
  k_thread_name_set(&thread, "gx_evtfor");
  k_thread_start(&thread);
}

static void uiEventsDiscard() {
  if (App()) { 
    App()->quit();
    App()->processEvents();
  }
  k_sem_take(&ui_event_quit, K_FOREVER);
}

//! Copy the applet icon to the file system.
static void setup_applet_icons() {
  Dir("/data/icons").mkpath();
  PackageDatabase *db = AppletKit::instance()->database(ADBT_Applet);
  Vector<String> applets = db->keys();
  for (const auto &name : applets) {
    auto manifest = AppletKit::instance()->queryPackage(name);
    String dst = "/data/icons/" + manifest.package() + ".png";
    if (!File::exists(dst))
      File::copy(manifest.iconUri(), dst);
  }
}

void lvgx_port_post_init(void) {
  uiEventsForward();
  setup_applet_icons();
}

void lvgx_port_post_deinit(void) {
    uiEventsDiscard();
}
