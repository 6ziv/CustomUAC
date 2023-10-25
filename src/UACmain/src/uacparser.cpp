#define _CRT_SECURE_NO_WARNINGS
#include <ntstatus.h>
#define WIN32_NO_STATUS
#include "uacparser.h"
#include <winver.h>
#include <strsafe.h>
#include <wincrypt.h>
#include <SoftPub.h>
#include <NTSecAPI.h>
#include <WtsApi32.h>
#include <LMaccess.h>
#include <LM.h>
#include <boost/scope_exit.hpp>
#include "securedesktop.h"
#include <QApplication>
void initLSAString(LSA_STRING& str, const char* content) {
	size_t l = strlen(content);
	str.Length = static_cast<USHORT>(l);
	str.MaximumLength = static_cast<USHORT>(l + 1);
	str.Buffer = new char[l + 1];
	strcpy(str.Buffer, content);
}
void freeLSAString(LSA_STRING& str) {
	delete[] str.Buffer;
}
void* UACParser::readUAC(int argc, char** argv, size_t& retlen) {
	if (argc != 4)return nullptr;
	DWORD pid = strtoul(argv[1], nullptr, 10);
	DWORD len = strtoul(argv[2], nullptr, 10);
	PVOID address = reinterpret_cast<PVOID>(static_cast<uintptr_t>(strtoull(argv[3], nullptr, 16)));
	if (pid == 0 || len == 0 || address == nullptr)return nullptr;
	hProcess = OpenProcess(PROCESS_DUP_HANDLE | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION, FALSE, pid);
	if (hProcess == NULL)return nullptr;
	void* memory = malloc(len);
	if (memory == nullptr) {
		CloseHandle(hProcess);
		return nullptr;
	}
	SIZE_T tmp;
	if (!ReadProcessMemory(hProcess, address, memory, len, &tmp) || (tmp != len)) {
		free(memory);
		CloseHandle(hProcess);
		return nullptr;
	}
	retlen = len;
	return memory;
}
void UACParser::tryGetFileDescription() {
	DWORD len, tmp;
	len = GetFileVersionInfoSizeExW(FILE_VER_GET_LOCALISED, path.c_str(), &tmp);
	if (len == 0)return;
	void* info = malloc(len);
	if (!GetFileVersionInfoExW(FILE_VER_GET_LOCALISED, path.c_str(), NULL, len, info)) {
		free(info);
		return;
	}
	DWORD* lpTranslate;
	UINT value_len;
	VerQueryValueW(info, L"\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &value_len);
	if (value_len < sizeof(DWORD)) {
		free(info);
		return;
	}
	wchar_t* description_str;
	wchar_t arg[64];
	if (LOWORD(lpTranslate[0]) == 0x0000)lpTranslate[0] |= 0x0906;// assume english for neutral.
	HRESULT hr = StringCchPrintfW(arg, 256, L"\\StringFileInfo\\%04x%04x\\FileDescription", LOWORD(lpTranslate[0]), HIWORD(lpTranslate[0]));
	if (FAILED(hr)) {
		free(info);
		return;
	}
	if (!VerQueryValueW(info, arg, (LPVOID*)&description_str, &value_len)) {
		free(info);
		return;
	}
	file_description = std::wstring(description_str, value_len);
	free(info);
}
void UACParser::tryGetFileSigner() {
	GUID GenericActionId = WINTRUST_ACTION_GENERIC_VERIFY_V2;

	WINTRUST_DATA tData;
	memset(&tData, 0, sizeof(WINTRUST_DATA));
	tData.cbStruct = sizeof(WINTRUST_DATA);
	tData.pPolicyCallbackData = NULL;
	tData.pSIPClientData = NULL;
	tData.dwUIChoice = WTD_UI_NONE;
	tData.fdwRevocationChecks = WTD_REVOKE_NONE;
	tData.dwUnionChoice = WTD_CHOICE_FILE;

	WINTRUST_FILE_INFO fInfo;
	fInfo.cbStruct = sizeof(WINTRUST_FILE_INFO_);
	fInfo.hFile = NULL;
	fInfo.pcwszFilePath = path.c_str();
	fInfo.pgKnownSubject = NULL;

	tData.pFile = &fInfo;
	tData.dwStateAction = WTD_STATEACTION_VERIFY;
	tData.hWVTStateData = NULL;
	tData.pwszURLReference = NULL;
	tData.dwProvFlags = WTD_DISABLE_MD2_MD4 | WTD_LIFETIME_SIGNING_FLAG | WTD_REVOCATION_CHECK_NONE;
	tData.dwUIContext = 0;

	WINTRUST_SIGNATURE_SETTINGS SignSettings;
	SignSettings.cbStruct = sizeof(WINTRUST_SIGNATURE_SETTINGS);
	SignSettings.dwFlags = WSS_VERIFY_SPECIFIC;
	SignSettings.dwIndex = 0;
	SignSettings.dwVerifiedSigIndex = 0;
	SignSettings.cSecondarySigs = NULL;

	tData.pSignatureSettings = NULL;

	if (ERROR_SUCCESS != WinVerifyTrust(NULL, &GenericActionId, &tData))return;
	tData.dwStateAction = WTD_STATEACTION_CLOSE;
	WinVerifyTrust(NULL, &GenericActionId, &tData);
	HCERTSTORE Store;
	HCRYPTMSG  CMsg;

	if (!CryptQueryObject(CERT_QUERY_OBJECT_FILE, path.c_str(), CERT_QUERY_CONTENT_FLAG_PKCS7_SIGNED_EMBED,
		CERT_QUERY_FORMAT_FLAG_BINARY, 0, NULL, NULL, NULL, &Store, &CMsg, NULL
	))
		return;
	DWORD Size = 0;
	CryptMsgGetParam(CMsg, CMSG_SIGNER_INFO_PARAM, 0, NULL, &Size);
	CMSG_SIGNER_INFO* Signer = reinterpret_cast<CMSG_SIGNER_INFO*>(malloc(Size));
	if (Signer == nullptr)return;
	CryptMsgGetParam(CMsg, CMSG_SIGNER_INFO_PARAM, 0, Signer, &Size);
	CERT_INFO Cert;
	Cert.Issuer = Signer->Issuer;
	Cert.SerialNumber = Signer->SerialNumber;
	PCCERT_CONTEXT Context;
	Context = CertFindCertificateInStore(Store, X509_ASN_ENCODING | PKCS_7_ASN_ENCODING, 0, CERT_FIND_SUBJECT_CERT, (PVOID)&Cert, NULL);
	if (Context == NULL) {
		free(Signer);
		return;
	}
	DWORD nameSize;
	DWORD nameSize2;
	nameSize = CertGetNameStringW(Context, CERT_NAME_FRIENDLY_DISPLAY_TYPE, 0, NULL, NULL, 0);
	signer.resize(nameSize);
	nameSize2 = CertGetNameStringW(Context, CERT_NAME_SIMPLE_DISPLAY_TYPE, 0, NULL, signer.data(), nameSize);
	if (nameSize2 != nameSize)signer.clear();
	free(Signer);
	return;

}
void UACParser::get_administrators() {
	PBYTE buffer = 0;
	DWORD entries_count = 0;
	DWORD total_entries = 0;
	DWORD resume = 0;
	NET_API_STATUS status = NERR_Success;
	do {
		status = NetUserEnum(NULL, 1, NULL, &buffer, MAX_PREFERRED_LENGTH, &entries_count, &total_entries, &resume);
		if ((status != NERR_Success) && (status != ERROR_MORE_DATA))
			return;
		if (!buffer)return;
		BOOST_SCOPE_EXIT(buffer) { NetApiBufferFree(buffer); }BOOST_SCOPE_EXIT_END;
		USER_INFO_1* ptr = reinterpret_cast<USER_INFO_1*>(buffer);
		for (size_t i = 0; i < entries_count; i++) {
			if (ptr[i].usri1_priv == USER_PRIV_ADMIN) {
				if ((ptr[i].usri1_flags & (UF_ACCOUNTDISABLE | UF_PASSWORD_EXPIRED)) == 0)
					this->local_administrators.emplace_back(ptr[i].usri1_name);
			}
		}
	} while (status == ERROR_MORE_DATA);
}
bool UACParser::parseUAC(void* uac, size_t len) {
	unsigned char* buffer = reinterpret_cast<unsigned char*>(uac);
	if (len < 6 * sizeof(DWORD) + 4 * sizeof(void*))return false;
	UINT32& flag1 = *reinterpret_cast<UINT32*>(buffer);
	if (flag1 != len)return false;
	uac_type = *reinterpret_cast<UINT32*>(buffer + sizeof(UINT32));
	HANDLE& old_foreign_handle = *reinterpret_cast<HANDLE*>(buffer + 2 * sizeof(UINT32) + 2 * sizeof(void*));
	HANDLE duplicated_handle;
	if (!DuplicateHandle(hProcess, old_foreign_handle, GetCurrentProcess(), &duplicated_handle, NULL, FALSE, DUPLICATE_SAME_ACCESS))
		return false;
	DWORD retLen;
	TOKEN_ELEVATION_TYPE elevation_type;
	if (!GetTokenInformation(duplicated_handle, TokenElevationType, &elevation_type, sizeof(TOKEN_ELEVATION_TYPE), &retLen))return false;
	if (retLen != sizeof(TOKEN_ELEVATION_TYPE))return false;
	if (elevation_type == TokenElevationTypeLimited) {
		TOKEN_LINKED_TOKEN linked_token;
		if (!GetTokenInformation(duplicated_handle, TokenLinkedToken, &linked_token, sizeof(TOKEN_LINKED_TOKEN), &retLen) || (retLen != sizeof(TOKEN_LINKED_TOKEN))) {
			CloseHandle(duplicated_handle);
			return false;
		}
		if (!DuplicateHandle(GetCurrentProcess(), linked_token.LinkedToken, hProcess, &access_handle, NULL, FALSE, DUPLICATE_SAME_ACCESS)) {
			CloseHandle(linked_token.LinkedToken);
			CloseHandle(duplicated_handle);
			return false;
		}
		CloseHandle(linked_token.LinkedToken);
		is_admin = true;
	}
	else {
		is_admin = false;
		if(TokenElevationTypeFull==elevation_type){
			is_admin = true;
			if (!DuplicateHandle(GetCurrentProcess(), duplicated_handle, hProcess, &access_handle, NULL, FALSE, DUPLICATE_SAME_ACCESS)) {
				CloseHandle(duplicated_handle);
				return false;
			}
		}
	}
	CloseHandle(duplicated_handle);

	unsigned char* buf_write_target = buffer + 6 * sizeof(UINT32) + 4 * sizeof(void*);
	/* 
		At this address, the new consent data stores the base64 of a UUID (24 characters) padded to 129 bytes, and then left 7 bytes margin at the tail. 
		So there should be a long all-zero area if we skip the first 24 bytes.
		We can make a guess.
	*/
	bool is_new_type = true;
	
	for(size_t i=24;i<129;i++){
		
		if(buf_write_target[i]!=0){
			is_new_type = false;
			break;
		}
	}
	int new_type_offset = is_new_type?136:0;
	
	write_target = *reinterpret_cast<void**>(buffer + 6 * sizeof(UINT32) + 4 * sizeof(void*) + new_type_offset);
	
	size_t base_offset = 0;
	if (uac_type == 0 || uac_type ==2) {//Known to be excutable
	//uac_type==2 means msi. which is similar to exe except that content meanings have changed.
		base_offset = 6 * sizeof(DWORD) + 6 * sizeof(void*) + new_type_offset;
	}
	else if (uac_type == 1) {//dll
		base_offset = 6 * sizeof(DWORD) + 5 * sizeof(void*) + new_type_offset;
	}
	else {
		//see if any of these fits.
		do
		{
			{
				base_offset = 6 * sizeof(DWORD) + 6 * sizeof(void*) + new_type_offset;
				uintptr_t description_offset = *reinterpret_cast<uintptr_t*>(buffer + base_offset);
				uintptr_t path_offset = *reinterpret_cast<uintptr_t*>(buffer + base_offset + sizeof(uintptr_t));
				uintptr_t parameters_offset = *reinterpret_cast<uintptr_t*>(buffer + base_offset + 2 * sizeof(uintptr_t));
				uintptr_t end_offset = *reinterpret_cast<uintptr_t*>(buffer + base_offset + 3 * sizeof(uintptr_t));
				if(
					base_offset + 4 * sizeof(uintptr_t) <= description_offset && 
					description_offset < path_offset && 
					path_offset < parameters_offset &&
					parameters_offset < end_offset &&
					end_offset <= len
				)break;
			}
			{
				base_offset = 6 * sizeof(DWORD) + 5 * sizeof(void*) + new_type_offset;
				uintptr_t description_offset = *reinterpret_cast<uintptr_t*>(buffer + base_offset);
				uintptr_t path_offset = *reinterpret_cast<uintptr_t*>(buffer + base_offset + sizeof(uintptr_t));
				uintptr_t parameters_offset = *reinterpret_cast<uintptr_t*>(buffer + base_offset + 2 * sizeof(uintptr_t));
				uintptr_t end_offset = *reinterpret_cast<uintptr_t*>(buffer + base_offset + 3 * sizeof(uintptr_t));
				if(
					base_offset + 4 * sizeof(uintptr_t) <= description_offset && 
					description_offset < path_offset && 
					path_offset < parameters_offset &&
					parameters_offset < end_offset &&
					end_offset <= len
				)break;
			}
			return false;
		}while(0);
	}
	uintptr_t description_offset = *reinterpret_cast<uintptr_t*>(buffer + base_offset);
	uintptr_t path_offset = *reinterpret_cast<uintptr_t*>(buffer + base_offset + sizeof(uintptr_t));
	uintptr_t parameters_offset = *reinterpret_cast<uintptr_t*>(buffer + base_offset + 2 * sizeof(uintptr_t));
	uintptr_t end_offset = *reinterpret_cast<uintptr_t*>(buffer + base_offset + 3 * sizeof(uintptr_t));
	description.assign(reinterpret_cast<wchar_t*>(buffer + description_offset), (path_offset - description_offset) / sizeof(wchar_t) - 1);
	path.assign(reinterpret_cast<wchar_t*>(buffer + path_offset), (parameters_offset - path_offset) / sizeof(wchar_t) - 1);
	size_t current_offset = parameters_offset;
	while (current_offset < end_offset) {
		size_t tmp_len = wcsnlen(reinterpret_cast<wchar_t*>(buffer + current_offset), end_offset - current_offset);
		if (tmp_len == 0)break;
		parameters.emplace_back(reinterpret_cast<wchar_t*>(buffer + current_offset), tmp_len);
		current_offset += (tmp_len + 1) * sizeof(wchar_t);
	}
	file_description.clear();
	tryGetFileDescription();
	tryGetFileSigner();
	get_administrators();
	return true;
}
UACParser::UACParser(int argc, char** argv)
{
	is_ok = false;
	size_t len = 0;
	void* uac = readUAC(argc, argv, len);
	if (uac == nullptr)return;
	if (!parseUAC(uac, len))return;
	free(uac);
	is_ok = true;
}
UACParser::~UACParser() {
	if (hProcess != NULL && hProcess != INVALID_HANDLE_VALUE)
		CloseHandle(hProcess);
}
void UACParser::accept_admin() {
	SIZE_T ret_len;
	WriteProcessMemory(hProcess, write_target, reinterpret_cast<PVOID>(&access_handle), sizeof(HANDLE), &ret_len);
	SecureDesktop::instance().switch_back();
    ExitProcess(0);
}
void UACParser::reject() {
	SecureDesktop::instance().switch_back();

    ExitProcess(1223);
}
int UACParser::accept_nonadmin(std::wstring user, std::wstring pass) {
	HANDLE process_token;
	BOOL r = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &process_token);
	if (!r)return -1;

	LUID luid;
	r = LookupPrivilegeValue(NULL, SE_TCB_NAME, &luid);
	if (!r)return -1;
	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	r = AdjustTokenPrivileges(process_token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
	if (!r)return -1;


	DWORD session = WTSGetActiveConsoleSessionId();
	if (session == 0xFFFFFFFF)return -1;
	HANDLE current_user_token;
	r = WTSQueryUserToken(session, &current_user_token);
	if (!r)return -1;

	DWORD required_length, ret_length;
	GetTokenInformation(current_user_token, TokenLogonSid, NULL, 0, &required_length);
	TOKEN_GROUPS* logon = reinterpret_cast<TOKEN_GROUPS*>(malloc(required_length));
	r = GetTokenInformation(current_user_token, TokenLogonSid, logon, required_length, &ret_length);
	CloseHandle(current_user_token);
	if (!r)return -1;
	LSA_STRING str;
	LSA_HANDLE hLSA;
	LSA_OPERATIONAL_MODE mode;
	initLSAString(str, "UACLogon");
	HRESULT hr = LsaRegisterLogonProcess(&str, &hLSA, &mode);
	freeLSAString(str);
	if (hr != STATUS_SUCCESS)return -1;
	ULONG ap;
	initLSAString(str, MSV1_0_PACKAGE_NAME);
	hr = LsaLookupAuthenticationPackage(hLSA, &str, &ap);
	freeLSAString(str);
	if (hr != STATUS_SUCCESS) {
		LsaDeregisterLogonProcess(hLSA);
		return -1;
	}
	initLSAString(str, "UACLogon");
	size_t required_len = sizeof(MSV1_0_INTERACTIVE_LOGON) + (user.length() + 1 + pass.length() + 1 + 2) * sizeof(wchar_t);
	//Do not consider domain: because I do not know much about it.
	MSV1_0_INTERACTIVE_LOGON* logon_data = reinterpret_cast<MSV1_0_INTERACTIVE_LOGON*>(malloc(required_len));

	logon_data->MessageType = MsV1_0InteractiveLogon;
	logon_data->LogonDomainName.MaximumLength = 2 * sizeof(wchar_t);
	logon_data->LogonDomainName.Length = 1 * sizeof(wchar_t);
	logon_data->LogonDomainName.Buffer = reinterpret_cast<wchar_t*>(
		reinterpret_cast<unsigned char*>(logon_data) + sizeof(MSV1_0_INTERACTIVE_LOGON));
	wcscpy(logon_data->LogonDomainName.Buffer, L".");
	logon_data->UserName.MaximumLength = static_cast<USHORT>((user.length() + 1) * sizeof(wchar_t));
	logon_data->UserName.Length = static_cast<USHORT>(user.length() * sizeof(wchar_t));
	logon_data->UserName.Buffer = logon_data->LogonDomainName.Buffer + 2;
	wcscpy(logon_data->UserName.Buffer, user.c_str());
	logon_data->Password.MaximumLength = static_cast<USHORT>((pass.length() + 1) * sizeof(wchar_t));
	logon_data->Password.Length = static_cast<USHORT>(pass.length() * sizeof(wchar_t));
	logon_data->Password.Buffer = logon_data->UserName.Buffer + user.length() + 1;
	wcscpy(logon_data->Password.Buffer, pass.c_str());

	TOKEN_SOURCE token_source;
	memcpy(token_source.SourceName, "UACLogon", 8);
	token_source.SourceIdentifier.HighPart = NULL;
	token_source.SourceIdentifier.LowPart = NULL;

	MSV1_0_INTERACTIVE_PROFILE* profile;
	ULONG profile_length;
	LUID logon_session;
	HANDLE hToken;
	QUOTA_LIMITS quota_limits;
	NTSTATUS substatus;
	hr = LsaLogonUser(hLSA, &str, Interactive, ap, logon_data, static_cast<ULONG>(required_len), logon, &token_source, reinterpret_cast<PVOID*>(&profile), &profile_length, &logon_session, &hToken, &quota_limits, &substatus);
	free(logon_data);
	LsaDeregisterLogonProcess(hLSA);
	if (hr != STATUS_SUCCESS)return 0;
	LsaFreeReturnBuffer(profile);

	TOKEN_LINKED_TOKEN linked_token;
	if (!GetTokenInformation(hToken, TokenLinkedToken, &linked_token, sizeof(TOKEN_LINKED_TOKEN), &ret_length) || (ret_length != sizeof(TOKEN_LINKED_TOKEN))) {
		CloseHandle(hToken);
		return false;
	}
	CloseHandle(hToken);

	DuplicateHandle(GetCurrentProcess(), linked_token.LinkedToken, hProcess, &access_handle, NULL, FALSE, DUPLICATE_SAME_ACCESS);
	CloseHandle(linked_token.LinkedToken);
	accept_admin();
	return 1;
}
void UACParser::accept(std::wstring user, std::wstring pass) {
	if (is_admin)
		accept_admin();
	else
		accept_nonadmin(user, pass);
}
