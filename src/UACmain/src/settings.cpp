#include "settings.h"
#include <WinReg.hpp>
#include <boost/filesystem.hpp>

void Settings::load() {
    prompt_on_secure_desktop = true;

    bool prompt_set = false;
    winreg::RegKey key;
    auto reg_result = key.TryOpen(HKEY_LOCAL_MACHINE, L"Software\\CustomUAC", KEY_READ);
    if (reg_result) {
        auto val = key.TryGetDwordValue(L"PromptOnSecureDesktop");
        if (val.has_value()) {
            if (val.value() == 0 || val.value() == 1) {
                prompt_on_secure_desktop = val.value();
                prompt_set = true;
            }
        }
        key.Close();
    }
	
    if (!prompt_set) {
        reg_result = key.TryOpen(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", KEY_READ);
        if (reg_result) {
            auto val = key.TryGetDwordValue(L"PromptOnSecureDesktop");
            prompt_on_secure_desktop = val.value_or(1) != 0;
            key.Close();
        }
    }
	
    reg_result = key.TryOpen(HKEY_LOCAL_MACHINE, L"Software\\CustomUAC", KEY_READ);
    if (reg_result) {
        installation = key.TryGetStringValue(L"installation").value_or(std::wstring());
        theme = key.TryGetStringValue(L"Theme").value_or(std::wstring());
        network = key.TryGetDwordValue(L"AllowNetwork").value_or(0) != 0;
		allow_local_storage = key.TryGetDwordValue(L"AllowLocalStorage").value_or(0) != 0;
        key.Close();
    }
	
}

std::wstring Settings::get_allowlist() const{
    boost::filesystem::path p(installation);
    p /= "conf/allowlist.db";
    return p.wstring();
}
std::wstring Settings::get_theme() const{
    boost::filesystem::path p(installation);
    p /= "themes";
    p /= theme + L".rcc";
    if (theme.empty()||boost::filesystem::exists(p))
        return p.wstring();
    else
        return std::wstring();
}
std::wstring Settings::get_local_storage() const{
    // it is not elegant but useful to give an illegal path.
    if (!allow_local_storage)
        return std::wstring();
    
    boost::filesystem::path p(installation);
    if (!boost::filesystem::exists(p / "themes" / (theme + L".rcc"))) {
        p /= "storage";
        p /= "default_theme";
    }
    else {
        p /= "storage";
        p /= theme;
    }
    if (!boost::filesystem::exists(p))
        boost::filesystem::create_directories(p);
    return p.wstring();
}
std::wstring Settings::get_bin_directory() const {
    boost::filesystem::path p(installation);
    p /= "bin";
    if (!boost::filesystem::exists(p))
        return std::wstring();
    else
        return p.wstring();
}
std::wstring Settings::get_softgl() const {
    boost::filesystem::path p(installation);
    p /= "bin/opengl32sw.dll";
    if (!boost::filesystem::exists(p))
        return std::wstring();
    else
        return p.wstring();
}
