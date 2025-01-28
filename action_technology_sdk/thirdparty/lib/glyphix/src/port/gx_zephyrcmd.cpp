#include "gx_applet.h"
#include "gx_appletkit.h"
#include "gx_application.h"
#include "gx_briefmanifest.h"
#include "gx_coreapplication.h"
#include "gx_logger.h"
#include "gx_packagedatabase.h"
#include "gx_string.h"
#include "gx_tlsf.h"
#include "shell/shell.h"
#include "sys/printk.h"

#include "gx_logger.h"

namespace gx {
using logger::endl;
using logger::nospace;

class CmdLoggerStream : public LoggerStream {
public:
  struct LoggerState : State {
    String buffer;
  };
  CmdLoggerStream() GX_DEFAULT;
  virtual ~CmdLoggerStream() GX_DEFAULT;
  virtual State *start(int, const char *) { return new LoggerState; }
  virtual void puts(State *state, const char *span) {
    static_cast<LoggerState *>(state)->buffer += span;
  }
  virtual State *flush(State *state, bool end) {
    LoggerState *s = static_cast<LoggerState *>(state);
    printk("%s\n", s->buffer.c_str());
    delete s;
    return nullptr;
  }
};

static CmdLoggerStream LogStream;

struct LogCmd : public LogInfo {
  LogCmd() : LogInfo(&LogStream) {}
};

static void glyphix_launch(const struct shell *shell, size_t argc,
                           char **argv) {
  LogInfo() << "glyphix launch" << argv[0] << argv[1];
  if (argc == 1) {
    App()->postTask([=]() {
      LogCmd() << "Available application packages:";
      PackageDatabase *db = AppletKit::instance()->database(ADBT_Applet);
      Vector<String> applets = db->keys();
      for (const auto &name : applets) {
        auto manifest = AppletKit::instance()->queryPackage(name);
        LogCmd() << "  " << manifest.package();
      }
    });
    return;
  }
  if (argc != 2) {
    LogCmd() << "usage: glyphix launch <package-name>";
    return;
  }
  String appName = argv[1];
  App()->postTask([=]() {
    Applet *applet = AppletKit::instance()->launch(appName);
    LogCmd() << "launch application" << appName
             << (applet ? "success" : "fail");
  });
}
static void glyphix_exit(const struct shell *shell, size_t argc, char **argv) {
  if (argc == 1) {
    App()->postTask([=]() {
      LogCmd() << "running application packages:";
      Vector<Applet *> applets = AppletKit::instance()->appletInstances();
      for (Applet *a : applets) {
        LogCmd() << "  " << a->objectName();
      }
    });
    return;
  }
  if (argc != 2) {
    LogCmd() << "usage: glyphix launch <package-name>";
    return;
  }
  String appName = argv[1];
  App()->postTask([=]() {
    Applet *applet = AppletKit::instance()->lookup(appName);
    delete applet;
    JsVM::current().garbageCollection();
    App()->flushCache(1024*1024*1.5);
  });
}

static void glyphix_jsheap(const struct shell *shell, size_t argc,
                           char **argv) {
  App()->postTask([=]() {
    auto sizeLimit = JsVM::current().heapStats().size;
    auto allocated = JsVM::current().heapStats().allocated;
    JsVM::current().garbageCollection();
    auto allocatedGCed = JsVM::current().heapStats().allocated;
    LogCmd() << "JsVM heap stats:" << endl                 //
             << "        size limit:" << sizeLimit << endl //
             << "         allocated:" << allocated << endl //
             << "allocated after GC:" << allocatedGCed << nospace
             << " (-" //
             << allocated - allocatedGCed << ")";
    Vector<Applet *> applets = AppletKit::instance()->appletInstances();
    for (Applet *a : applets) {
      LogCmd() << "  " << a->objectName();
    }
  });
}

static void glyphix_cache_usage(const struct shell *shell, size_t argc,
                                char **argv) {
  App()->postTask([=]() {
    LogCmd() << "ImageTextureCache information:" << endl
             << "        Total Size:"
             << App()->cacheCapacity(Application::ImageTextureCache) << "  bytes" << endl
             << "         Used Size:" 
             << App()->cacheSize(Application::ImageTextureCache) << "  bytes" ; 
    LogCmd() << "FontGlyphCache information:" << endl
             << "        Total Size:"
             << App()->cacheCapacity(Application::FontGlyphCache) << "  bytes" << endl
             << "         Used Size:"
             << App()->cacheSize(Application::FontGlyphCache) << "  bytes";
    LogCmd() << "WidgetSnapshotCache information:" << endl
             << "        Total Size:"
             << App()->cacheCapacity(Application::WidgetSnapshotCache) << "  bytes" << endl
             << "         Used Size:"
             << App()->cacheSize(Application::WidgetSnapshotCache) << "  bytes";
  });
}

static void glyphix_memory_usage(const struct shell *shell, size_t argc,
                                 char **argv) {                            
    glyphix_jsheap(shell, argc, argv);
    glyphix_cache_usage(shell, argc, argv);
}

static void glyphix_fault(const struct shell *shell, size_t argc, char **argv) {
    k_panic();
}

SHELL_STATIC_SUBCMD_SET_CREATE(
    sub_glyphix,
    SHELL_CMD(launch, NULL, "start glyphix application", glyphix_launch),
    SHELL_CMD(exit, NULL, "stop glyphix application", glyphix_exit),
    SHELL_CMD(memory_usage, NULL, "glyphix cache and jsheap memory usage", glyphix_memory_usage),
    SHELL_CMD(fault, NULL, "trigger glyphix fault", glyphix_fault),
    SHELL_SUBCMD_SET_END
);

SHELL_CMD_REGISTER(glyphix, &sub_glyphix, "Glyphix application management",
                   NULL);
} // namespace gx
