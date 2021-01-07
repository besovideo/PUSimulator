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
```

### 编译说明
项目是Windows Visual Studio 2010项目，推荐使用Visual Studio 2010编译。  
项目工程文件在build目录中。  

### 运行
将lib目录下的dll文件拷贝到bin目录下。bin目录是生成/调试目录。  
BVCSP依赖msvcr100.dll，msvcp100.dll。  
PUSimulator.exe 是开放源码的，具体依赖的运行环境，需要根据您的开发环境确定。  
可以直接双击bin目录下的run.bat运行。  