#include <Windows.h>
#include <string>
#include <WinReg.hpp>
#include <filesystem>
#include "Privileges.hpp"
#include "Utils.hpp"
#include "FileUtils.hpp"
SERVICE_STATUS ServiceStatus;
SERVICE_STATUS_HANDLE hStatus = NULL;
HANDLE LockedFile = INVALID_HANDLE_VALUE;

VOID WINAPI ControlHandler(DWORD request) {
	switch (request) {
	case SERVICE_CONTROL_STOP:
		if (LockedFile != INVALID_HANDLE_VALUE) {
			CloseHandle(LockedFile);
		}
		ServiceStatus.dwCurrentState = SERVICE_STOPPED;
		SetServiceStatus(hStatus, &ServiceStatus);
	}
	return;
}

VOID WINAPI ServiceMain(DWORD argc, LPSTR* argv) {
	hStatus = RegisterServiceCtrlHandlerA("UACService", ControlHandler);

	ServiceStatus.dwCurrentState = SERVICE_START_PENDING;//SERVICE_RUNNING;
	ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	ServiceStatus.dwControlsAccepted = NULL;
	ServiceStatus.dwWin32ExitCode = NO_ERROR;
	ServiceStatus.dwServiceSpecificExitCode = NO_ERROR;
	ServiceStatus.dwCheckPoint = 0;
	ServiceStatus.dwWaitHint = 5000;//a minute
	SetServiceStatus(hStatus, &ServiceStatus);
	std::filesystem::path uac = std::filesystem::path(Utils::GetCurrentProcessPath()).replace_filename("UACmain.exe");
	if (!Utils::file_is_equal(
			Utils::GetSystemPath() + std::wstring(L"\\consent.exe"),
			uac.wstring(),
			[]() {
				ServiceStatus.dwCheckPoint++;
				SetServiceStatus(hStatus, &ServiceStatus);
			}
		)) 
	{
		HANDLE child = Privileges::CreateProcessAsTrustedInstaller(Utils::CommandFix.data());
		while (DWORD wait = WaitForSingleObject(child, 1000)) {
			ServiceStatus.dwCheckPoint++;
			SetServiceStatus(hStatus, &ServiceStatus);
			if (wait == WAIT_TIMEOUT)continue;
			if (wait == WAIT_OBJECT_0)break;
			ServiceStatus.dwWin32ExitCode = GetLastError();
			ServiceStatus.dwCurrentState = SERVICE_STOPPED;
			SetServiceStatus(hStatus, &ServiceStatus);
		}
	}
	LockedFile = CreateFileW((Utils::GetSystemPath() + std::wstring(L"\\consent.exe")).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(hStatus, &ServiceStatus);
	
}
int wmain(int argc, wchar_t** argv)
{
	if (argc == 2 && std::wstring(argv[1]) == L"--install") {
		HANDLE hChild = Privileges::CreateProcessAsTrustedInstaller(Utils::CommandInstall.data());
		WaitForSingleObject(hChild, INFINITE);
		DWORD ret;
		GetExitCodeProcess(hChild, &ret);
		return ret;
	}
	if (argc == 2 && std::wstring(argv[1]) == L"--uninstall") {
		HANDLE hChild = Privileges::CreateProcessAsTrustedInstaller(Utils::CommandUninstall.data());
		WaitForSingleObject(hChild, INFINITE);
		DWORD ret;
		GetExitCodeProcess(hChild, &ret);
		return ret;
	}
	if (Utils::GetRunCmd() == Utils::CMD_FIX) {
		std::filesystem::path uac = std::filesystem::path(Utils::GetCurrentProcessPath()).replace_filename("UACmain.exe");
		if (CopyFileW(uac.wstring().c_str(), (Utils::GetSystemPath() + std::wstring(L"\\consent.exe")).c_str(), FALSE))return 0;
		if (!CopyFileW(uac.wstring().c_str(), (Utils::GetSystemPath() + std::wstring(L"\\consent.exe.new")).c_str(), FALSE))return 1;
		if (!Utils::RenameFile(Utils::GetSystemPath() + std::wstring(L"\\consent.exe.new"), std::wstring(L"consent.exe")))
			return 1;
		return 0;
	}
	if (Utils::GetRunCmd() == Utils::CMD_INSTALL) {
		std::filesystem::path uac = std::filesystem::path(Utils::GetCurrentProcessPath()).replace_filename("UACmain.exe");
		if (!Utils::AdjustPrivilegeForACL())return 1;
		if (!CopyFileW(uac.wstring().c_str(), (Utils::GetSystemPath() + std::wstring(L"\\consent.exe.new")).c_str(), FALSE))
			return 2;
		if (!Utils::CopyACL(Utils::GetSystemPath() + std::wstring(L"\\consent.exe"), Utils::GetSystemPath() + std::wstring(L"\\consent.exe.new")))
			return 3;
		if (INVALID_HANDLE_VALUE == CreateFileW((Utils::GetSystemPath() + std::wstring(L"\\consent.exe.backup")).c_str(), NULL, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) {
			if (GetLastError() == ERROR_FILE_NOT_FOUND) {
				if (!Utils::RenameFile(Utils::GetSystemPath() + std::wstring(L"\\consent.exe"), std::wstring(L"consent.exe.backup")))
					return 4;
			}
			else return 6;
		}
		if (!Utils::RenameFile(Utils::GetSystemPath() + std::wstring(L"\\consent.exe.new"),
			Utils::GetSystemPath() + std::wstring(L"\\consent.exe")
		))
			return 5;
		return 0;
	}
	if (Utils::GetRunCmd() == Utils::CMD_UNINSTALL) {
		if (!Utils::RenameFile(Utils::GetSystemPath() + std::wstring(L"\\consent.exe"),
			Utils::GetSystemPath() + std::wstring(L"\\consent.exe.new")
		))
			return 1;
		if (!Utils::RenameFile(Utils::GetSystemPath() + std::wstring(L"\\consent.exe.backup"),
			Utils::GetSystemPath() + std::wstring(L"\\consent.exe")
		))
			return 2;
		if (!Utils::RemoveOnReboot(Utils::GetSystemPath() + std::wstring(L"\\consent.exe.new").c_str()))
			return 3;
		return 0;
	}
	SERVICE_TABLE_ENTRYA ServiceTable[2];
	ServiceTable[0].lpServiceName = (char*)(malloc(16));
	if (ServiceTable[0].lpServiceName == NULL)return ERROR_OUTOFMEMORY;
	strcpy_s(ServiceTable[0].lpServiceName, 16, "UACService");
	ServiceTable[0].lpServiceProc = (LPSERVICE_MAIN_FUNCTIONA)ServiceMain;

	ServiceTable[1].lpServiceName = NULL;
	ServiceTable[1].lpServiceProc = NULL;
	StartServiceCtrlDispatcherA(ServiceTable);
	free(ServiceTable[0].lpServiceName);
	return 0;
}