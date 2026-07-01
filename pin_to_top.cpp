#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <iostream>
#include <conio.h>
using namespace std;

// 函数声明
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam);
BOOL CALLBACK EnumExplorerWindowsProc(HWND hWnd, LPARAM lParam);
BOOL CALLBACK EnumTopmostProc(HWND hWnd, LPARAM lParam);
vector<HWND> GetWindowsByProcessId(DWORD processId);
vector<HWND> GetExplorerFileWindowsByProcessId(DWORD processId);
vector<HWND> GetAllTopmostWindows();
DWORD GetProcessIdByName(const char* processName);
void BringToTopmostFront(HWND hWnd);
void KeepProcessTopmost(const char* processName);
string GetProcessNameById(DWORD processId);

// 根据进程ID获取进程名
string GetProcessNameById(DWORD processId)
{
    string processName = "未知";
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (snapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(processEntry);
        
        if (Process32First(snapshot, &processEntry))
        {
            do
            {
                if (processEntry.th32ProcessID == processId)
                {
                    processName = processEntry.szExeFile;
                    break;
                }
            } while (Process32Next(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
    }
    
    return processName;
}

// 枚举进程窗口的回调函数
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{
    DWORD processId;
    GetWindowThreadProcessId(hWnd, &processId);
    
    pair<DWORD, vector<HWND>*>* param = 
        reinterpret_cast<pair<DWORD, vector<HWND>*>*>(lParam);
    
    if (processId == param->first)
    {
        if (IsWindowVisible(hWnd))
        {
            param->second->push_back(hWnd);
        }
    }
    return TRUE;
}

// 专门为explorer.exe枚举文件资源管理器窗口的回调函数
BOOL CALLBACK EnumExplorerWindowsProc(HWND hWnd, LPARAM lParam)
{
    DWORD processId;
    GetWindowThreadProcessId(hWnd, &processId);
    
    pair<DWORD, vector<HWND>*>* param = 
        reinterpret_cast<pair<DWORD, vector<HWND>*>*>(lParam);
    
    if (processId == param->first)
    {
        if (IsWindowVisible(hWnd))
        {
            char className[256] = {0};
            GetClassNameA(hWnd, className, 256);
            
            // 只添加文件资源管理器窗口，跳过系统组件
            if (strcmp(className, "CabinetWClass") == 0)
            {
                param->second->push_back(hWnd);
            }
        }
    }
    return TRUE;
}

// 枚举置顶窗口的回调函数
BOOL CALLBACK EnumTopmostProc(HWND hWnd, LPARAM lParam)
{
    if (GetWindowLongPtr(hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST)
    {
        vector<HWND>* windows = reinterpret_cast<vector<HWND>*>(lParam);
        windows->push_back(hWnd);
    }
    return TRUE;
}

// 根据进程ID获取窗口
vector<HWND> GetWindowsByProcessId(DWORD processId)
{
    vector<HWND> windows;
    pair<DWORD, vector<HWND>*> param(processId, &windows);
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&param));
    return windows;
}

// 获取explorer.exe的文件资源管理器窗口（跳过系统组件）
vector<HWND> GetExplorerFileWindowsByProcessId(DWORD processId)
{
    vector<HWND> windows;
    pair<DWORD, vector<HWND>*> param(processId, &windows);
    EnumWindows(EnumExplorerWindowsProc, reinterpret_cast<LPARAM>(&param));
    return windows;
}

// 获取所有置顶窗口
vector<HWND> GetAllTopmostWindows()
{
    vector<HWND> topmostWindows;
    EnumWindows(EnumTopmostProc, reinterpret_cast<LPARAM>(&topmostWindows));
    return topmostWindows;
}

// 根据进程名获取进程ID
DWORD GetProcessIdByName(const char* processName)
{
    DWORD processId = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    
    if (snapshot != INVALID_HANDLE_VALUE)
    {
        PROCESSENTRY32 processEntry;
        processEntry.dwSize = sizeof(processEntry);
        
        if (Process32First(snapshot, &processEntry))
        {
            do
            {
                if (_stricmp(processEntry.szExeFile, processName) == 0)
                {
                    processId = processEntry.th32ProcessID;
                    break;
                }
            } while (Process32Next(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
    }
    
    return processId;
}

// 将窗口置顶并放到最前面
void BringToTopmostFront(HWND hWnd)
{
    SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, 
                SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    SetForegroundWindow(hWnd);
    BringWindowToTop(hWnd);
}

// 持续置顶进程
void KeepProcessTopmost(const char* processName)
{
    system("cls");  // 清屏
    cout << "\n========== 持续置顶模式 ==========\n";
    cout << "进程: " << processName << endl;
    
    // 检测是否为explorer.exe
    bool isExplorer = (_stricmp(processName, "explorer.exe") == 0);
    if (isExplorer)
    {
        cout << "检测到 explorer.exe，将只置顶文件资源管理器窗口\n";
    }
    
    cout << "\n按 'q' 键返回主菜单\n";
    cout << "================================\n\n";
    
    int count = 0;
    bool keepRunning = true;
    
    while (keepRunning)
    {
        // 检查是否有按键输入
        if (kbhit())
        {
            char ch = getch();
            if (ch == 'q' || ch == 'Q')
            {
                keepRunning = false;
                break;
            }
        }
        
        // 获取进程ID
        DWORD processId = GetProcessIdByName(processName);
        
        if (processId == 0)
        {
            Sleep(1000);
            continue;
        }
        
        // 根据是否为explorer.exe选择不同的窗口获取方式
        vector<HWND> windows;
        if (isExplorer)
        {
            windows = GetExplorerFileWindowsByProcessId(processId);
        }
        else
        {
            windows = GetWindowsByProcessId(processId);
        }
        
        if (windows.empty())
        {
            Sleep(1000);
            continue;
        }
        
        // 获取所有置顶窗口
        vector<HWND> topmostWindows = GetAllTopmostWindows();
        
        // 检查目标窗口是否在所有置顶窗口的最前面
        bool isAtTop = false;
        
        if (!topmostWindows.empty())
        {
            HWND topmostWindow = topmostWindows[0];
            DWORD topmostPid;
            GetWindowThreadProcessId(topmostWindow, &topmostPid);
            
            if (topmostPid == processId)
            {
                if (isExplorer)
                {
                    char className[256] = {0};
                    GetClassNameA(topmostWindow, className, 256);
                    if (strcmp(className, "CabinetWClass") == 0)
                    {
                        isAtTop = true;
                    }
                }
                else
                {
                    isAtTop = true;
                }
            }
        }
        else
        {
            isAtTop = true;
        }
        
        // 如果不在最前面，就调整位置
        if (!isAtTop)
        {
            cout << "调整窗口位置... (第" << ++count << "次)" << endl;
            
            for (size_t i = 0; i < windows.size(); i++)
            {
                if (isExplorer)
                {
                    char className[256] = {0};
                    GetClassNameA(windows[i], className, 256);
                    if (strcmp(className, "CabinetWClass") == 0)
                    {
                        BringToTopmostFront(windows[i]);
                    }
                }
                else
                {
                    BringToTopmostFront(windows[i]);
                }
                Sleep(50);
            }
        }
        
        //Sleep(500);
    }
}

int main()
{
    while (true)
    {
        system("cls");  // 清屏
        cout << "\n========== 窗口置顶工具 ==========\n";
        cout << "1. 开始持续置顶\n";
        cout << "0. 退出程序\n";
        cout << "================================\n";
        cout << "请选择: ";
        
        int choice;
        cin >> choice;
        
        switch (choice)
        {
            case 1:
            {
                string processName;
                cout << "请输入进程名：";
                cin >> processName;
                KeepProcessTopmost(processName.c_str());
                break;
            }
            
            case 0:
                cout << "程序退出" << endl;
                return 0;
                
            default:
                cout << "无效选择，请按回车键继续..." << endl;
                cin.ignore();
                cin.get();
        }
    }
    
    return 0;
}
