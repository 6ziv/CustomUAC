#include <Windows.h>
#include <sddl.h>
#include <securitybaseapi.h>
#include <string>
#include <functional>
#include <boost/scope_exit.hpp>
#include <boost/range/algorithm/equal.hpp>
namespace Utils {
	bool file_is_equal(const std::wstring& file1, const std::wstring file2, std::function<void(void)> callback) {
		HANDLE hFile1 = CreateFileW(file1.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile1 == INVALID_HANDLE_VALUE)
			return false;
		std::invoke(callback);
		BOOST_SCOPE_EXIT(hFile1) { CloseHandle(hFile1); } BOOST_SCOPE_EXIT_END;

		HANDLE hFile2 = CreateFileW(file2.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile2 == INVALID_HANDLE_VALUE)
			return false;
		std::invoke(callback);
		BOOST_SCOPE_EXIT(hFile2) { CloseHandle(hFile2); } BOOST_SCOPE_EXIT_END;

		LARGE_INTEGER fileSize1;
		if (!GetFileSizeEx(hFile1, &fileSize1))return false;
		std::invoke(callback);

		LARGE_INTEGER fileSize2;
		if (!GetFileSizeEx(hFile2, &fileSize2))return false;
		std::invoke(callback);

		if (fileSize1.QuadPart != fileSize2.QuadPart)return false;

		HANDLE mapping1 = CreateFileMappingW(hFile1, NULL, PAGE_READONLY, 0, 0, NULL);
		if (mapping1 == NULL)
			return false;
		std::invoke(callback);
		BOOST_SCOPE_EXIT(mapping1) { CloseHandle(mapping1); } BOOST_SCOPE_EXIT_END;

		HANDLE mapping2 = CreateFileMappingW(hFile2, NULL, PAGE_READONLY, 0, 0, NULL);
		if (mapping2 == NULL)
			return false;
		std::invoke(callback);
		BOOST_SCOPE_EXIT(mapping2) { CloseHandle(mapping2); } BOOST_SCOPE_EXIT_END;

		unsigned char* content1 = reinterpret_cast<unsigned char*>(MapViewOfFile(mapping1, FILE_MAP_READ, 0, 0, 0));
		if (content1 == NULL)
			return false;
		std::invoke(callback);
		BOOST_SCOPE_EXIT(content1) { UnmapViewOfFile(content1); } BOOST_SCOPE_EXIT_END;

		unsigned char* content2 = reinterpret_cast<unsigned char*>(MapViewOfFile(mapping2, FILE_MAP_READ, 0, 0, 0));
		if (content2 == NULL)
			return false;
		std::invoke(callback);
		BOOST_SCOPE_EXIT(content2) { UnmapViewOfFile(content2); } BOOST_SCOPE_EXIT_END;

		return std::equal(content1, content1 + fileSize1.QuadPart, content2);
	}

	bool RenameFile(const std::wstring& filepath,const std::wstring& newname){
		while (1) {
			if (!MoveFileExW(filepath.c_str(), newname.c_str(), MOVEFILE_REPLACE_EXISTING)) {
				DWORD err = GetLastError();
				if (err == ERROR_SHARING_VIOLATION || err == ERROR_LOCK_VIOLATION) {
					if (IDYES == MessageBoxA(0, "Required file is in use.\n", "Retry?", MB_YESNO))continue;
				}
				else
					return false;
			}
			break;
		}
		return true;
	}
	bool CopyACL(const std::wstring& srcfile, const std::wstring& descfile) {
		DWORD security_len = 0, len2;
		SECURITY_DESCRIPTOR* info = reinterpret_cast<SECURITY_DESCRIPTOR*>(&security_len);
		BOOL t = GetFileSecurityW(srcfile.c_str(), BACKUP_SECURITY_INFORMATION, info, 0, &security_len);
		if (t || (GetLastError() != ERROR_INSUFFICIENT_BUFFER))return false;
		info = reinterpret_cast<SECURITY_DESCRIPTOR*>(malloc(security_len));
		BOOST_SCOPE_EXIT(info) { free(info); }BOOST_SCOPE_EXIT_END;
		t = GetFileSecurityW(srcfile.c_str(), BACKUP_SECURITY_INFORMATION, info, security_len, &len2);
		if ((!t) || (len2 > security_len))return false;
		t = SetFileSecurityW(descfile.c_str(), BACKUP_SECURITY_INFORMATION, info);
		return t;
	}
	bool AdjustPrivilegeForACL() {
		HANDLE hToken;
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &hToken))
			return false;
		BOOST_SCOPE_EXIT(hToken) { CloseHandle(hToken); }BOOST_SCOPE_EXIT_END;
		LUID luid;
		if (!LookupPrivilegeValue(NULL, SE_SECURITY_NAME, &luid))
			return false;
		TOKEN_PRIVILEGES* tp = reinterpret_cast<TOKEN_PRIVILEGES*>(malloc(sizeof(DWORD) + sizeof(LUID_AND_ATTRIBUTES)));
		if (tp == nullptr)
			return false;
		BOOST_SCOPE_EXIT(tp) { free(tp); }BOOST_SCOPE_EXIT_END;
		tp->PrivilegeCount = 1;
		tp->Privileges[0].Luid = luid;
		tp->Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		return AdjustTokenPrivileges(hToken, FALSE, tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
	}

	bool RemoveOnReboot(const std::wstring& path) {
		PSECURITY_DESCRIPTOR pSecurity;
		if (!ConvertStringSecurityDescriptorToSecurityDescriptorA("O:SYG:SYD:(A;;GRGX;;;BU)(A;;GRGX;;;AC)(A;;GA;;;SY)(A;;GA;;;BA)S:", SDDL_REVISION_1, &pSecurity, NULL))return false;
		BOOST_SCOPE_EXIT(pSecurity) { LocalFree(pSecurity); }BOOST_SCOPE_EXIT_END;
		if(!SetFileSecurityW(path.c_str(), ATTRIBUTE_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION, pSecurity))
			return false;
		return MoveFileExW(path.c_str(), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
	}
}