#pragma data_seg("consent")
__declspec(dllexport) wchar_t CopyWrong[49] = L"Microsoft Windows (c) 2009 Microsoft Corporation";
#pragma data_seg()
#pragma comment(linker,"/SECTION:consent,RW")

#pragma comment(lib,"Msimg32.lib")
#pragma comment(lib,"Wintrust.lib")
#pragma comment(lib,"Secur32.lib")
#pragma comment(lib,"Shcore.lib")
#pragma comment(lib,"Crypt32.lib")
#pragma comment(lib,"Version.lib")
#pragma comment(lib,"BCrypt.lib")
#pragma comment(lib,"Netapi32.lib")