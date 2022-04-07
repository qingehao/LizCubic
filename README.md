![](doc\img\LizCubic-F.jpg)

## 简介

视频链接:https://www.bilibili.com/video/BV1fr4y1H7NV/

LizCubic借鉴了稚晖君[HoloCubic](https://github.com/peng-zhihui/HoloCubic)的使用分光棱镜达到伪全息显示的创意所制作的小玩意（异地恋神器）。

在设计上，我只在外壳上借鉴HoloCubic的实现，其硬件以及软件均与其毫无关系，因此本项目的程序并不适用于其他Cubic。

其通过4G蜂窝网络进行网络连接，具备时钟天气显示界面，可显示时间以及当前实时天气以及室外的风向、风力等级、温湿度和外出建议；具备音乐频谱界面，可显示当前环境音频的频谱，频谱的颜色具有渐变效果；具备纪念日界面，可显示当前日期距离目标日期之间的天数，其界面参考了手机端同名APP；具备HeartBeat功能，使的LizCubic(s)可进行联动，表达对远方的他（她）的思念！

## 外壳结构

与稚晖君不同的地方有四处。

1. 外壳底部开窗设计，以方便SIM卡的插拔和音频的拾取

<img src="doc\img\结构底部.png" style="zoom: 67%;" />

2. 尾部放弃使用TypeC接口直插设计，伸出一段软线+TypeC母口，尾部可使用一个线尾巴，我使用的线尾巴见图。

<img src="doc\img\尾部.png"  style="zoom:33%;" />

![image-20220407013338655](doc\img\线尾巴.png)

3. 尾部开了个正方形槽，以方便4G天线的嵌入。上图尾部带点红色的便为4G天线。

4. 内部部分区域厚度做了减薄，以为了4G模块的塞入。

## 关于硬件

LizCubic的硬件与其他Cubic完全不同，区别如下：

1. 并未采用ESP32作为主控，而是使用STM32F412CEU6。
2. 采用4G Cat.1的联网方式，基于SIMCOM的A7680C模组实现。
3. 未使用SD卡，使用的是SPI FLASH芯片，当然这使得图片以及字体资源变得不太方便写入其中。
4. 添加了音频拾取以及音频放大芯片，麦克风采用AP3722AT模拟硅麦，音频放大芯片采用MAX9814ETD，具备AGC放大功能。
5. 供电部分，A7680C采用单独的DC-DC 3.8V供电，其余部分采用LDO 3.3V供电，这里需要做好隔离以及纹波控制，不然的话，频谱会有较大干扰。
6. 4G天线采用的是贴片天线，型号为CrossAir CA-L01，使用该天线注意阻抗匹配，否则信号很差。
7. 在如此小体积的空间内塞入一个4G模块、一个4G天线还有SIM卡槽，属实比较困难。正因如此才去掉了SD卡槽，换成了nano sim卡槽，另外，特别设计了一块PCB用以连接主板和4G模块，和iPhone的双层主板设计相同，不同的是iPhone的叠层中框采用的是BGA封装设计，我这里考虑成本，使用的是半孔，最终效果十分完美。

## 软件部分

本项目整体基于RT-Thread进行开发，并使用到了FAL、LittleFs、FlashDB、at_device、jsmn、lvgl、motiondriver、pahomqtt、webclient软件包。

需要注意的是，lcd的spi驱动并未使用官方SPI驱动框架，因为官方的spi驱动在等待数据传输完成的方案上使用的死等的方式，既无法发挥RTOS多任务的优势，也使得lvgl屏幕刷新机制无法得到充分利用。

#### 构建工程

本仓库中未包含项目中使用的软件包，请下载ENV工具后，使用`pkgs --update`命令下载更新所使用的软件包，而后使用`scons --target=mdk5`命令构建并生成MDK工程（具体使用方法请移步[RT-Thread官网资料中心](https://www.rt-thread.org/document/site/#/development-tools/env/env)）。构建完成后即可打开MDK工程文件，本项目推荐使用MDK AC6编译器进行编译，并选择-Os优化等级 和 开启LTO优化，可发挥最大性能。如图所示。

<img src="doc\img\MDK优化.png" style="zoom:67%;" />

#### 代码修改

1. ##### 天气API修改

   在`firmware\LizCubic\config\liz_config.h`中修改图中两处地方。天气的获取使用的是[一刻天气](https://www.yiketianqi.com/index)API，可自行去注册账号，获得这两个值。
   
   <img src="doc\img\weather_api修改.png" style="zoom:67%;" />

2. ##### at_device软件包修改

   at_deivce中并没有A7680C的原生驱动，但其具有sim76xx的驱动，两者的AT指令稍有差别，需要做一些修改。并且官方的AT SOCKET在多TCP连接的实现上存在漏洞，也需要做一些修改。

   由于修改的地方比较多，我将修改后的文件单独上传到`firmware\LizCubic\port\A7680C`中，在构建完工程后，替换掉相应文件即可。

3. ##### MQTT服务器修改

   修改`firmware\LizCubic\config\liz_config.h`如图所示宏定义为自己的MQTT服务器即可。

   <img src="doc\img\mqtt修改.png" style="zoom:67%;" />

#### 代码编译

除了使用MDK进行编译外，可采用GCC进行编译，但需要注意的是，进行FFT计算所用的DSP库，我目前是在MDK的Run-Time Environment中配置的，使用GCC编译的话需要加入相关的文件。

#### 资源文件上传

项目中的资源文件包括字体以及HeartBeat中用到的兔小只的表情，分别在`firmware\LizCubic\app\fonts`和`firmware\LizCubic\app\imgs`目录中。这些资源文件存储在SPI FLASH中。

项目中，我已为flash挂载了文件系统，flash中的目录如下图所示。

![image-20220407195125205](doc\img\资源目录.png)

在成功编译好工程并烧录后，通过USB转串口模块连接串口1（PA9、PA10）到电脑端，电脑端使用Xshell进行连接，便可进入系统shell命令行界面，如图所示。

<img src="doc\img\系统命令行.png" style="zoom:67%;" />

输入`ry /fonts`命令后回车，右键终端界面，选择"`传输->YMODEM->用YMODEM发送`，上传`firmware\LizCubic\app\fonts\bin\msyhbd_22_ex.bin`文件到flash中。该文件为天气信息所用到的一些汉字。

其余兔小只表情的发送、接收以及跳舞的表情，使用相同方法上传到flash中，命令分别为`ry /archerr`、`ry /archers`、`ry /heart`。

## 关于其他

- 我没想着量产或者拿来卖，只是自己做给女朋友的一件礼物。
- 教程什么的，上面已经写的蛮详细的了。平时工作较忙，应该没时间出保姆级教程的视频，期待哪位大佬可以分享教程。
- 硬件不开源主要是害怕，一些人拿着这些直接就去卖了，然后导致满大街均是LizCubic，本来是我给女朋友送的礼物，我想要保持其独一无二性。
- 最近芯片涨价比较厉害。MCU可以更换为STM32F411CEU6，与F412程序完全相同，只是RAM小了点，但是够用。因为我手头有很多F412，因此才选择了这个。至于其他的MCU，可以哪位大佬自己设计硬件，使用别的方案，例如ESP32等。
- 很多人说，手机不行么？其实，并不冲突。首先这个作为一个桌面摆件，也还挺好看。其次就像现在的手环、智能手表等，其与手机功能均高度重合，但其仍具有市场，因为其拓展了通知的方式，使得即使忙于工作而无法及时查看手机消息的人以额外的通知方式。但是如果你整天捧着手机，这个确实没啥用......。最后，有种东西叫做仪式感！懂吧！
- 其他的，大家可以在issue中问我。

## 支持

<img src="doc\img\赞赏.png" style="zoom:50%;" /> 

如果 LizCubic 解决了你的问题，不妨扫描上面二维码请我 **喝杯咖啡**~ 

## 许可

采用 GPL-2.0 开源协议，细节请阅读项目中的 LICENSE 文件内容。