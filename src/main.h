#pragma once

// 功能接口， 实现在各个功能模块cpp中。
int Auth();     // 认证。 auth.cpp
int Login(bool autoOption);    // 登录。 loginout.cpp
int Logout();   // 退出。 loginout.cpp
