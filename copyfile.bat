@echo off
set ROOTDIR=G:\work\Develop\SDK\
set Target=%~dp0\

md %Target%\include 
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

xcopy %ROOTDIR%\BVCSP\include\*.h %Target%\include /s /h /y
xcopy %ROOTDIR%\BVCU\include\*.h %Target%\include /s /h /y
xcopy %ROOTDIR%\libSAV\include\*.h %Target%\include /s /h /y
xcopy %ROOTDIR%\AAA\include\AAABase.h %Target%\include /s /h /y
xcopy %ROOTDIR%\libAAA\include\*.h %Target%\include /s /h /y
