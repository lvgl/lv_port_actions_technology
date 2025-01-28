##  The Download Algorithm file usage instructions

1.Copy the leopard_flash.FLM file to the keil installation directory, e.g. C:\Keil_v5\ARM\Flash Folder

2.Open the SDK-generated keil project, click Options for Target... button, select the debugger on the right in the debug bar, for example J-LINK/J-TRACE Cortex and click Settings

3.If the debugger is connected, you can see IDCODE in the SW Device window, if the window does not display the device, please confirm whether the debugger is correctly connected to the dvb board, whether the debugger is correctly connected to the PC, whether the dvb board has opened the JTAG function, and confirm whether the port window on the left selects the debugging interface is SW, not JTAG

4.Select the Flash Download tab, click the Add button in the Programming Algorithm below, and load the flash algorithm **leopard_flash** (note that the file path is: C:\Keil_v5\ARM\flash\leopard_flash. FLM)

5.Click keil's load button to download the corresponding axf file to Norflash, if there is a prompt similar to the following:

> Partial Programming Done (areas with no algorithms skipped!)
> Partial Verify OK (areas with no algorithms skipped!)
> Flash Load finished at 09:40:59

This means that the download was successful