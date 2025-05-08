cd ".\Debug"
lib /OUT:libdisplay.lib gesture_manager.obj input_dispatcher.obj ui_service.obj view_animation.obj view_manager.obj view_manager_gui.obj libdisplay_version.obj
xcopy libdisplay.lib "..\..\lib\" /Y
echo "gen libdisplay done"
