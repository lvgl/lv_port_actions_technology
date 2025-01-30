# LVGL port to Actions Technology ATS30xx based EVB

## Overview

This repository provides the LVGL integration to the Actions Technology ATS30xx family of
System on Chips that have a comprehensive set of peripherals plus display controller and 
a 2.5D GPU compatible to the VGLite API to perform graphics rendering faster. Even though
the SDK is contained on this repository including the LVGL, there is a minimal, LVGL ready
application for users creating their own applications.

Please notice, the SDK is on a Beta version so changes in the API as long this repo evolves
is expected.

## Buy

Contact Actions Technology Sales to acquire a board: https://www.actionstech.com/index.php?id=contactus&siteId=4


## Benchmark

The default configuration of the LVGL in the Actions SDK supports double buffering plus acceleration
using the SoC GPU, which is compatible to the VGLite API. On this configuration it is possible to 
get a smooth perfomance of 30FPS with a 17% of CPU usage in average cases considering the smartwatch
case on the SDK which uses a round display of 466 x 466 pixels.


## Specification

### CPU and Memory
- **MCU:** Actions Technology ATS30xx Family ARM Core Cortex-M33
- **RAM:** 1.1MB of internal RAM, 32MB of external PSRAM
- **Flash:** 8MB of NOR Flash built-in (EVB suppors external NAND EMMC)
- **GPU:** VGLite compatible GPU

### Display and Touch
- **Resolution:** 466 x 466 round display
- **Display Size:** N/A
- **Interface:** MIPI-DSI
- **Color Depth:** 32-bit ARGB8888 format
- **Technology:** IPS
- **Touch Pad:** Capacitive touch panel

### Connectivity
- 1 Bluetooth
- Serial peripherals: UART/SPI/I2C
- SD EMMC controller
- Audio IN + OUT

## Getting started

Before get started please notice the application firmware can be built and 
flashed using Linux or Windows, but the SoC configuration, including flashing
blobs and file-system resources can only be done using a configuration tool 
that only runs on Windows OS.

For the getting started we will considering the ATS3089P EVB which is the standard
evaluation platform from Actions technology.

### Hardware setup

Before proceed please notice, although is not mandatory, having a Jlink Debug
Probe would speed-up the development process, because it allows the user to 
flash the application to the device without using the configuration tool

- Set the DC5VIN switch to off
- If have a jlink connect the UART0 connector pins as following: 
    * JLink's SWDIO <--> UART0  RX
    * JLink's SWDCLK <--> UART0 TX
    * JLink's GND <--> UART0 GND
    * JLink's VCC is not required to be connected, although it is recommended
- Connect the USB cable from PC to the Board;
- Set the DC5VIN switch to on
- The board shall start with the default firmware on its memory

### Software setup
Although you can use your favorite IDE to edit code like VSCode, the SDK operation
like build or flashing will be mostly on the time into the terminal.

- Install Zephyr SDK (Linux or Windows): https://docs.zephyrproject.org/latest/develop/toolchains/zephyr_sdk.html

- Install on Host or on Virtual Machine (in Linux development enviroment) the config tool it is located in this repo in tools/config_tool.rar

- If JLink is available install JLink software package and Ozone: https://www.segger.com/downloads/jlink/

- If JLink is available, locate the place of JLinkDevices folder on your host OS and copy `tools/JLinkDevices/Actions` folder to that location, it will add the support of the ATS30xx SoC on JLink Software (including Ozone).

### Run the project

Building and running an application involves the several steps, please check everyone, as long you repeat them, it start to become easy to memorize it.

- Clone this repository repository, simple cloning only no submodules are present

- Open your favorite IDE and integrated terminal on the root of this repo

- You would see the `hello_word` folder on the root, here is the template application

- to make it discoverable there are two options:
    1. Make a symbolic link of `hello_world` folder into `action_technology_sdk/application/app_demo`
    2. Simply copy `hello_world` folder into `action_technology_sdk/application/app_demo`

- Navigate to the `action_technology_sdk/zephyr` and source the environment:
    1. On Windows just type: ```.\zephyr-env.cmd\ ```
    2. On Linux: ```$ source zephyr-env.sh```

- For building your application just run the build script:
    1. On Windows just type: ```.\build.cmd\ -n ```
    2. On Linux: ```$ ./build.sh -n```

- You should see the following menu:

```
 Select boards configuration:

    1. ats3089p_dev_watch
    2. ats3089p_dev_watch_sdnand
    3. ats3085s_dev_watch_ext_nor
    4. ats3089p_dev_watch_sdnand_8b
    5. ats3089c_dev_watch
    6. ats3089_dev_watch

Which would you like? [ats3089p_dev_watch] 

```

- For the board we are using on this guide, select option 4, then you should see:

```
 Select application:

    1. app_demo
    2. bt_watch

Which would you like? [app_demo] 

```
- Select option 1 to list the sample apps plus the hello world template:

```
 Select app_demo sub app:

    1. usbd_msc_demo
    2. bt_call_demo
    3. noise_cancel_demo
    4. hello_world
    5. audio_demo
    6. local_music_demo
    7. bt_music_demo
    8. gpu_demo
    9. lvgl_demo
    10. usbh_msc_demo

Which would you like? [usbd_msc_demo] 

```
- Now you see the hello world demo select it depending on which option it takes, in case here it is option 4, after selecting it the build will start and if it is okay you should see at the end of logs:

```
FW: Post build app.bin


 build cmd : ['python', '-B', '/home/ulipe/action_tech_ws/lv_port_action_technolgy-ATS3089P_EVB/action_technology_sdk/zephyr/tools/pack_app_image.py', '/home/ulipe/action_tech_ws/lv_port_action_technolgy-ATS3089P_EVB/action_technology_sdk/application/app_demo/hello_world/outdir/ats3089p_dev_watch_sdnand_8b/_firmware/bin/app.bin']


build finished

```

- The files of interest for building are located on `hello_world/outdir/ats3089p_dev_watch_sdnand_8b`

- The two most important files are:

    1. The firmware configuration file:`hello_world/outdir/ats3089p_dev_watch_sdnand_8b/_firmware/ats3089p_dev_watch_sdnand_8b_250129.fw` 

    2. The zephyr application binary file: `hello_world/outdir/ats3089p_dev_watch_sdnand_8b/zephyr/zephyr.[elf | hex | bin]`

- In the posession of the firmware configuration file (move it to Windows Virtual Machine on Linux environment), open the previously installed configuration tool.

- It will ask for the location of this file, provide the path and click in OK.

- Then go to the board, turn the DC5VIN switch to OFF and hold the key ADFU.

- Move the DC5VIN switch to on.

- On Virtual Machine scenario attach the Actions Technology DFU device to the VM.

- On the config tool in the port selection, select ADFU0.

- Go on factory settings tab and set to YES the storage erase option

- Click on the Download button on the side of the Port checkbox.

- In some cases the connection might be closed, just repeat the procedure to put the board in DFU and click in the Download again

- Wait for finish, depending on the environment the progress bar will not show green, just the messade "DOWNLOAD FINISHED!", you will know if succeds because once finished the firmware will start to print the hello world on the display.

For quick changes, and development, you can use the JLink for download (users on Linux environment for now can drop from the Virtual Machine). for using JLink, make sure you updated the JLinkDevices folder as instructed in the Software Setup section, then go to the terminal inside of zephyr folder.

- Rebuild the firmware, to use the current workspace just run the build script from before but without any parameters.

- Put the board in the ADFU0 mode.

- Use west flash for download:

```
west flash -r jlink -d ../../hello_world/outdir/ats3089p_dev_watch_sdnand_8b/

```

- If okay you should see something like that and the application starts immediately:

```
-- west flash: rebuilding
ninja: no work to do.
-- west flash: using runner jlink
-- runners.jlink: JLink version: 8.10
-- runners.jlink: Flashing file: ../../hello_world/outdir/ats3089p_dev_watch_sdnand_8b/zephyr/zephyr.hex

```

- Failures here also may occur, so just retry if failure is reported.

### Debugging

For now debug is not supported yet, stay tuned a stable set of steps will be provided soon. For
now users can use the console log provided on the board UART0 connector after the power-up 
steps complete. Use your favorite terminal in a 115200bps/8/N/1 configuration.

## Notes

Even though the debug is not supported, it is possible to hack the Jlink Ozone to estabilish
a Debug connection and run the application, just when opening Ozone select `Open an existing Project`
and inside of `action_technology_sdk/zephyr/tools/ozone` use the .jdebug file and use your zephyr.elf
produced to flash and debug. Please notice this method is not stable so you may face failures like
device unexpected disconnection.

You can also try out the bt_watch application which is a complete watch demonstration app, during build you can just select the option two in the second menu:

```
 Select application:

    1. app_demo
    2. bt_watch

Which would you like? [app_demo] 
```
Let the build finish and proceed like the regular hello world application.

Also to create another application you can just duplicate hello world, change the name and copy the folder (or create a symbolic link) into app_demo subfolder, this procedure also works if you want to have multiple different apps.

## Contribution and Support

If you find any issues with the development board feel free to open an Issue in this repository. For LVGL related issues (features, bugs, etc) please use the main [lvgl repository](https://github.com/lvgl/lvgl).

If you found a bug and found a solution too please send a Pull request. If you are new to Pull requests refer to [Our Guide](https://docs.lvgl.io/master/CONTRIBUTING.html#pull-request) to learn the basics.

