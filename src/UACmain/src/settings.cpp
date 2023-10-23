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
        if (val.IsValid()) {
            if (val.GetValue() == 0 || val.GetValue() == 1) {
                prompt_on_secure_desktop = val.GetValue();
                prompt_set = true;
            }
        }
        key.Close();
    }
	
    if (!prompt_set) {
        reg_result = key.TryOpen(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System", KEY_READ);
        if (reg_result) {
            auto val = key.TryGetDwordValue(L"PromptOnSecureDesktop");
			if(val.IsValid()){
				prompt_on_secure_desktop = val.GetValue()!=0;
			}else{
				prompt_on_secure_desktop = true;
			}
            key.Close();
        }
    }
	
    reg_result = key.TryOpen(HKEY_LOCAL_MACHINE, L"Software\\CustomUAC", KEY_READ);
    if (reg_result) {
		auto installation_reg = key.TryGetStringValue(L"installation");
		auto theme_reg = key.TryGetStringValue(L"Theme");
        auto network_reg = key.TryGetDwordValue(L"AllowNetwork");
		auto allow_local_storage_reg = key.TryGetDwordValue(L"AllowLocalStorage");
		
        installation = installation_reg.IsValid()?installation_reg.GetValue():std::wstring();
        theme = theme_reg.IsValid()?theme_reg.GetValue():std::wstring();
		network = network_reg.IsValid()?(network_reg.GetValue()!=0):false;
		allow_local_storage = allow_local_storage_reg.IsValid()?(allow_local_storage_reg.GetValue()!=0):false;

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
