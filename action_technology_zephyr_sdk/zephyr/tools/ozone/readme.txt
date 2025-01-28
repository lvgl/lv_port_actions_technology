1. zephyr_cm4.js is an ozone plugin for debugging and loading OS information

2. The ozone version number should be greater than 3.22c

3. When using it, copy the zephyr_cm4.js to the plugin/os directory under the ozone installation directory

4. After opening the ozone tool, enter Project.SetOSPlugin ("ZephyrPlugin_CM4") in the console input field of ozone

5. After that, there is an additional zephyr option in the view window, and when you open it, you can see the running information of each task

6. The native ozone zephyr plugin has bugs and some task stack backtrace is incomplete, which has been fixed by this plugin
