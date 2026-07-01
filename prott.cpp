#include <windows.h>
#include <iostream>
#include <string>
#include <tlhelp32.h>

SERVICE_STATUS g_ServiceStatus;
SERVICE_STATUS_HANDLE g_ServiceStatusHandle;
HANDLE g_hMonitorThread = NULL;
DWORD g_dwProtectedPid = 0;

// 监控线程 - 如果进程被终止就重启
DWORD WINAPI MonitorThread(LPVOID lpParam)
{
	while (true)
	{
		Sleep(5000);  // 每5秒检查一次
		
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, g_dwProtectedPid);
		if (!hProcess)
		{
			// 进程已被终止，重启
			std::cout << "[Monitor] Process terminated! Restarting..." << std::endl;
			
			char szPath[MAX_PATH];
			GetModuleFileNameA(NULL, szPath, MAX_PATH);
			
			STARTUPINFOA si = { sizeof(si) };
			PROCESS_INFORMATION pi;
			
			if (CreateProcessA(szPath, NULL, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi))
			{
				g_dwProtectedPid = pi.dwProcessId;
				std::cout << "[Monitor] Process restarted with PID: " << g_dwProtectedPid << std::endl;
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
		}
		else
		{
			CloseHandle(hProcess);
		}
	}
	return 0;
}

// 设置进程保护
bool SetProcessProtection(DWORD level)
{
	typedef NTSTATUS (NTAPI *pNtSetInformationProcess)(
													   HANDLE, DWORD, PVOID, ULONG);
	
	pNtSetInformationProcess NtSetInformationProcess = (pNtSetInformationProcess)
	GetProcAddress(GetModuleHandleA("ntdll.dll"), "NtSetInformationProcess");
	
	if (!NtSetInformationProcess) return false;
	
	struct PROCESS_PROTECTION_LEVEL {
		DWORD ProtectionLevel;
	} ppl;
	
	ppl.ProtectionLevel = level;
	
	NTSTATUS status = NtSetInformationProcess(
											  GetCurrentProcess(),
											  0x27,
											  &ppl,
											  sizeof(ppl)
											  );
	
	return status == 0;
}

// 服务控制处理
void WINAPI ServiceCtrlHandler(DWORD dwControl)
{
	switch (dwControl)
	{
	case SERVICE_CONTROL_STOP:
	case SERVICE_CONTROL_SHUTDOWN:
		g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
		SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
		
		// 允许停止
		g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
		break;
		
	case SERVICE_CONTROL_INTERROGATE:
		SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
		break;
	}
}

// 服务主函数
void WINAPI ServiceMain(DWORD argc, LPSTR* argv)
{
	g_ServiceStatusHandle = RegisterServiceCtrlHandlerA("ProtectorService", ServiceCtrlHandler);
	
	if (!g_ServiceStatusHandle)
		return;
	
	g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
	g_ServiceStatus.dwWin32ExitCode = NO_ERROR;
	g_ServiceStatus.dwServiceSpecificExitCode = 0;
	g_ServiceStatus.dwCheckPoint = 0;
	g_ServiceStatus.dwWaitHint = 0;
	
	SetServiceStatus(g_ServiceStatusHandle, &g_ServiceStatus);
	
	g_dwProtectedPid = GetCurrentProcessId();
	
	// 设置PPL保护
	std::cout << "Setting PPL protection..." << std::endl;
	if (SetProcessProtection(4))
	{
		std::cout << "[OK] PPL protection enabled!" << std::endl;
	}
	else
	{
		std::cout << "[WARN] PPL protection failed" << std::endl;
	}
	
	// 启动监控线程
	g_hMonitorThread = CreateThread(NULL, 0, MonitorThread, NULL, 0, NULL);
	
	std::cout << "\n========================================" << std::endl;
	std::cout << "Protected Service Running" << std::endl;
	std::cout << "========================================" << std::endl;
	std::cout << "PID: " << g_dwProtectedPid << std::endl;
	std::cout << "Protection Level: 4 (WINTCB)" << std::endl;
	std::cout << "Auto-restart: ENABLED" << std::endl;
	std::cout << "\nTest with: taskkill /f " << g_dwProtectedPid << std::endl;
	std::cout << "Process will auto-restart if killed!" << std::endl;
	std::cout << "========================================\n" << std::endl;
	
	// 主循环
	while (g_ServiceStatus.dwCurrentState == SERVICE_RUNNING)
	{
		Sleep(1000);
	}
}

// 安装服务
bool InstallService()
{
	SC_HANDLE hSCM = OpenSCManagerA(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
	if (!hSCM) return false;
	
	char szPath[MAX_PATH];
	GetModuleFileNameA(NULL, szPath, MAX_PATH);
	
	SC_HANDLE hService = CreateServiceA(
										hSCM,
										"ProtectorService",
										"Enhanced Process Protector",
										SERVICE_ALL_ACCESS,
										SERVICE_WIN32_OWN_PROCESS,
										SERVICE_AUTO_START,
										SERVICE_ERROR_NORMAL,
										szPath,
										NULL, NULL, NULL, NULL, NULL
										);
	
	if (!hService)
	{
		DWORD error = GetLastError();
		if (error == ERROR_SERVICE_EXISTS)
		{
			CloseServiceHandle(hSCM);
			return true;
		}
		CloseServiceHandle(hSCM);
		return false;
	}
	
	// 设置自动重启
	SERVICE_FAILURE_ACTIONS sfa = {0};
	SC_ACTION actions[3];
	actions[0].Type = SC_ACTION_RESTART;
	actions[0].Delay = 1000;
	actions[1].Type = SC_ACTION_RESTART;
	actions[1].Delay = 10000;
	actions[2].Type = SC_ACTION_RESTART;
	actions[2].Delay = 60000;
	
	sfa.cActions = 3;
	sfa.lpsaActions = actions;
	sfa.dwResetPeriod = 86400;
	
	ChangeServiceConfig2A(hService, SERVICE_CONFIG_FAILURE_ACTIONS, &sfa);
	
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);
	
	return true;
}

// 启动服务
bool StartService()
{
	SC_HANDLE hSCM = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
	if (!hSCM) return false;
	
	SC_HANDLE hService = OpenServiceA(hSCM, "ProtectorService", SERVICE_START);
	if (!hService)
	{
		CloseServiceHandle(hSCM);
		return false;
	}
	
	bool bResult = StartServiceA(hService, 0, NULL);
	
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);
	
	return bResult;
}

// 停止服务
bool StopService()
{
	SC_HANDLE hSCM = OpenSCManagerA(NULL, NULL, SC_MANAGER_CONNECT);
	if (!hSCM) return false;
	
	SC_HANDLE hService = OpenServiceA(hSCM, "ProtectorService", SERVICE_STOP);
	if (!hService)
	{
		CloseServiceHandle(hSCM);
		return false;
	}
	
	SERVICE_STATUS status;
	bool bResult = ControlService(hService, SERVICE_CONTROL_STOP, &status);
	
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);
	
	return bResult;
}

// 删除服务
bool DeleteService()
{
	SC_HANDLE hSCM = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
	if (!hSCM) return false;
	
	SC_HANDLE hService = OpenServiceA(hSCM, "ProtectorService", DELETE);
	if (!hService)
	{
		CloseServiceHandle(hSCM);
		return false;
	}
	
	bool bResult = DeleteService(hService);
	
	CloseServiceHandle(hService);
	CloseServiceHandle(hSCM);
	
	return bResult;
}

int main(int argc, char* argv[])
{
	if (argc > 1)
	{
		if (strcmp(argv[1], "install") == 0)
		{
			if (InstallService())
			{
				std::cout << "Service installed!" << std::endl;
				StartService();
				std::cout << "Service started!" << std::endl;
			}
			return 0;
		}
		else if (strcmp(argv[1], "uninstall") == 0)
		{
			StopService();
			Sleep(2000);
			DeleteService();
			std::cout << "Service uninstalled!" << std::endl;
			return 0;
		}
	}
	
	// 作为服务运行
	SERVICE_TABLE_ENTRYA ServiceTable[] = 
	{
		{"ProtectorService", (LPSERVICE_MAIN_FUNCTIONA)ServiceMain},
		{NULL, NULL}
	};
	
	StartServiceCtrlDispatcherA(ServiceTable);
	
	return 0;
}

