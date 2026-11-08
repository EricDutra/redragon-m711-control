@echo off
setlocal

set "VSPATH="
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq tokens=*" %%i in (`call "%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSPATH=%%i"
if not defined VSPATH (
    echo Visual Studio with C++ tools not found.
    exit /b 1
)
call "%VSPATH%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1

cd /d "%~dp0"
if not exist build mkdir build
if not exist build\obj mkdir build\obj

set "IMGUI=third_party\imgui"
set "CFLAGS=/nologo /std:c++20 /O2 /MT /EHsc /W3 /utf-8 /DUNICODE /D_UNICODE /DNOMINMAX /DWIN32_LEAN_AND_MEAN /I src /I %IMGUI% /I %IMGUI%\backends /Fo:build\obj\\"
set "LIBS=hid.lib setupapi.lib user32.lib advapi32.lib"

cl %CFLAGS% src\cli_main.cpp src\mouse_protocol.cpp src\hid_device.cpp src\i18n.cpp /Fe:build\redragon-cli.exe /link %LIBS% /SUBSYSTEM:CONSOLE || exit /b 1

if exist src\app_main.cpp (
    cl %CFLAGS% src\app_main.cpp src\app_ui.cpp src\i18n.cpp src\keys.cpp src\mouse_protocol.cpp src\hid_device.cpp ^
        %IMGUI%\imgui.cpp %IMGUI%\imgui_draw.cpp %IMGUI%\imgui_tables.cpp %IMGUI%\imgui_widgets.cpp ^
        %IMGUI%\backends\imgui_impl_win32.cpp %IMGUI%\backends\imgui_impl_dx11.cpp ^
        /Fe:build\RedragonControl.exe /link %LIBS% d3d11.lib d3dcompiler.lib dxgi.lib gdi32.lib dwmapi.lib shell32.lib /SUBSYSTEM:WINDOWS || exit /b 1
)

echo.
echo Build finished in build\
