# 如何编译ATT测试代码

1. 当前att代码既可以支持使用gcc工具链进行编译，也支持使用keil工具链编译
2. 由于编译的工程最终会生成多个可执行文件，为简化工程编译，编译模式采用命令行编译
3. 如果使用gcc工具链编译，在linux环境下直接make即可
4. 如果使用keil工具链编译，可使用powershell或cmd,输入make即可
5. keil编译的时候，需要设置一下系统环境变量，将keil安装路径设置到系统环境变量中
6.  对于keil, 增加对armclang/armlink工具链的支持，由于该工具是在windows上使用的，所以makefile也相应的适配了windows的一些命令行工具。Makefile会根据当前的OS运行环境选择clang编译器或gcc编译器，固件打包的时候会把atttest.bin打包到固件，默认的atttest.bin是keil环境编译的，如果想使用gcc环境的，需要到att目录重新make一遍att代码

