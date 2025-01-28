#include "gx_lvgx.h"
#include "gx_applet.h"
#include "gx_appletkit.h"
#include "gx_application.h"
#include "gx_brightness.h"
#include "gx_dir.h"
#include "gx_environment.h"
#include "gx_event.h"
#include "gx_eventloop.h"
#include "gx_file.h"
#include "gx_fontfacemap.h"
#include "gx_fontmanager.h"
#include "gx_framework.h"
#include "gx_keyevent.h"
#include "gx_object.h"
#include "gx_painter.h"
#include "gx_persimwear.h"
#include "gx_pixmappaintdevice.h"
#include "gx_platformadapter.h"
#include "gx_profiling_backend.h"
#include "gx_snprintf.h"
#include "gx_styleengine.h"
#include "gx_touchevent.h"
#include "gx_window.h"
#include "gx_widgetmanager.h"
#include "gx_filelogger.h"
#include "platform/gx_time.h"
#include "src/core/lv_obj_tree.h"
#include "vglite/gx_vglite_register.h"
#include "gx_runtime_version.h"

#ifdef GX_EXTRA_MAPVIEW
#include "gx_mapview.h"
#endif

#define GX_LOG_TAG "lvgx"
#include "gx_logger.h"

#ifndef CONFIG_GX_LOG_CAPACITY
#define CONFIG_GX_LOG_CAPACITY (8 * 1024 * 1024)
#endif

using namespace gx;

extern "C" {
extern const lv_obj_class_t lvgx_widget_class;
}

static LvglGlyphix *lvgx;

class VDBPaintDevice : public PixmapPaintDevice {
public:
  VDBPaintDevice() GX_DEFAULT;
  void setBuffer(void *buffer, int w, int h) {
    data.pixels = static_cast<uint8_t *>(buffer);
    data.width = w;
    data.height = h;
    data.pitch = w * 2;
    data.pixfmt = PixelFormat::RGB565;
  }
};

static TouchEvent::State touchProcState = TouchEvent::Released;
static int first_touch = false;

static void printVersion() {
  LogInfo() << logger::nospace << "Glyphix Nucleus v" << CoreApplication::versionString()
            << " commit " GX_GIT_COMMIT_HASH " - " GX_GIT_COMMIT_DATE;
  LogInfo() << "Glyphix Runtime commit " GX_RUNTIME_GIT_COMMIT_HASH " - "
            << GX_RUNTIME_GIT_COMMIT_DATE;
  LogInfo() << "Glyphix UI heap memory pool size: " << CONFIG_GX_UI_HEAP_MEM_POOL_SIZE;
  LogInfo() << "Glyphix rootfs path: " << CONFIG_GX_STORAGE_ROOTFS_PATH << logger::endl
            << "Glyphix log capacity: " << CONFIG_GX_LOG_CAPACITY << logger::endl
            << "Screen height: " << CONFIG_GX_SCREEN_HEIGHT << logger::endl
            << "Screen width: " << CONFIG_GX_SCREEN_WIDTH << logger::endl
            << "Screen dpi: " << CONFIG_GX_SCREEN_DPI << logger::endl
            << "Default font size: " << CONFIG_GX_DEFAULT_FONT_SIZE;
}

class LVGLWindow : public lv_obj_t {
public:
  class WidgetWrapper : public Window {
  public:
    WidgetWrapper() : m_active() {}

    lv_obj_t *lv_cast() const {
      return reinterpret_cast<lv_obj_t *>(
          intptr_t(this) -
          intptr_t(&static_cast<LVGLWindow *>(nullptr)->widget));
    }
    void onBeforePaint() {
      if (m_active) {
        m_active = false;
        lv_obj_clear_flag(lv_cast(), LV_OBJ_FLAG_HIDDEN);
      }
      if (isInvalidated() && !App()->widgetManager()->unitedDirtyRect().isEmpty()) {
        lv_obj_invalidate(lv_cast());
      }
    }
    void onAppletActiveChanged(Applet *applet, bool active) {
      LogInfo() << "onAppletActiveChanged:" << applet->objectName() << active;
      if (!active && AppletKit::instance()->appletStack().empty()) {
        lv_obj_add_flag(lv_cast(), LV_OBJ_FLAG_HIDDEN);
        m_active = false;
        lvgx_applets_inactivate();

        App()->postEvent(new TouchEvent(TouchEvent::Released, Point(0, 0), os::clock_ms()));    // make sure the gesture is always raised when exiting the three-way app
        
      } else if (active && AppletKit::instance()->appletStack().empty()) {
        m_active = true;
      }
    }

  private:
    bool m_active;
  };

  WidgetWrapper widget;

  LVGLWindow() {
    lv_obj_add_flag(this, LV_OBJ_FLAG_CLICKABLE | LV_OBJ_FLAG_SCROLLABLE |
                              LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(this,
                      LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(this, onPressed, LV_EVENT_PRESSED, nullptr);
    lv_obj_add_event_cb(this, onPressing, LV_EVENT_PRESSING, nullptr);
    lv_obj_add_event_cb(this, onReleased, LV_EVENT_RELEASED, nullptr);

    App()->widgetManager()->beforePaint.connect(&widget,
                                                &WidgetWrapper::onBeforePaint);
    AppletKit::instance()->appletActiveChanged.connect(
        &widget, &WidgetWrapper::onAppletActiveChanged);

    if (AppletKit::instance()->activeApplet())
      lv_obj_clear_flag(this, LV_OBJ_FLAG_HIDDEN);
  }
  ~LVGLWindow();

  void setGeometry(const Rect &rect) {
    lv_obj_set_pos(this, rect.x(), rect.y());
    lv_obj_set_size(this, rect.width(), rect.height());
    widget.setGeometry(rect);
  }

private:
  static void postPressEvent(TouchEvent::State state) {
    if(state == TouchEvent::Pressed) {
      if(!first_touch) {
        touchProcState = TouchEvent::Pressed;
        first_touch = true;
      } else {
        return;
      }
    } else if(state == TouchEvent::Released) {
      first_touch = false;
      touchProcState = TouchEvent::Released;
    }
    lv_indev_t *indev = lv_indev_get_act();
    if (lv_indev_get_type(indev) != LV_INDEV_TYPE_POINTER)
      return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);
    // LogInfo() << state << Point(p.x, p.y) << os::clock_ms();
    App()->postEvent(new TouchEvent(state, Point(p.x, p.y), os::clock_ms()));
  }
  static void onPressed(lv_event_t *e) {
    LVGLWindow *w = static_cast<LVGLWindow *>(lv_event_get_current_target(e));
    if (w->class_p != &lvgx_widget_class) {
      LogWarn() << "w->class_p != &lvgx_widget_class";
      return;
    }
    postPressEvent(TouchEvent::Pressed);
  }
  static void onPressing(lv_event_t *e) { postPressEvent(TouchEvent::Pressed); }
  static void onReleased(lv_event_t *e) {
    postPressEvent(TouchEvent::Released);
  }
};

static void lv_widget_constructor(const lv_obj_class_t *class_p,
                                  lv_obj_t *obj) {
  LV_UNUSED(class_p);
  LV_TRACE_OBJ_CREATE("begin");
  new (static_cast<LVGLWindow *>(obj)) LVGLWindow;
  LV_TRACE_OBJ_CREATE("finished");
}

static void lv_widget_destructor(const lv_obj_class_t *class_p, lv_obj_t *obj) {
  LVGLWindow *w = static_cast<LVGLWindow *>(obj);
  w->~LVGLWindow();
}

static void draw_widget(lv_event_t *e) {
  lv_obj_t *obj = lv_event_get_target(e);
  LVGLWindow *w = static_cast<LVGLWindow *>(obj);
  lv_draw_ctx_t *draw_ctx = lv_event_get_draw_ctx(e);
  lv_draw_rect_dsc_t draw_dsc_sel;
  lv_draw_rect_dsc_init(&draw_dsc_sel);
  draw_dsc_sel.bg_color = lv_color_hex(0xff808080);
  lv_area_t coords;
  lv_obj_get_content_coords(obj, &coords);

  // lv_draw_rect(draw_ctx, &draw_dsc_sel, &coords);
  int buf_w = lv_area_get_width(draw_ctx->buf_area);
  int buf_h = lv_area_get_height(draw_ctx->buf_area);
  VDBPaintDevice buf;
  buf.setBuffer(draw_ctx->buf, buf_w, buf_h);
  Painter p(&buf);
  Point position(coords.x1, coords.y1);
  Rect clip(Point(draw_ctx->clip_area->x1, draw_ctx->clip_area->y1),
            Point(draw_ctx->clip_area->x2, draw_ctx->clip_area->y2));
  Point offset(draw_ctx->buf_area->x1, draw_ctx->buf_area->y1);
  position -= offset;
  clip -= offset;
  w->widget.render(&p, position, clip,
                   Widget::DrawChild | Widget::UpdateShowing);
}

static void lv_widget_event(const lv_obj_class_t *class_p, lv_event_t *e) {
  LV_UNUSED(class_p);

  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_DRAW_MAIN) {
    draw_widget(e);
  }
}

const lv_obj_class_t lvgx_widget_class = {
    .base_class = &lv_obj_class,
    .constructor_cb = lv_widget_constructor,
    .destructor_cb = lv_widget_destructor,
    .event_cb = lv_widget_event,
    .width_def = LV_SIZE_CONTENT,
    .height_def = LV_SIZE_CONTENT,
    .instance_size = sizeof(LVGLWindow),
};

LVGLWindow *gx_lvgl_widget_create(lv_obj_t *parent) {
  LV_LOG_INFO("begin");
  lv_obj_t *obj = lv_obj_class_create_obj(&lvgx_widget_class, parent);
  lv_obj_class_init_obj(obj);
  return static_cast<LVGLWindow *>(obj);
}

static void setupFont(float fontSize) {
  // load font-faces map
  String uri = "/font-faces";
  if (!File::exists(uri))
    uri = "pkg:///font-faces";
  FontFaceMap &map = App()->fontManager()->faces();
  if (!map.readFile(uri))
    LogError() << "failed to load global font face mapping table";
  Font font("sans-serif", fontSize); // config default font
  if (!font.isNull())
    App()->setFont(font);
  else
    LogError() << "failed to load default font";
}

class AsyncProxyObject : public Object {
#define ASYNC_STACK_SIZE (8 * 1024)
public:
  AsyncProxyObject() {
    k_sem_init(&quit, 0, 5);
    // high priority task
    static K_KERNEL_STACK_DEFINE(hig0_stack, ASYNC_STACK_SIZE);
    static k_thread_stack_t *high_stack[1] = {hig0_stack};
    static struct k_thread high_thread[1];
    threadCreate(1, high_thread, high_stack, &higPriorityLoop, 2, "hig");
    // normal priority task
    static K_KERNEL_STACK_DEFINE(nor0_stack, ASYNC_STACK_SIZE);
    static K_KERNEL_STACK_DEFINE(nor1_stack, ASYNC_STACK_SIZE);
    static k_thread_stack_t *normal_stack[2] = {nor0_stack, nor1_stack};
    static struct k_thread normal_thread[2];
    threadCreate(2, normal_thread, normal_stack, &norPriorityLoop, 5, "nor");
    // low priority task
    static K_KERNEL_STACK_DEFINE(low0_stack, ASYNC_STACK_SIZE);
    static K_KERNEL_STACK_DEFINE(low1_stack, ASYNC_STACK_SIZE);
    static k_thread_stack_t *low_stack[2] = {low0_stack, low1_stack};
    static struct k_thread low_thread[2];
    threadCreate(2, low_thread, low_stack, &lowPriorityLoop, 12, "low");
  }
  ~AsyncProxyObject() override {
    higPriorityLoop.postEvent(new QuitEvent);
    norPriorityLoop.postEvent(new QuitEvent);
    lowPriorityLoop.postEvent(new QuitEvent);
    for (size_t i = 0; i < 5; i++) {
      k_sem_take(&quit, K_FOREVER);
    }
  }
  bool event(Event *event) override {
    if (event->type() == Event::Task) {
      auto *task = static_cast<TaskEvent *>(event);
      switch (task->priority()) {
      case TaskEvent::HighPriority: higPriorityLoop.postEvent(event); break;
      case TaskEvent::NormalPriority: norPriorityLoop.postEvent(event); break;
      case TaskEvent::LowPriority: lowPriorityLoop.postEvent(event); break;
      default: break;
      }
      return true;
    }
    delete event;
    return false;
  }

private:
  void threadCreate(int count, struct k_thread thread[], k_thread_stack_t *stack[], EventLoop *loop, int priority, const char *name) {
    char threadName[32] = {0};
    for (int i = 0; i < count; i++) {
      k_thread_create(&thread[i], stack[i], ASYNC_STACK_SIZE, threadEntry, loop, &quit, NULL, priority, 0, K_NO_WAIT);
      gx::snprintf(threadName, sizeof(threadName), "%s_%s_%d", "gx", name, i);
      k_thread_name_set(&thread[i], threadName);
      k_thread_start(&thread[i]);
    }
  }
  static void threadEntry(void *p1, void *p2, void *p3) {
    EventLoop *loop = (EventLoop *)p1;
    struct k_sem *quit = (struct k_sem*)p2;
    while (loop->waitEvents())
      loop->processEvents();
    k_sem_give(quit);
  }

private:
  EventLoop higPriorityLoop;
  EventLoop norPriorityLoop;
  EventLoop lowPriorityLoop;
  struct k_sem quit;
};

class LvgxPlatform : public PlatformAdapter {
public:
  void processNative(CoreApplication *app, int filter) override {
    if (filter & EventLoop::ExcludeInputEvents)
      os::delay_ms(1);
  }

  void screenMetrics(PaintDeviceMetric *metric) const override {
    metric->width = CONFIG_GX_SCREEN_WIDTH;
    metric->height = CONFIG_GX_SCREEN_HEIGHT;
    metric->dpi = CONFIG_GX_SCREEN_DPI;
    metric->pixelFormat = PixelFormat::RGB565;
    metric->shape = (CONFIG_GX_SCREEN_WIDTH == CONFIG_GX_SCREEN_HEIGHT);
  }
};

static void renamePrc2Pkg() {
  Dir dir("/apps");
  Vector<String> files;
  for(auto list = dir.listdir(Dir::NoDot | Dir::NoDirectory); list.hasNext(); list.next()) {
    if(Dir::splitExtension(list.name()).second == ".prc")
      files.push_back(list.name());
  }
  for(auto &name : files)
    dir.rename(name, Dir::splitExtension(name).first += ".pkg");
  dir.setPath("/pkgs");
  dir.mkpath();
  dir.setPath("");
  if(dir.exists("global.prc"))
    dir.rename("global.prc", "global.pkg");
}

static void preinstallation(AppletKit *kit) {
  renamePrc2Pkg();
  const String packages[] = {
    "apps/com.xfaith.recorder-test.pkg",
    "apps/com.xfaith.device-info-test.pkg",
    "apps/com.xfaith.file-storge-test.pkg",
    "apps/com.xfaith.network-test.pkg",
    "apps/com.xfaith.crypto-test.pkg",
    "apps/com.xfaith.asr-demo.pkg",
    "apps/com.xfaith.game-2048.pkg",
    "apps/com.xfaith.woodenfish.pkg",
    "apps/com.xfaith.buddhabeads.pkg",
    "apps/com.xfaith.flappy-bird.pkg",
    "apps/com.xfaith.weather.pkg",
    "apps/com.xfaith.calendar.pkg",
    "apps/com.xfaith.bbc-news.pkg",
    "apps/com.xfaith.map.pkg",
    "apps/com.xfaith.spotify.pkg",
    "apps/com.xfaith.ximalaya.pkg",
    "apps/com.xfaith.netease-cloud-music.pkg",
  };
  for(const String &fileUri : packages) {
    auto status = kit->installPackage(fileUri, AppletKit::UpgradeOnly);
    if(status && status != AppletKit::InvalidVersion)
      LogWarn() << "install fail:" << fileUri;
  }
  os::timestamp_ms();
}

struct LvglGlyphix : PrimitiveObject {
  Application app;
  RotateFileLoggerStream loggerStream;
  JsVM vm;
  AppletKit *kit;
  PersimWear *service;
  LVGLWindow *lvglWindow;
  ProfilingBackend profBackend;
  Widget window;
  LvglGlyphix() : app(new LvgxPlatform), loggerStream("/logs", CONFIG_GX_LOG_CAPACITY, 4), lvglWindow() {
    loggerStream.setFeature(LoggerStream::WriteFile | LoggerStream::Timestamp, true);
    lvgx = this;
    gx_engine_register();
    EnvPath::setEntry(EnvPath::GlobalPackage, "/global.pkg");
    EnvPath::packages().push_back(EnvPath::Entry("/pkgs"));
    app.styleEngine()->setPalette(StyleEngine::Background, Color::Black);
    app.styleEngine()->setPalette(StyleEngine::Text, Color::White);
    app.setDefaultPixelFormat(PixelFormat::RGB565, false);
    app.setCacheCapacity(Application::ImageTextureCache, 1800 * 1024);
    app.setCacheCapacity(Application::FontGlyphCache, 1 * 1024 * 1024);
    app.setCacheCapacity(Application::WidgetSnapshotCache, 800 * 1024);
    app.setDefaultPixelFormat(PixelFormat::RGB565, false);
    app.setDefaultPixelFormat(PixelFormat::ARGB4444, true);
    app.setAsyncProxy(new AsyncProxyObject);

    kit = new AppletKit(&window, "/pkgs/pkg.db");
    preinstallation(kit);
    setupFont(CONFIG_GX_DEFAULT_FONT_SIZE);
    
    service = new PersimWear;
    // set system language and transitions
    kit->setLanguage("zh-CN");
    kit->setOptions(AppletKit::NoLauncherApplet);
    // kit->setDefaultTransition(AppletKit::OpenEnter, "slide");
    // kit->setDefaultTransition(AppletKit::OpenExit, "zoom");
    // kit->setDefaultTransition(AppletKit::CloseEnter, "zoom");
    // kit->setDefaultTransition(AppletKit::CloseExit, "slide");

    kit->packageChanged.connect(this, &LvglGlyphix::onPackageChanged);
    kit->appletActiveChanged.connect(this, &LvglGlyphix::onAppletActiveChanged);

#ifdef GX_EXTRA_MAPVIEW
    kit->installWidget<MapView>("Mapview");
#endif

    lvgx_port_post_init();
    printVersion();
  }
  ~LvglGlyphix() {
    lvgx_port_post_deinit();
    delete lvglWindow;
    delete kit;
    delete service;
    lvgx = nullptr;
  }

private:
  void onPackageChanged() { lvgx_applet_list_update(); }
  void onAppletActiveChanged(Applet *, bool active) {
    if (active && kit->appletStack().empty()) {
      lvgx_applet_view_show();
    }
  }
};

LVGLWindow::~LVGLWindow() {
  LogInfo() << "LVGLWindow::~LVGLWindow():" << this;
  if (lvgx->lvglWindow == this) {
    LogInfo() << "LVGLWindow::~LVGLWindow(lvgxInstance->lvglWindow)";
    lvgx->window.setParent(nullptr);
    lvgx->lvglWindow = nullptr;
  }
}

void lvgx_init(void) {
  GX_ASSERT(lvgx == nullptr);
  screen_always_bright(true);
  lvgx_heap_init();
  new LvglGlyphix;
}

void lvgx_deinit(void) {
  if (lvgx) {
    delete lvgx;
    lvgx = nullptr;
  }
}

void lvgx_post_key_event(lvgx_key_code code, bool state) {
  App()->postEvent(new KeyEvent(code == LVGX_KEY_POWER ? KeyPower : KeyFunction,
                                state ? KeyEvent::Down : KeyEvent::Up,
                                os::clock_ms()));
}

void lvgx_attach_window(lv_obj_t *parent) {
  GX_ASSERT(lvgx != nullptr);
  LogInfo() << "lvgx attach window:" << parent;
  if (lv_obj_get_parent(lvgx->lvglWindow) == parent)
    return;
  if (lvgx->lvglWindow)
    lv_obj_del(lvgx->lvglWindow);
  lvgx->lvglWindow = gx_lvgl_widget_create(parent);
  lv_area_t coords;
  lv_obj_get_content_coords(parent, &coords);
  Rect rect(0, 0, lv_area_get_width(&coords), lv_area_get_height(&coords));
  lvgx->lvglWindow->setGeometry(rect);
  lvgx->window.setParent(&lvgx->lvglWindow->widget);
  lvgx->window.setRect(rect);
  lvgx->kit->appletsWindow()->setGeometry(rect);
  lvgx->kit->popupsWindow()->setGeometry(rect);
  lvgx->kit->toastsWindow()->setGeometry(rect);
}

int lvgx_launch_applet(const char *package) {
  return AppletKit::instance()->launch(package) ? 0 : -1;
}

void lvgx_process_events() {
  if (App()){
    touchProcState = TouchEvent::Released;
    first_touch = false;
    App()->processEvents();
  }
}

bool lvgx_applet_actived(void) {
  if (!AppletKit::instance())
    return false;
  return AppletKit::instance()->activeApplet();
}
extern "C" {
int ui_message_send_async2(uint16_t view_id, uint8_t msg_id, uint32_t msg_data,
                           MSG_CALLBAK msg_cb);
}

bool lvgx_send_message_to_ui_thread_async(uint16_t view_id, uint8_t msg_id, uint32_t msg_data, MSG_CALLBAK msg_cb) {
#ifdef CONFIG_UI_SERVICE
	return ui_message_send_async2(view_id, msg_id, msg_data, msg_cb);
#else
	struct app_msg msg = {
		.sender = view_id,
		.type = MSG_UI_EVENT,
		.cmd = msg_id,
		.value = (int)msg_data,
		.callback = msg_cb,
	};
	return !send_async_msg((char *)"main", &msg);
#endif

}
