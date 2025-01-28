cd "Debug"
lib /OUT:libdisplay.lib bitmap_font_api.obj glyf_decompress.obj lvgl_bitmap_font.obj lvgl_input_dev.obj lvgl_view.obj lvgl_virtual_display.obj gesture_manager.obj input_dispatcher.obj input_recorder.obj input_recorder_buffer.obj input_recorder_slide_fixedstep.obj input_recorder_stream.obj ui_service.obj view_animation.obj view_manager.obj view_manager_gui.obj libdisplay_version.obj cube_map.obj face_map.obj img2d_map.obj
xcopy libdisplay.lib "..\..\lib\" /Y
echo "gen libdisplay done"
