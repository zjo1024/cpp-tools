#include <windows.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <tlhelp32.h>

#define WM_TRAYICON (WM_USER + 1)
#define WM_RECOVER_TRAY (WM_USER + 2)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_SHOW 1002

NOTIFYICONDATA nid = {};
HWND g_hMainWnd = NULL;
HANDLE g_hMonitorThread = NULL;
HANDLE g_hRecoverThread = NULL;

// 获取窗口类名
std::string GetWindowClassName(HWND hwnd) {
    char className[256] = {0};
    GetClassNameA(hwnd, className, sizeof(className));
    return std::string(className);
}

// 获取窗口标题
std::string GetWindowTitle(HWND hwnd) {
    char title[256] = {0};
    GetWindowTextA(hwnd, title, sizeof(title));
    return std::string(title);
}

// 检查窗口是否全屏
bool IsFullscreenWindow(HWND hwnd) {
    RECT rect;
    GetWindowRect(hwnd, &rect);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // 允许5像素的误差
    return (rect.left <= 5 && rect.top <= 5 && 
            rect.right >= screenWidth - 5 && 
            rect.bottom >= screenHeight - 5);
}

// 检查是否是广播窗口
bool IsBroadcastWindow(HWND hwnd) {
    if (!IsWindowVisible(hwnd)) return false;
    
    std::string title = GetWindowTitle(hwnd);
    std::string className = GetWindowClassName(hwnd);
    
    // 方法1：通过标题精确匹配（最可靠）
    if (title == "屏幕广播" || title == "广播教学") {
        return true;
    }
    
    // 方法2：标题包含"广播"且窗口可见
    if ((title.find("广播") != std::string::npos || 
         title.find("教学") != std::string::npos) && 
        IsFullscreenWindow(hwnd)) {
        return true;
    }
    
    // 方法3：特定的类名（极域电子教室常见类名）
    if (className.find("TForm") != std::string::npos || 
        className.find("Broadcast") != std::string::npos ||
        className.find("ScreenCast") != std::string::npos) {
        if (IsFullscreenWindow(hwnd)) {
            return true;
        }
    }
    
    return false;
}

// 查找广播窗口
HWND FindBroadcastWindow() {
    HWND hFoundWnd = NULL;
    
    // 方法1：通过标题直接查找
    hFoundWnd = FindWindowA(NULL, "屏幕广播");
    if (hFoundWnd && IsWindowVisible(hFoundWnd)) {
        return hFoundWnd;
    }
    
    hFoundWnd = FindWindowA(NULL, "广播教学");
    if (hFoundWnd && IsWindowVisible(hFoundWnd)) {
        return hFoundWnd;
    }
    
    // 方法2：枚举所有窗口，检查是否是广播窗口
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL CALLBACK {
        if (IsBroadcastWindow(hwnd)) {
            HWND* pResult = (HWND*)lParam;
            *pResult = hwnd;
            return FALSE;
        }
        return TRUE;
    }, (LPARAM)&hFoundWnd);
    
    return hFoundWnd;
}

// 修改窗口样式
bool ModifyWindowStyle(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) {
        return false;
    }
    
    // 获取当前样式
    LONG style = GetWindowLong(hwnd, GWL_STYLE);
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    
    // 添加标题栏和系统菜单
    style |= WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | 
             WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
    
    // 移除弹出窗口样式
    style &= ~WS_POPUP;
    
    // 如果窗口是全屏，可能需要移除最大化标志
    if (style & WS_MAXIMIZE) {
        style &= ~WS_MAXIMIZE;
    }
    
    // 应用新样式
    if (!SetWindowLong(hwnd, GWL_STYLE, style)) {
        return false;
    }
    
    // 设置扩展样式（移除置顶等）
    exStyle &= ~WS_EX_TOPMOST;
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle);
    
    // 设置窗口大小和位置（1024x768居中）
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int width = 1024;
    int height = 768;
    int x = (screenWidth - width) / 2;
    int y = (screenHeight - height) / 2;
    
    // 更新窗口位置和大小
    BOOL result = SetWindowPos(hwnd, HWND_TOP, x, y, width, height,
                               SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    
    if (!result) {
        return false;
    }
    
    // 强制重绘窗口框架
    RedrawWindow(hwnd, NULL, NULL, RDW_FRAME | RDW_UPDATENOW);
    
    // 发送恢复消息（如果窗口是最大化状态）
    PostMessage(hwnd, WM_SYSCOMMAND, SC_RESTORE, 0);
    
    return true;
}

// 添加托盘图标
void AddTrayIcon(HWND hwnd) {
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hwnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    strcpy_s(nid.szTip, "屏幕广播窗口化工具");
    
    Shell_NotifyIcon(NIM_ADD, &nid);
}

// 删除托盘图标
void RemoveTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

// 显示托盘菜单
void ShowTrayMenu(HWND hwnd) {
    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, ID_TRAY_SHOW, "显示窗口");
    AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenu(hMenu, MF_STRING, ID_TRAY_EXIT, "退出");
    
    // 获取鼠标位置
    POINT pt;
    GetCursorPos(&pt);
    
    // 显示菜单
    SetForegroundWindow(hwnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
    PostMessage(hwnd, WM_NULL, 0, 0);
    
    DestroyMenu(hMenu);
}

// 检查窗口是否有效
bool IsWindowValid(HWND hwnd) {
    return hwnd != NULL && IsWindow(hwnd);
}

DWORD WINAPI MonitorBroadcast(LPVOID lpParam) {
    HWND hOldWnd = NULL;
    int errorCount = 0;
    
    while (TRUE) {
        // 检查主窗口是否还存在
        if (!IsWindowValid(g_hMainWnd)) {
            break;
        }
        
        // 查找广播窗口
        HWND hBroadcastWnd = FindBroadcastWindow();
        
        if (hBroadcastWnd && hBroadcastWnd != hOldWnd) {
            // 托盘气泡提示
            nid.uFlags = NIF_INFO;
            strcpy_s(nid.szInfo, "检测到屏幕广播，正在窗口化...");
            strcpy_s(nid.szInfoTitle, "屏幕广播窗口化工具");
            nid.dwInfoFlags = NIIF_INFO;
            Shell_NotifyIcon(NIM_MODIFY, &nid);
            
            // 尝试修改窗口
            if (ModifyWindowStyle(hBroadcastWnd)) {
                strcpy_s(nid.szInfo, "窗口化成功！");
                Shell_NotifyIcon(NIM_MODIFY, &nid);
                hOldWnd = hBroadcastWnd;
                errorCount = 0;
            } else {
                strcpy_s(nid.szInfo, "窗口化失败");
                Shell_NotifyIcon(NIM_MODIFY, &nid);
                errorCount++;
            }
            
            // 恢复标志
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            Shell_NotifyIcon(NIM_MODIFY, &nid);
        } else if (hBroadcastWnd == NULL && hOldWnd != NULL) {
            // 广播窗口已关闭
            hOldWnd = NULL;
        }
        
        // 如果连续失败次数过多，暂停一下避免频繁操作
        if (errorCount > 10) {
            Sleep(5000);
            errorCount = 0;
        } else {
            Sleep(2000);  // 每2秒检查一次
        }
    }
    
    return 0;
}

// 定时恢复托盘图标的线程
DWORD WINAPI RecoverTrayIcon(LPVOID lpParam) {
    while (TRUE) {
        Sleep(5000);  // 每5秒检查一次
        
        // 检查主窗口是否还存在
        if (!IsWindowValid(g_hMainWnd)) {
            break;
        }
        
        // 尝试重新添加托盘图标
        AddTrayIcon(g_hMainWnd);
        
        // 确保图标消息标志正确
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_TRAYICON;
        Shell_NotifyIcon(NIM_MODIFY, &nid);
    }
    return 0;
}

// 窗口过程函数
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE:
            g_hMainWnd = hwnd;
            AddTrayIcon(hwnd);
            return 0;
            
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                ShowTrayMenu(hwnd);
            } else if (lParam == WM_LBUTTONDBLCLK) {
                ShowWindow(hwnd, SW_SHOW);
                SetForegroundWindow(hwnd);
            }
            return 0;
            
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_TRAY_EXIT) {
                RemoveTrayIcon();
                PostQuitMessage(0);
                return 0;
            } else if (LOWORD(wParam) == ID_TRAY_SHOW) {
                MessageBox(hwnd, 
                    "屏幕广播窗口化工具正在运行中\n\n"
                    "功能：自动检测并窗口化屏幕广播\n"
                    "状态：监控中\n"
                    "退出：右键托盘图标选择退出",
                    "关于", MB_OK | MB_ICONINFORMATION);
            }
            break;
            
        case WM_DESTROY:
            RemoveTrayIcon();
            PostQuitMessage(0);
            return 0;
    }
    
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 防止多个实例运行
    HANDLE hMutex = CreateMutex(NULL, TRUE, "BroadcastWindowTool_Mutex");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        MessageBox(NULL, "程序已经在运行中！", "提示", MB_OK | MB_ICONINFORMATION);
        return 0;
    }
    
    // 创建隐藏窗口
    const char CLASS_NAME[] = "BroadcastWindowClass";
    
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    
    RegisterClass(&wc);
    
    HWND hwnd = CreateWindowEx(
        0,
        CLASS_NAME,
        "屏幕广播窗口化工具",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    
    if (hwnd == NULL) {
        MessageBox(NULL, "创建窗口失败！", "错误", MB_OK | MB_ICONERROR);
        return 0;
    }
    
    // 隐藏主窗口
    ShowWindow(hwnd, SW_HIDE);
    
    // 检查管理员权限
    BOOL isAdmin = FALSE;
    HANDLE hToken = NULL;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        TOKEN_ELEVATION elevation;
        DWORD size = sizeof(TOKEN_ELEVATION);
        if (GetTokenInformation(hToken, TokenElevation, &elevation, size, &size)) {
            isAdmin = elevation.TokenIsElevated;
        }
        CloseHandle(hToken);
    }
    
    // 创建监控线程
    g_hMonitorThread = CreateThread(NULL, 0, MonitorBroadcast, NULL, 0, NULL);
    if (!g_hMonitorThread) {
        MessageBox(NULL, "创建监控线程失败！", "错误", MB_OK | MB_ICONERROR);
        return 1;
    }
    
    // 创建托盘图标恢复线程
    g_hRecoverThread = CreateThread(NULL, 0, RecoverTrayIcon, NULL, 0, NULL);
    if (!g_hRecoverThread) {
        MessageBox(NULL, "创建恢复线程失败！", "错误", MB_OK | MB_ICONERROR);
        TerminateThread(g_hMonitorThread, 0);
        CloseHandle(g_hMonitorThread);
        return 1;
    }
    
    // 显示启动提示
    nid.uFlags = NIF_INFO;
    if (!isAdmin) {
        strcpy_s(nid.szInfo, "建议以管理员身份运行以获得最佳效果");
        strcpy_s(nid.szInfoTitle, "提示");
        nid.dwInfoFlags = NIIF_WARNING;
    } else {
        strcpy_s(nid.szInfo, "程序已启动，将自动检测并窗口化屏幕广播");
        strcpy_s(nid.szInfoTitle, "屏幕广播窗口化工具");
        nid.dwInfoFlags = NIIF_INFO;
    }
    Shell_NotifyIcon(NIM_MODIFY, &nid);
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    Shell_NotifyIcon(NIM_MODIFY, &nid);
    
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // 清理
    if (g_hMonitorThread) {
        TerminateThread(g_hMonitorThread, 0);
        CloseHandle(g_hMonitorThread);
    }
    if (g_hRecoverThread) {
        TerminateThread(g_hRecoverThread, 0);
        CloseHandle(g_hRecoverThread);
    }
    if (hMutex) {
        ReleaseMutex(hMutex);
        CloseHandle(hMutex);
    }
    
    return 0;
}
