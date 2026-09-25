===※WindowsManager※===
控制Windows系统的一些窗口/进程

MinGW编译命令（推荐）：g++ -O3 -Os -mwindows -static -s -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--strip-all WindowsManager.cpp -lcomctl32 -lshlwapi -lgdi32 -luser32 -lkernel32 -lole32 -luuid -o WindowsManager.exe

可能编译失败，这是用MinGW写的：
MSVC编译命令：cl /O2 /MT /EHsc /DUNICODE /D_UNICODE WindowsManager.cpp /link user32.lib kernel32.lib gdi32.lib comctl32.lib shlwapi.lib ole32.lib uuid.lib /OUT:WindowsManager.exe

TDM-GCC编译命令：g++ -O3 -Os -mwindows -static -s -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--strip-all WindowsManager.cpp -lcomctl32 -lshlwapi -lgdi32 -luser32 -lkernel32 -lole32 -luuid -o WindowsManager.exe

*注：
- 这个命令需要搭配Nsudo提权使用，即把Nsudo放在根目录下（也可以直接用Enigma Virtual Box合体）
- 编译好的版本都是可以直接使用（用Enigma Virtual Box合体过的）
