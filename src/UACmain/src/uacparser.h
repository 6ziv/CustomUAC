#ifndef UACPARSER_H
#define UACPARSER_H
#include <Windows.h>
#include <string>
#include <vector>
class UACParser
{
private:
    HANDLE hProcess=INVALID_HANDLE_VALUE;
    void* readUAC(int argc,char** argv,size_t& retlen);
    bool parseUAC(void* uac,size_t len);
    void tryGetFileDescription();
    void tryGetFileSigner();
    void get_administrators();
    void accept_admin();
    int accept_nonadmin(std::wstring user,std::wstring pass);
public:
    UACParser(int argc,char** argv);
    ~UACParser();
    bool is_ok;
    std::wstring description;
    std::wstring path;
    std::vector<std::wstring> parameters;
    std::vector<std::wstring> local_administrators;
    std::wstring file_description;
    std::wstring signer;
    void* write_target;
    HANDLE access_handle;
    bool is_admin;
    uint32_t uac_type;
    void accept(std::wstring user,std::wstring pass);
    void reject();
};

#endif // UACPARSER_H
