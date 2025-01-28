cd "..\Output\Objects\Debug\LVGL.libdisplay\Win32"
lib /OUT:libdisplay.lib gesture_manager.obj input_dispatcher.obj input_recorder.obj input_recorder_buffer.obj input_recorder_slide_fixedstep.obj input_recorder_stream.obj ui_service.obj view_animation.obj view_manager.obj view_manager_gui.obj libdisplay_version.obj
xcopy libdisplay.lib "..\..\..\..\..\lib\" /Y
echo "gen libdisplay done"
