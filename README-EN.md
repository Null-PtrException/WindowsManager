# WindowsManager

Controls some windows/processes of the Windows system.

## MinGW compile command (recommended)

```bash
g++ -O3 -Os -mwindows -static -s -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--strip-all WindowsManager.cpp -lcomctl32 -lshlwapi -lgdi32 -luser32 -lkernel32 -lole32 -luuid -o WindowsManager.exe
```

It may fail to compile; this was written with MinGW.

## MSVC compile command

```bash
cl /O2 /MT /EHsc /DUNICODE /D_UNICODE WindowsManager.cpp /link user32.lib kernel32.lib gdi32.lib comctl32.lib shlwapi.lib ole32.lib uuid.lib /OUT:WindowsManager.exe
```

## TDM-GCC compile command

```bash
g++ -O3 -Os -mwindows -static -s -ffunction-sections -fdata-sections -Wl,--gc-sections -Wl,--strip-all WindowsManager.cpp -lcomctl32 -lshlwapi -lgdi32 -luser32 -lkernel32 -lole32 -luuid -o WindowsManager.exe
```

## Notes

- This command needs to be used with Nsudo for privilege escalation, i.e. put Nsudo in the root directory (or you can directly merge them with Enigma Virtual Box).
- The compiled versions are all ready to use directly (merged with Enigma Virtual Box).