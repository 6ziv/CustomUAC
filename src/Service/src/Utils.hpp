#pragma once
#include <Windows.h>
#include <string>
#include <vector>
namespace Utils {
	constexpr std::wstring_view CommandFix = L"Fix";
	constexpr std::wstring_view CommandInstall = L"Install";
	constexpr std::wstring_view CommandSetACL = L"SetACL";
	constexpr std::wstring_view CommandUninstall = L"Uninstall";
	constexpr std::wstring_view CommandEmergency = L"Emergency";
	enum PROCESS_CMD {
		CMD_NONE = 0,
		CMD_FIX,
		CMD_INSTALL,
		CMD_UNINSTALL,
		CMD_EMERGENCY,
		CMD_SET_ACL
	};

	PROCESS_CMD GetRunCmd() {
		std::wstring cmd = GetCommandLineW();
		if (cmd == CommandFix)return CMD_FIX;
		if (cmd == CommandInstall)return CMD_INSTALL;
		if (cmd == CommandUninstall)return CMD_UNINSTALL;
		if (cmd == CommandEmergency)return CMD_EMERGENCY;
		if (cmd == CommandSetACL)return CMD_SET_ACL;
		return CMD_NONE;
	}

	std::wstring GetCurrentProcessPath() {
		std::vector<wchar_t> path;
		DWORD len, retLen;
		len = MAX_PATH;
		do {
			path.resize(len);
			retLen = GetModuleFileNameW(NULL, path.data(), len);
		} while (retLen == len && GetLastError() == ERROR_INSUFFICIENT_BUFFER);
		return path.data();
	}

	std::wstring GetSystemPath() {
		size_t size = GetSystemDirectoryW(NULL, 0);
		std::vector<wchar_t> buffer;
		buffer.resize(size);
		GetSystemDirectoryW(buffer.data(), (UINT)(size));
		std::wstring ret = buffer.data();
		buffer.clear();
		if (ret.back() == L'\\')
			ret.pop_back();
		return ret;
	}
};