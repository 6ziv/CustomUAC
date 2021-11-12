#ifndef SETTINGS_H
#define SETTINGS_H
#include <string>
class Settings {
public:
    void load();
	bool prompt_on_secure_desktop;
	bool network;
	bool allow_local_storage;
	std::wstring theme;
//	std::wstring allowlist;
//	std::wstring theme_identifier;
	std::wstring installation;

	std::wstring get_softgl() const;
	std::wstring get_bin_directory() const;
	std::wstring get_allowlist() const;
	std::wstring get_theme() const;
	std::wstring get_local_storage() const;
};
#endif // SETTINGS_H
