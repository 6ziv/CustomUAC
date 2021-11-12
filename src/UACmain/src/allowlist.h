#ifndef allowlist_H
#define allowlist_H
#include <string>
#include "uacdata.h"
bool add_to_allowlist(const UACData& data);
bool check_allowlist(const std::wstring& file);
void set_db(const std::wstring& db);
#endif