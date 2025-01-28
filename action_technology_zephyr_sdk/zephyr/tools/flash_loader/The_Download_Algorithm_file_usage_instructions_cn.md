# 下载算法文件使用说明

1.将leopard_flash.FLM文件拷贝到keil安装目录，例如C:\Keil_v5\ARM\Flash

2.打开SDK生成的keil工程，点击Options for Target...按钮, 在debug一栏中选择右侧的调试器，例如J-LINK/J-TRACE Cortex并点击settings

3.如果调试器已经连接，在SW Device窗口可以看到IDCODE，如果该窗口未显示设备，请确认调试器是否和小机正确连接，调试器是否和PC正确连接，小机是否打开了JTAG功能，同时确认左侧的port窗口是否选择的调试接口是SW，而不是JTAG

4.选择Flash Download标签页，在下面的Programming Algorithm中点击Add按钮，加载flash烧写算法leopard_flash(注意检查文件路径为:C:\Keil_v5\ARM\flash\leopard_flash.FLM)

5.点击keil的load按钮，就可以把对应的axf文件下载到norflash中，如果有类似下面的提示:

> Partial Programming Done (areas with no algorithms skipped!)
> Partial Verify OK (areas with no algorithms skipped!)
> Flash Load finished at 09:40:59

就表示下载成功
