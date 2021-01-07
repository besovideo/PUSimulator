@echo off
set ROOTDIR=G:\work\Develop\SDK\
set Target=%~dp0\

md %Target%\lib
md %Target%\bin
REM ¿½±´ÎÄ¼þ
xcopy %ROOTDIR%\BVCSP\lib\Release\BVCSP*.dll %Target%\lib /h /s /y
xcopy %ROOTDIR%\BVCSP\lib\Release\BVCSP*.lib %Target%\lib /h /s /y
xcopy %ROOTDIR%\bvrtc\lib\Release\bvrtc*.dll %Target%\lib /h /s /y
xcopy %ROOTDIR%\bvrtc\lib\Release\bvrtc*.lib %Target%\lib /h /s /y

xcopy %ROOTDIR%\BVCSP\lib\Debug\BVCSP*.dll %Target%\bin /h /s /y
xcopy %ROOTDIR%\BVCSP\lib\Debug\BVCSP*.lib %Target%\bin /h /s /y
xcopy %ROOTDIR%\BVCSP\lib\Debug\BVCSP*.pdb %Target%\bin /h /s /y
xcopy %ROOTDIR%\bvrtc\lib\Debug\bvrtc*.dll %Target%\bin /h /s /y
xcopy %ROOTDIR%\bvrtc\lib\Debug\bvrtc*.lib %Target%\bin /h /s /y
xcopy %ROOTDIR%\bvrtc\lib\Debug\bvrtc*.pdb %Target%\bin /h /s /y

md %Target%\include 
xcopy %ROOTDIR%\BVCSP\include\*.* %Target%\include /s /h /y
xcopy %ROOTDIR%\BVCU\include\*.* %Target%\include /s /h /y
xcopy %ROOTDIR%\libSAV\include\*.* %Target%\include /s /h /y
xcopy %ROOTDIR%\AAA\include\AAABase.h %Target%\include /s /h /y
xcopy %ROOTDIR%\libAAA\include\*.* %Target%\include /s /h /y
