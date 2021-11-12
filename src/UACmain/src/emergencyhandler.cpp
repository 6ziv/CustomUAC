#include "emergencyhandler.h"
#include "securedesktop.h"
#include "../../Service/src/Privileges.hpp"
#include <Windows.h>
/* Dirty code.
 * When error occurs, first try to use default theme.
 * If it does not work, use a "safe environment"(which means do not load settings, switch desktop, or use opengl.)
 * And if it still fails, the last thing to do is to rename the backup of the system UAC back.
 * If things goes worse, I guess we should depend on users to fix it manually.
 */
int error_level = 0;
std::string arguments;
void run_TI() {
	do {
		SC_HANDLE hSC = OpenSCManager(NULL, SERVICES_ACTIVE_DATABASE, SC_MANAGER_CONNECT);
		if (hSC == NULL) break;
		BOOST_SCOPE_EXIT(hSC) { CloseServiceHandle(hSC); }BOOST_SCOPE_EXIT_END;
		SC_HANDLE hSvc = OpenServiceW(hSC, L"UACService", DELETE);
		if (hSvc == NULL)break;
		BOOST_SCOPE_EXIT(hSvc) { CloseServiceHandle(hSvc); }BOOST_SCOPE_EXIT_END;
		DeleteService(hSvc);
	} while (0);

	HANDLE hFile1 = CreateFileA("consent.exe", DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	HANDLE hFile2 = CreateFileA("consent.exe.backup", DELETE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile1 == INVALID_HANDLE_VALUE || hFile2 == INVALID_HANDLE_VALUE)ExitProcess(1223);
	FILE_RENAME_INFO* info = reinterpret_cast<FILE_RENAME_INFO*>(malloc(sizeof(FILE_RENAME_INFO) + 19 * sizeof(wchar_t)));
	info->ReplaceIfExists = true;
	info->RootDirectory = NULL;
	info->FileNameLength = 18;
	wcscpy(info->FileName, L"consent.exe.broken");
	if (!SetFileInformationByHandle(hFile1, FileRenameInfo, info, sizeof(FILE_RENAME_INFO) + 18 * sizeof(wchar_t)))ExitProcess(1223);

	info->ReplaceIfExists = true;
	info->RootDirectory = NULL;
	info->FileNameLength = 11;
	wcscpy(info->FileName, L"consent.exe");
	if (!SetFileInformationByHandle(hFile2, FileRenameInfo, info, sizeof(FILE_RENAME_INFO) + 11 * sizeof(wchar_t)))ExitProcess(1223);
	ExitProcess(1223);
}
void try_restore() {
	MessageBoxA(0, "Fatal Error.\nCannot even run default theme in safe environment.\nTrying to roll back to system UAC.\n Press OK to continue.", "Fatal Error", MB_OK);
	Privileges::CreateProcessAsTrustedInstaller(Utils::CommandEmergency.data());
}
void parse_args(int argc, char** argv) {
	if (Utils::GetRunCmd() == Utils::CMD_EMERGENCY) {
		run_TI();
		ExitProcess(1223);
	}
	arguments = std::string("consent.exe") + " " + argv[1] + " " + argv[2] + " " + argv[3];
	if (argc == 5 && strcmp(argv[4], "use-default") == 0) {
		error_level = 1;
	}
	if (argc == 5 && strcmp(argv[4], "safe-environment") == 0) {
		error_level = 2;
	}
	if (argc == 5 && strcmp(argv[4], "restore") == 0) {
		error_level = 3;
		try_restore();
		ExitProcess(1223);
	}
}
void error_handler() {
	std::string attached_argument;
	switch (error_level)
	{
	case 0:attached_argument = "use-default"; break;
	case 1:attached_argument = "safe-environment"; break;
	case 2:attached_argument = "restore"; break;
	}
	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(STARTUPINFOA));
	si.cb = sizeof(STARTUPINFOA);
	si.lpDesktop = new char[1];
	si.lpDesktop[0] = '\0';
	ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));
	std::string cmd = arguments + " " + attached_argument;
	if (!CreateProcessA("consent.exe", cmd.data(), NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, &si, &pi))return;
	CloseHandle(pi.hThread);
	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD ret;
	if (!GetExitCodeProcess(pi.hProcess, &ret))return;
	CloseHandle(pi.hProcess);
	ExitProcess(ret);
}
DWORD WINAPI ErrorHandler(LPVOID) {
	error_handler();
	ExitProcess(1223);
}
LONG WINAPI ExceptionHandler(PEXCEPTION_POINTERS exception)
{
	if (!SecureDesktop::instance().has_switched()) {
		ErrorHandler(NULL);
	}
	else {
		SecureDesktop::instance().switch_back();
		HANDLE hThread = CreateThread(NULL, 0, ErrorHandler, NULL, CREATE_SUSPENDED, NULL);
		SetThreadDesktop(SecureDesktop::instance().get_saved());
		ResumeThread(hThread);
		WaitForSingleObject(hThread, INFINITE);
	}
	return EXCEPTION_EXECUTE_HANDLER;
}
