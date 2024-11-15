#pragma once

// 功能接口， 实现在各个功能模块cpp中。
int Auth();     // 认证。 auth.cpp
int Login(bool autoOption);    // 登录。 loginout.cpp
int Logout();        // 退出。 loginout.cpp
int SendAlarm();     // 发送报警，测试。
int SendCommand();
int HandleEvent();  // 处理事件。 loginout.cpp
void UploadFile1(bool bContinue); // 上传文件，测试多个设备并发上传不同文件
void UploadFile2(); // 测试同一个设备并发上传同一个文件。
