#pragma once
#include <Windows.h>
#include <WtsApi32.h>
#include <TlHelp32.h>
#include <Psapi.h>
#include <string>
#include <boost/scope_exit.hpp>
#include "Utils.hpp"
namespace Privileges {
#pragma comment(lib,"Wtsapi32.lib")
#define GUARD_HANDLE(hWnd) BOOST_SCOPE_EXIT(hWnd){CloseHandle(hWnd);} BOOST_SCOPE_EXIT_END
#define GUARD_SC_HANDLE(hWnd) BOOST_SCOPE_EXIT(hWnd){CloseServiceHandle(hWnd);} BOOST_SCOPE_EXIT_END
#define GUARD_MALLOC(mem) BOOST_SCOPE_EXIT(mem){free(mem);} BOOST_SCOPE_EXIT_END

	void InitSecurityAttributes(SECURITY_ATTRIBUTES& security) {
		security.nLength = sizeof(SECURITY_ATTRIBUTES);
		security.lpSecurityDescriptor = NULL;
		security.bInheritHandle = FALSE;
	}

	void InitSPInformation(STARTUPINFOW& si, PROCESS_INFORMATION& pi) {
		ZeroMemory(&si, sizeof(STARTUPINFOW));
		si.cb = sizeof(STARTUPINFOW);
		si.lpDesktop = new wchar_t[1];
		si.lpDesktop[0] = L'\0';
		ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	}
#define INIT_GUARDED_SP_INFORMATION(si,pi) \
	InitSPInformation(si,pi);\
	BOOST_SCOPE_EXIT(si){\
	delete[] si.lpDesktop;\
	}\
	BOOST_SCOPE_EXIT_END

	HANDLE DuplicateTokenAndRun(HANDLE hToken, std::wstring command) {
		SECURITY_ATTRIBUTES tokenAttributes;
		InitSecurityAttributes(tokenAttributes);
		HANDLE hDupToken;
		if (!DuplicateTokenEx(hToken, MAXIMUM_ALLOWED, &tokenAttributes, SecurityImpersonation, TokenPrimary, &hDupToken))
			return NULL;
		GUARD_HANDLE(hDupToken);

		std::wstring cmd1 = command;
		std::wstring current_process = Utils::GetCurrentProcessPath();
		STARTUPINFOW si;
		PROCESS_INFORMATION pi;
		INIT_GUARDED_SP_INFORMATION(si, pi);
		if (CreateProcessWithTokenW(hDupToken, 0, current_process.c_str(), cmd1.data(), 0, NULL, NULL, &si, &pi)) {
			CloseHandle(pi.hThread);
			return pi.hProcess;
		}
		return NULL;
	}

	DWORD GetProcessIdByName(const wchar_t* processName)
	{
		HANDLE hSnapshot;
		if ((hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)) == INVALID_HANDLE_VALUE)
			return 0;
		DWORD pid = 0;
		PROCESSENTRY32W pe;
		ZeroMemory(&pe, sizeof(PROCESSENTRY32W));
		pe.dwSize = sizeof(PROCESSENTRY32W);
		if (Process32FirstW(hSnapshot, &pe)) {
			while (Process32NextW(hSnapshot, &pe)) {
				if (0 == wcscmp(pe.szExeFile, processName))
				{
					pid = pe.th32ProcessID;
					break;
				}
			}
		}
		CloseHandle(hSnapshot);
		return pid;
	}
	bool ImpersonateSystem()
	{
		DWORD systemPid = GetProcessIdByName(L"winlogon.exe");
		if (0 == systemPid)return 0;
		HANDLE hSystemProcess = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, systemPid);
		if (hSystemProcess == NULL)
			return false;
		GUARD_HANDLE(hSystemProcess);
		HANDLE hSystemToken;
		if (!OpenProcessToken(hSystemProcess, MAXIMUM_ALLOWED, &hSystemToken))
			return false;

		GUARD_HANDLE(hSystemToken);
		HANDLE hDupToken;
		SECURITY_ATTRIBUTES tokenAttributes;
		tokenAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
		tokenAttributes.lpSecurityDescriptor = NULL;
		tokenAttributes.bInheritHandle = FALSE;
		if (!DuplicateTokenEx(hSystemToken, MAXIMUM_ALLOWED, &tokenAttributes, SecurityImpersonation, TokenImpersonation, &hDupToken))
			return false;
		GUARD_HANDLE(hDupToken);
		return ImpersonateLoggedOnUser(hDupToken);
	}
	DWORD StartTrustedInstallerService()
	{
		SC_HANDLE hSCManager;
		if ((hSCManager = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, GENERIC_EXECUTE)) == NULL)
			return 0;

		GUARD_SC_HANDLE(hSCManager);
		SC_HANDLE hService;
		if ((hService = OpenServiceW(hSCManager, L"TrustedInstaller", GENERIC_READ | GENERIC_EXECUTE)) == NULL)
			return 0;
		GUARD_SC_HANDLE(hService);
		SERVICE_STATUS_PROCESS statusBuffer;
		DWORD bytesNeeded;

		while (QueryServiceStatusEx(hService, SC_STATUS_PROCESS_INFO, (LPBYTE)(&statusBuffer), sizeof(SERVICE_STATUS_PROCESS), &bytesNeeded))
		{
			if (statusBuffer.dwCurrentState == SERVICE_STOPPED)
				if (!StartServiceW(hService, 0, NULL))
					return 0;
			if (statusBuffer.dwCurrentState == SERVICE_START_PENDING || statusBuffer.dwCurrentState == SERVICE_STOP_PENDING)
				Sleep(statusBuffer.dwWaitHint);
			if (statusBuffer.dwCurrentState == SERVICE_RUNNING)
			{
				return statusBuffer.dwProcessId;
			}
		}
		return 0;
	}
	HANDLE CreateProcessAsTrustedInstaller(const std::wstring& command)
	{
		HANDLE hToken;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
			return NULL;
		{
			GUARD_HANDLE(hToken);
			LUID luid0, luid1;
			if ((!LookupPrivilegeValue(NULL, SE_DEBUG_NAME, &luid0)) || (!LookupPrivilegeValue(NULL, SE_IMPERSONATE_NAME, &luid1)))
				return NULL;
			TOKEN_PRIVILEGES* tp = reinterpret_cast<TOKEN_PRIVILEGES*>(malloc(sizeof(DWORD) + 2 * sizeof(LUID_AND_ATTRIBUTES)));
			if (tp == NULL)
				return NULL;
			GUARD_MALLOC(tp);
			tp->PrivilegeCount = 2;
			tp->Privileges[0].Luid = luid0;
			tp->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			tp->Privileges[1].Luid = luid1;
			tp->Privileges[1].Attributes = SE_PRIVILEGE_ENABLED;
			if (!AdjustTokenPrivileges(hToken, FALSE, tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
				return NULL;
		}
		if (!ImpersonateSystem())
			return NULL;
		DWORD pid = StartTrustedInstallerService();
		if (pid == 0)
			return NULL;
		HANDLE hTIProcess, hTIToken;
		if ((hTIProcess = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_QUERY_INFORMATION, FALSE, pid)) == NULL)
			return NULL;
		{
			GUARD_HANDLE(hTIProcess);
			if (!OpenProcessToken(hTIProcess, MAXIMUM_ALLOWED, &hTIToken))
				return NULL;
		}
		GUARD_HANDLE(hTIToken);
		return DuplicateTokenAndRun(hTIToken, command);
	}
#undef GUARD_HANDLE
#undef GUARD_SC_HANDLE
#undef GUARD_MALLOC
#undef INIT_GUARDED_SP_INFORMATION
}