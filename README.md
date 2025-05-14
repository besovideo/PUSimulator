### 介绍
PUSimulator是基于BVCSP实现的设备模拟器项目。  
目的：
* 开放源码用于供BVCSP使用者了解如何开发设备端。
* 用于测试BVCSP功能。
* 用于模拟设备测试平台性能。

### 目录说明
```
bin     生成/调试 目录
build   sln工程文件/编译临时文件 目录
include BVCSP库相关头文件 目录
lib     BVCSP库相关dll/lib 目录
src     模拟器源码 目录
   base 对BVCSP库接口的高级封装，初学者不用关心。
   function  对base封装的C接口封装，简化为单设备上线+单路音视频+GPS相关C接口和回调。初学者只需要看bv.h中接口和参数。
   simulator 模拟器代码，模拟实时音视频和GPS定位。初学者可以参考。
linux   linux下编译以及相关lib/头文件 目录
```
> 正常情况下，您只需根据情况修改src/main.cpp里的代码，不需要修改src中子目录里的代码，他们是对bvcsp接口的封装。**重点看main.cpp+function/bv.h两个文件**

### Linux编译说明

通过linux目录下makefile编译，生成可执行文件PUSimulator。
> 注意：编译前，请先准备好libBVCSP.so及其依赖库。
在make命令中传入需要进行编译的linux平台，例如 make OS=x86。

### Linux运行

将可执行文件PUSimulator与相关依赖库拷贝至对应linux系统中运行。

### Windows编译说明

项目是Windows Visual Studio 2010项目，推荐使用Visual Studio 2010编译。  
项目工程文件在build目录中。  

### Windows运行

1. 修改bin目录下pusimulator.ini配置文件。重点：id（设备ID号）ip（上线服务器地址） port（上线服务器端口）
2. 自动拷贝文件运行：直接双击bin目录下的run.bat运行。  
3. 手动拷贝文件运行：将lib目录下的dll文件拷贝到bin目录下。 双击PUSimulator.exe 运行。
> BVCSP库依赖msvcr100.dll，msvcp100.dll（没有vs2010开发环境，可以安装运行环境）。
PUSimulator.exe 是开放源码的，具体依赖的运行环境，需要根据您的开发环境确定。 
BVCSP是收费库，需要认证后，才能上线服务器。认证需要联系销售人员（需提供auth_code，见打印输出）。

### 设备端开发流程
刚接触bvcsp库设备开发者，**不用关心bvcsp库和base里的接口，只需要看main.cpp中的源码**。
**保留base，function目录中文件不用修改.按目录结构拷贝到您的项目中。**
**simulator目录中为模拟音视频+GPS数据源代码, 供参考，不用保留**。
**main.cpp中为具体需要看懂，并修改的代码，拷贝一份到您的项目中修改。**
参考main.cpp中源码，实现：

- 认证相关参数填写;
-  登录相关参数填写;
- 实时音视频相关参数和回调；
- GPS定位相关参数和回调；
- 文件相关参数和回调；文件的读写已经封装在base里，回调只需要支持文件列表和文件详情。

#### 认证
BVCSP是收费库，需要认证后，才能上线服务器。认证需要联系销售人员（需提供auth_code）。  
认证相关代码在 main.cpp中，需要您做如下修改：

* 修改**APP_ID,APP_N,APP_E** 为您申请的开发者密钥信息（需要联系销售人员分配）。
* 修改**PU_ID**为您根据硬件信息自动生成的设备ID（不能和其它设备冲突），格式为PU_%X
* 修改**my_auth_inf()**函数，填写您的真实设备硬件信息。
> 相同的硬件信息会被认为是同一台设备，认证信息绑定硬件信息，同一个硬件信息平台同时只允许一个有效。

#### 上线/下线服务器
修改main.cpp文件**main()函数**中登录服务器的**地址，端口**等参数。

#### 实时音视频
如果**不支持实时音视频，只需要修改InitBVLib的channelParam为NULL**。
音视频相关功能实现可以参考simulator/media.cpp中从文件中读取音视频帧并发送。

修改main.cpp中以下代码：
* **main()**中对**channelParam**参数的填写：**视频、音频的编码信息**（如果有）,**是否支持对讲**等参数；
* 实现**OpenMedia()**接口，用于根据请求的媒体类型打开采集+编码，将音视频帧数据通过SendAudioData()/SendVideoData()接口发送给平台，参考simulator/media.cpp中实现。
* 实现**CloseMedia()**接口，用于关闭所有采集+编码，不再发送音视频数据。
* 实现**ReqPLI()**接口，用于通知编码器产生关键帧。
* **如果支持对讲， 实现OnRecvAudio()**接口，用于播放来自平台的音频。
*  **如果支持云台，实现OnPTZCtrl()**接口，用于响应云台控制命令。

#### GPS 
如果**不支持GPS定位，只需要修改InitBVLib的gpsParam为NULL**。
如果支持GPS定位，请参考下面的实现步骤。
GPS相关功能实现可以参考simulator/gps.cpp中模拟生成定位数据。

修改main.cpp中以下代码：
*  修改**main()**中对**gpsParam**参数的填写，**正常情况下不需要改动**。
*  修改**OnSubscribeGPS()/OnGetGPSData()/OnGetGPSParam()**接口实现，**正常情况下不需要改动**。
* 修改**GPSInterval**参数初始值，**建议根据设备应用场景改为一个合适的上报间隔**。
* **建议修改OnSetGPSParam()**实现，将后台配置的**GPSInterval保存到文件**。
* **必须实现GetGPSData()**接口，用于获取最新GPS定位数据。

#### 录像文件
录像文件传输相关功能（读写）都已经封装在base中，不要开发者关心。开发者需要维护好录像文件列表，将main.cpp中获取文件列表和文件详情相关接口重新实现。支持了文件相关接口，便支持了平台的在线回放功能。
如果**不支持音视频录像文件，只需要修改InitBVLib的fileParam为NULL**。

修改main.cpp中以下代码支持录像文件：
* 修改**main()**中**fileParam.iBandwidthLimit**参数，用于限制文件传输速率。
* 修改**OnGetRecordFiles()**文件检索接口，根据过滤条件返回真实的文件列表。
* 修改**OnFileRequest()**文件下载请求接口，根据文件路径返回真实的文件信息。
* 修改**OnGetRecordStatus()**获取录像状态接口，返回当前真实的录像状态。
* 修改**OnManualRecord()**手动开始/停止录像接口，根据命令要求，开始或停止 手动录像（不应影响定时/报警等录像逻辑）。