1、把client与server文件夹均拷贝至虚拟机桌面，直接鼠标选中拉进去就可以
2、开一个终端，运行wireshark,命令为：   sudo wireshark
4、按说明提示设置wireshark为实时抓包，选择抓包接口为 any，这样可以捕获本地通过lo接口传递的包
5、如果没有tftp.c代码，根据实验指导书编写代码，并完成编译
2、开两个终端，分别进入server及client文件夹
6、在两个终端下分别运行服务器与客户端
a、./tftpd
b、./c localhost 6969 get zImage
7、一切顺利的话，就成功了，没成功，通过看wireshark分析。