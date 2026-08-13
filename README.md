# WindowsManager
Control windows system windows/processes

MinGW compile command: g++ -O3 -Os -mwindows -static -s -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--strip-all WindowsManager.cpp -lcomctl32 -lshlwapi -lgdi32 -luser32 -lkernel32 -lole32 -luuid -o WindowsManager.exe
