// Guard.cpp
#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <sstream>

using namespace std;

// ==================== 全局变量 ====================
HANDLE g_hThread = NULL;
bool g_bRunning = true;
DWORD g_targetPID = 0;
string g_exePath = "";
string g_exeName = "";

// ==================== 工具函数 ====================
string IntToStr(int n) {
    char buf[32];
    _itoa(n, buf, 10);
    return string(buf);
}

string DwordToStr(DWORD n) {
    char buf[32];
    _ultoa(n, buf, 10);
    return string(buf);
}

void Show(const string& msg) {
    cout << msg << endl;
}

void Info(const string& msg) {
    cout << "[*] " << msg << endl;
}

void Error(const string& msg) {
    cerr << "[-] " << msg << endl;
}

void Warn(const string& msg) {
    cout << "[!] " << msg << endl;
}

// ==================== 进程函数 ====================
bool IsAlive(DWORD pid) {
    if (pid == 0) return false;
    HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!h) return false;
    
    DWORD code;
    GetExitCodeProcess(h, &code);
    CloseHandle(h);
    return (code == STILL_ACTIVE);
}

DWORD FindPID(const char* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    
    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);
    
    DWORD result = 0;
    vector<DWORD> pids;
    
    if (Process32First(snap, &pe)) {
        do {
            if (_stricmp(pe.szExeFile, name) == 0) {
                pids.push_back(pe.th32ProcessID);
            }
        } while (Process32Next(snap, &pe));
    }
    
    CloseHandle(snap);
    
    if (!pids.empty()) {
        result = pids[0];
        if (pids.size() > 1) {
            Warn("发现 " + IntToStr(pids.size()) + " 个同名进程，保护第一个 PID: " + DwordToStr(result));
        }
    }
    
    return result;
}

string GetName(const string& path) {
    size_t pos = path.find_last_of("\\/");
    if (pos != string::npos) {
        return path.substr(pos + 1);
    }
    return path;
}

DWORD StartProc(const string& path) {
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    si.cb = sizeof(si);
    
    char* cmd = new char[path.length() + 1];
    strcpy(cmd, path.c_str());
    
    DWORD pid = 0;
    
    if (CreateProcess(NULL, cmd, NULL, NULL, FALSE,
                     CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi)) {
        pid = pi.dwProcessId;
        CloseHandle(pi.hThread);
        CloseHandle(pi.hProcess);
        Info("启动成功 PID: " + DwordToStr(pid));
    } else {
        Error("启动失败 代码: " + DwordToStr(GetLastError()));
    }
    
    delete[] cmd;
    return pid;
}

// ==================== 守护线程 ====================
DWORD WINAPI GuardFunc(LPVOID lpParam) {
    int interval = *(int*)lpParam;
    
    Info("守护中，间隔 " + IntToStr(interval/1000) + " 秒");
    
    while (g_bRunning) {
        Sleep(interval);
        
        if (!IsAlive(g_targetPID)) {
            Warn(g_exeName + " 已停止");
            
            if (!g_exePath.empty()) {
                Info("重启 " + g_exeName);
                DWORD newPID = StartProc(g_exePath);
                if (newPID > 0) {
                    g_targetPID = newPID;
                }
            }
        }
    }
    
    delete (int*)lpParam;
    return 0;
}

// ==================== 帮助信息 ====================
void Help() {
    cout << "\n";
    cout << "Guard - 进程守护工具\n";
    cout << "版本: 1.0\n\n";
    cout << "用法: guard [参数]\n\n";
    cout << "参数:\n";
    cout << "  /?        显示帮助\n";
    cout << "  /p PID    保护指定PID\n";
    cout << "  /n 名称   保护进程名\n";
    cout << "  /f 路径   保护程序路径\n";
    cout << "  /s 秒数   检查间隔(默认2秒)\n";
    cout << "  /l        列出进程\n\n";
    cout << "示例:\n";
    cout << "  guard /?             帮助\n";
    cout << "  guard /n notepad     保护记事本\n";
    cout << "  guard /p 1234        保护PID 1234\n";
    cout << "  guard /f app.exe     保护app.exe\n";
    cout << "  guard /s 5 /n calc   5秒检查计算器\n\n";
}

// ==================== 参数解析 ====================
bool ParseArgs(int argc, char* argv[]) {
    if (argc < 2) {
        Error("需要参数");
        cout << "使用 Guard /? 查看帮助" << endl;
        return false;
    }
    
    if (strcmp(argv[1], "/?") == 0) {
        Help();
        return false;
    }
    
    if (strcmp(argv[1], "/l") == 0) {
        system("tasklist");
        return false;
    }
    
    if (argc < 3) {
        Error("参数不足");
        return false;
    }
    
    int interval = 2000;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "/p") == 0 && i + 1 < argc) {
            g_targetPID = atoi(argv[i + 1]);
            if (g_targetPID == 0) {
                Error("无效PID");
                return false;
            }
            
            // 获取进程名
            HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
            if (snap != INVALID_HANDLE_VALUE) {
                PROCESSENTRY32 pe;
                pe.dwSize = sizeof(PROCESSENTRY32);
                
                if (Process32First(snap, &pe)) {
                    do {
                        if (pe.th32ProcessID == g_targetPID) {
                            g_exeName = pe.szExeFile;
                            g_exePath = g_exeName;
                            break;
                        }
                    } while (Process32Next(snap, &pe));
                }
                CloseHandle(snap);
            }
            
            if (g_exeName.empty()) {
                Error("找不到PID " + DwordToStr(g_targetPID));
                return false;
            }
            
            i++;
        }
        else if (strcmp(argv[i], "/n") == 0 && i + 1 < argc) {
            g_exeName = argv[i + 1];
            g_exePath = g_exeName;
            g_targetPID = FindPID(g_exeName.c_str());
            
            if (g_targetPID == 0) {
                Error("找不到进程: " + g_exeName);
                return false;
            }
            
            i++;
        }
        else if (strcmp(argv[i], "/f") == 0 && i + 1 < argc) {
            g_exePath = argv[i + 1];
            g_exeName = GetName(g_exePath);
            g_targetPID = FindPID(g_exeName.c_str());
            i++;
        }
        else if (strcmp(argv[i], "/s") == 0 && i + 1 < argc) {
            int sec = atoi(argv[i + 1]);
            if (sec < 1) sec = 1;
            if (sec > 60) sec = 60;
            interval = sec * 1000;
            i++;
        }
    }
    
    // 启动进程
    if (g_targetPID == 0 && !g_exePath.empty()) {
        Info("启动: " + g_exePath);
        g_targetPID = StartProc(g_exePath);
        if (g_targetPID == 0) {
            return false;
        }
    }
    
    // 验证
    if (!IsAlive(g_targetPID)) {
        Error("进程不存在");
        return false;
    }
    
    // 启动线程
    int* pInt = new int(interval);
    g_hThread = CreateThread(NULL, 0, GuardFunc, pInt, 0, NULL);
    
    if (g_hThread == NULL) {
        Error("创建线程失败");
        delete pInt;
        return false;
    }
    
    return true;
}

// ==================== 主函数 ====================
int main(int argc, char* argv[]) {
    SetConsoleTitle("Guard - 进程守护");
    
    // 禁用 Ctrl+C
    SetConsoleCtrlHandler(NULL, TRUE);
    
    cout << "\nGuard - 进程守护工具\n";
    
    if (!ParseArgs(argc, argv)) {
        system("pause");
        return 1;
    }
    
    cout << "\n================================\n";
    Info("开始保护");
    cout << "进程: " << g_exeName << endl;
    cout << "PID:  " << g_targetPID << endl;
    cout << "路径: " << g_exePath << endl;
    cout << "\n关闭窗口停止保护\n";
    cout << "================================\n\n";
    
    // 主循环 - 等待线程结束或窗口关闭
    while (g_bRunning) {
        // 检查线程是否还在运行
        DWORD exitCode;
        if (GetExitCodeThread(g_hThread, &exitCode)) {
            if (exitCode != STILL_ACTIVE) {
                // 线程已结束
                g_bRunning = false;
                break;
            }
        }
        
        // 检查控制台窗口是否还在
        if (GetConsoleWindow() == NULL) {
            g_bRunning = false;
            break;
        }
        
        Sleep(1000);
    }
    
    // 清理
    g_bRunning = false;
    if (g_hThread) {
        WaitForSingleObject(g_hThread, 3000);
        CloseHandle(g_hThread);
    }
    
    Info("守护结束");
    return 0;
}
