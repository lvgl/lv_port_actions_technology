# Glyphix 软件包

## LVGL + Glyphix 双 GUI 框架

该框架允许将 Glyphix 应用分发到使用 LVGL 作为主 GUI 方案的系统中，所有的 API 都在 `gx_lvgx.h` 头文件中提供。要启用 Glyphix，请先设置 `CONFIG_GLYPHIX=y`，或启用 `ats3089_dev_watch_glyphix` 配置方案，然后参考本文档在项目中集成。

### 集成方案概述

集成 Glyphix 的开发工作主要分为几个步骤：
- 初始化 Glyphix 和双 GUI 框架
- 实现 GLYPHIX_APPLET_VIEW view 对象
- 集成 Glyphix 应用跳转到宿主项目中

### Glyphix 初始化

在 `bt_watch` 项目中，Glyphix 的初始化在 `src/main/system_app.ui.c` 中完成，具体代码如下：
``` c
int system_app_ui_init(void) {
  // ...
#if CONFIG_GLYPHIX
  lvgx_set_applet_view(GLYPHIX_APPLET_VIEW);
  lvgx_view_system_init();
#endif
#endif
  // ...
}
```

其中 `lvgx_set_applet_view()` 函数将 `GLYPHIX_APPLET_VIEW` 注册到双 GUI 框架中，`GLYPHIX_APPLET_VIEW` 是用于 Glyphix 应用的 view 对象 ID。`lvgx_view_system_init()` 函数用于初始化 Glyphix 和双 GUI 框架，这个函数会在 UI 线程中异步调用 `lvgx_init()` 函数完成初始化。

### Glyphix View 实现

具体请参考 `bt_watch` 项目中的 `src/launcher/third_party_app/lvgx_applet_view.c`。该文件中还实现了按键事件等机制的转发。

### 跳转到 Glyphix 应用

请参考 `bt_watch` 项目中的 `src/launcher/third_party_app/third_party_app_view.c`，以及本软件包中 `src/lvgx/gx_lvgx_applist.cpp` 中的 `lvgx_applet_list()` 函数实现。