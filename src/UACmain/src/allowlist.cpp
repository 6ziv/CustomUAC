#include "allowlist.h"
#include <unqlite.h>
#include <algorithm>
#include <boost/scope_exit.hpp>
#include <Windows.h>
#include <bcrypt.h>
static char* db_name = nullptr;
const size_t HASH_LEN = 32;
typedef struct {
	uint64_t size;
	uint8_t hash[HASH_LEN];
}value_t;
void set_db(const std::wstring& db){
	int len = WideCharToMultiByte(CP_ACP, 0, db.c_str(), -1, NULL, 0, NULL, NULL);
	if (len < 0)return;
	db_name = new char[static_cast<size_t>(len + 1)];
	int rlen = WideCharToMultiByte(CP_ACP, 0, db.c_str(), -1, db_name, len, NULL, NULL);
	if (rlen == 0)return;
	db_name[len] = '\0';
}
bool calc_hash(const std::wstring& file, value_t& val) {
	BCRYPT_ALG_HANDLE hAlg = NULL;
	BCRYPT_HASH_HANDLE hHash = NULL;
	if (BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0) < 0)
		return false;
	BOOST_SCOPE_EXIT(hAlg) { BCryptCloseAlgorithmProvider(hAlg, 0); } BOOST_SCOPE_EXIT_END;
	HANDLE hFile = CreateFileW(file.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	BOOST_SCOPE_EXIT(hFile) { CloseHandle(hFile); } BOOST_SCOPE_EXIT_END;
	HANDLE mapping = CreateFileMappingW(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	if (mapping == NULL)
		return false;

	BOOST_SCOPE_EXIT(mapping) { CloseHandle(mapping); } BOOST_SCOPE_EXIT_END;
	LPVOID content = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
	if (content == NULL)
		return false;

	BOOST_SCOPE_EXIT(content) { UnmapViewOfFile(content); } BOOST_SCOPE_EXIT_END;

	DWORD cbHashObject = 0;
	DWORD cbData = 0;
	PBYTE pbHashObject = NULL;
	if (BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0) < 0)
		return false;

	pbHashObject = (PBYTE)HeapAlloc(GetProcessHeap(), 0, cbHashObject);
	if (pbHashObject == NULL)
		return false;

	BOOST_SCOPE_EXIT(pbHashObject) { HeapFree(GetProcessHeap(), 0, pbHashObject); } BOOST_SCOPE_EXIT_END;
	if (BCryptCreateHash(hAlg, &hHash, pbHashObject, cbHashObject, NULL, 0, 0) < 0)
		return false;

	BOOST_SCOPE_EXIT(pbHashObject) { BCryptDestroyHash(pbHashObject); } BOOST_SCOPE_EXIT_END;

	LARGE_INTEGER fileSize;
	if (!GetFileSizeEx(hFile, &fileSize))return false;

	PBYTE current_content = (PBYTE)content;
	BOOL flag = FALSE;
	for (LONG i = 0; i < fileSize.HighPart; i++) {

		if (BCryptHashData(hHash, (PBYTE)current_content, 2147483648, 0) < 0) {
			flag = TRUE;
			break;
		}
		current_content += 2147483648ull;

		if (BCryptHashData(hHash, (PBYTE)current_content, 2147483648, 0) < 0) {
			flag = TRUE;
			break;
		}
		current_content += 2147483648ull;
	}

	if (flag)
		return false;
	if (fileSize.LowPart > 0) {
		if (BCryptHashData(hHash, (PBYTE)current_content, fileSize.LowPart, 0) < 0)
			return false;
		current_content += fileSize.LowPart;
	}

	DWORD cbHash = 0;

	if (BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbData, 0) < 0)
		return false;
	if (cbHash != HASH_LEN)return false;

	if (BCryptFinishHash(hHash, val.hash, HASH_LEN, 0) < 0)
		return false;
	val.size = fileSize.QuadPart;
	return true;
}
bool check_allowlist(const std::wstring& file) {
	if (db_name == nullptr)return false;
    value_t val;
	{
		unqlite* db;
        int r = unqlite_open(&db, db_name, UNQLITE_OPEN_READONLY);
		if (r != UNQLITE_OK)return false;
        BOOST_SCOPE_EXIT(db) { unqlite_close(db);}BOOST_SCOPE_EXIT_END;
		
		unqlite_int64 val_length = sizeof(value_t);
		r = unqlite_kv_fetch(db, file.c_str(), static_cast<int>(file.length() * sizeof(wchar_t)), &val, &val_length);
		if (r != UNQLITE_OK)return false;
		if (val_length != sizeof(value_t))return false;
	}
	value_t val2;
    if (!calc_hash(file, val2))return false;
    bool is_allowlisted = (val.size == val2.size) && (std::equal(val.hash, val.hash + HASH_LEN, val2.hash));
	if (!is_allowlisted) {
		//remove allowlist item.
		unqlite* db;
		int r = unqlite_open(&db, db_name, UNQLITE_OPEN_READWRITE);
		if (r != UNQLITE_OK)return false;
        BOOST_SCOPE_EXIT(db) { unqlite_close(db); }BOOST_SCOPE_EXIT_END;
		r = unqlite_kv_delete(db, file.c_str(), static_cast<int>(file.length() * sizeof(wchar_t)));
		if (r != UNQLITE_OK)return false;
	}
	return is_allowlisted;
}
bool add_to_allowlist(const UACData& data) {
	if (db_name == nullptr)return false;
	//remove allowlist item.
	unqlite* db;
	int r = unqlite_open(&db, db_name, UNQLITE_OPEN_CREATE);
	if (r != UNQLITE_OK) {
		return false;
	}
	BOOST_SCOPE_EXIT(db) { unqlite_close(db); }BOOST_SCOPE_EXIT_END;
	
	std::wstring file = data.path().toStdWString();
	value_t val;
	calc_hash(file, val);
	r = unqlite_kv_store(db, data.path().toStdWString().c_str(), static_cast<int>(data.path().toStdWString().length() * sizeof(wchar_t)), &val, sizeof(val));
	if (r != UNQLITE_OK)return false;
	return true;
}
