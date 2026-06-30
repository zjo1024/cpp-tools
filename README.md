# C++ 小工具集

平时写的一些 C++ 小工具，全部基于 Windows API 开发。

## 工具列表

| 文件 | 功能 | 使用方法 |
|------|------|----------|
| ball-v.cpp | 球体体积计算器 | 输入半径，输出球体体积 |
| str-e.cpp | 字符串编码转换工具 | 输入字符串和编码方式（utf-8 / ascii-2 / ascii-16 / base64 / url） |
| file_encoder.cpp | 文件编码转换工具 | `file_encoder 文件名 编码方式` |
| file_decoder.cpp | 文件解码还原工具 | `file_decoder 文件名`（自动检测编码）或 `file_decoder 文件名 编码方式` |
| guard.cpp | 进程守护工具 | `guard /n 进程名` 或 `guard /p PID` |
| pin_to_top.cpp | 窗口置顶工具 | 输入进程名，持续保持其窗口置顶 |
| shredder.cpp | 文件粉碎工具 | `shredder 文件路径 [-p 次数] [-m]` |
| prott.cpp | 进程保护服务 | `prott install` 安装服务 / `prott uninstall` 卸载服务 |
| jyt.cpp | 屏幕广播窗口化工具 | 直接运行，后台托盘监控 |

## 编译环境

- 操作系统：Windows
- 编译器：g++ 或 MSVC
- 部分工具需要链接 `user32.lib`、`kernel32.lib`、`advapi32.lib` 等 Windows 库

## 编译示例

```bash
g++ ball-v.cpp -o ball-v.exe
g++ guard.cpp -o guard.exe -luser32 -lkernel32
g++ jyt.cpp -o jyt.exe -luser32 -lshell32 -ladvapi32
```

## 注意事项

- `jyt.cpp` 建议以**管理员身份**运行，否则可能无法修改其他进程的窗口样式
- `prott.cpp` 需要**管理员权限**安装为 Windows 服务
- 所有工具均为 Windows 专用，不支持 Linux/macOS
