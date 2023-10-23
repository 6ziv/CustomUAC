#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <unqlite.h>
#include <QFile>
#include <QUuid>
#include <QMessageBox>
#include <QFileDialog>
#include <QCryptographicHash>
#include <QResource>
#include <QSettings>
#include <QTranslator>
#include <WinReg.hpp>
#include <boost/filesystem.hpp>
#include <boost/scope_exit.hpp>
#include <Windows.h>
Q_DECLARE_METATYPE(std::vector<unsigned char>)
const size_t HASH_LEN = 32;
typedef struct {
    uint64_t size;
    uint8_t hash[HASH_LEN];
}value_t;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    std::wstring strInstallation;
    {
        winreg::RegKey key;
        auto reg_result = key.TryOpen(HKEY_LOCAL_MACHINE, L"Software\\CustomUAC", KEY_READ);
        if (reg_result) {
			auto get_installation_result = key.TryGetStringValue(L"installation");
            strInstallation = get_installation_result.IsValid()?get_installation_result.GetValue():std::wstring();
            key.Close();
        }
    }
    if(!strInstallation.empty()){
        installation=std::filesystem::path(strInstallation);
    }else{
        DWORD len=(MAX_PATH-1)/2+1;
        DWORD ret=len;
        while(ret>=len){
            len=len*2;
            wchar_t *filename=new wchar_t[len];
            ret=GetModuleFileNameW(NULL,filename,len);
            if(ret<len)
                installation=std::filesystem::path(filename).parent_path();
            delete filename;
        }
    }
    winreg::RegKey key;
    auto reg_result = key.TryOpen(HKEY_LOCAL_MACHINE, L"Software\\CustomUAC",KEY_SET_VALUE);
    if (reg_result) {
        key.SetStringValue(L"installation",installation.wstring());
    }
    reloadBasicSettings();
    reloadAdvancedSettings();
    reloadSecureDesktopSetting();
    reloadAllowlist();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::reloadBasicSettings(){
    winreg::RegKey key;
    auto reg_result = key.TryOpen(HKEY_LOCAL_MACHINE, L"Software\\CustomUAC", KEY_READ);
    std::wstring theme;
    if (reg_result) {
		auto get_theme_result = key.TryGetStringValue(L"Theme");
        theme = get_theme_result.IsValid()?get_theme_result.GetValue():std::wstring();
        key.Close();
    }
    QFile themelist(QString::fromStdWString((installation/"themes/themes.list").wstring()));
    themelist.open(QFile::ReadOnly|QFile::Text);
    ui->combo_theme->clear();
    themes.clear();
    while(1){
        if(themelist.atEnd())break;
        QString str=themelist.readLine().trimmed();
        if(str.isNull() || str.isEmpty())continue;
        boost::filesystem::path resource_path=(installation/"themes"/((str+".rcc").toStdWString()));
        if(!boost::filesystem::exists(resource_path))continue;
        std::wstring resource_strpath=resource_path.wstring();

        bool theme_loaded=QResource::registerResource(QString::fromStdWString(resource_strpath),QString("/uac-content"));
        BOOST_SCOPE_EXIT(resource_strpath){QResource::unregisterResource(QString::fromStdWString(resource_strpath),QString("/uac-content"));}BOOST_SCOPE_EXIT_END;
        if(!theme_loaded)continue;

        QString name;
        QString name_i;
        QSettings config(":/uac-content/config.ini",QSettings::IniFormat);
        if(config.contains("theme_name") && config.value("theme_name").canConvert<QString>())
            name=config.value("theme_name").toString();
        QTranslator translator;
        if (translator.load(QLocale(), QString(), QString(), ":/uac-content/translations", QString())) {
            name_i = translator.translate("METADATA",name.toStdString().c_str());
        }
        if(name_i.isNull()||name_i.isEmpty())
            name_i=name;
        if(name_i.isNull()||name_i.isEmpty()){
            name_i="Untitled theme";
        }
        themes.push_back(str);
        //themes.push_back(qMakePair(str,name_i));
        ui->combo_theme->addItem(name_i,str);
    }
    ui->combo_theme->insertItem(0,QTranslator::tr("default theme"));
    int i=ui->combo_theme->findData(QString::fromStdWString(theme),Qt::UserRole,Qt::MatchFixedString);
    if(i>=0)ui->combo_theme->setCurrentIndex(i);else ui->combo_theme->setCurrentIndex(0);

}

void MainWindow::reloadAdvancedSettings(){
    winreg::RegKey key;
    auto reg_result = key.TryOpen(HKEY_LOCAL_MACHINE, L"Software\\CustomUAC", KEY_READ);
    bool network,local_storage;
    if (reg_result) {
		auto get_network_result = key.TryGetDwordValue(L"AllowNetwork");
        network = get_network_result.IsValid()?(get_network_result.GetValue()!=0):false;
		auto get_local_storage_result = key.TryGetDwordValue(L"AllowLocalStorage");
        local_storage = get_local_storage_result.IsValid()?(get_local_storage_result.GetValue()!=0):false;;
        key.Close();
    }
    ui->cb_service->setEnabled(false);
    do{
        SC_HANDLE hSC=OpenSCManager(NULL,SERVICES_ACTIVE_DATABASE,SC_MANAGER_CONNECT);
        if(hSC==NULL) break;
        BOOST_SCOPE_EXIT(hSC){CloseServiceHandle(hSC);}BOOST_SCOPE_EXIT_END;

        SC_HANDLE hSvc=OpenServiceA(hSC,"UACService",SERVICE_QUERY_CONFIG);
        if(hSvc==NULL){
            if(ERROR_SERVICE_DOES_NOT_EXIST==GetLastError()){
                ui->cb_service->setEnabled(true);
                ui->cb_service->setChecked(false);
            }else
                qDebug()<<GetLastError();
        }else{
            CloseServiceHandle(hSC);
            ui->cb_service->setEnabled(true);
            ui->cb_service->setChecked(true);
        }
    }while(0);
    ui->cb_network->setChecked(network);
    ui->cb_localstorage->setChecked(local_storage);
}

void MainWindow::reloadSecureDesktopSetting(){
    winreg::RegKey key;
    auto reg_result = key.TryOpen(HKEY_LOCAL_MACHINE, L"Software\\CustomUAC", KEY_READ);
    DWORD securedesktop;
    if (reg_result) {
		auto get_secure_desktop_result = key.TryGetDwordValue(L"PromptOnSecureDesktop");
        securedesktop = get_secure_desktop_result.IsValid()?get_secure_desktop_result.GetValue():-1;
        key.Close();
    }
    switch (securedesktop) {
        case 1:ui->rb_sd_y->setChecked(true);break;
        case 0:ui->rb_sd_n->setChecked(true);break;
        default:ui->rb_sd_undefined->setChecked(true);break;
    }
}

void MainWindow::on_pb_ApplyAdvancedSettings_clicked()
{
    {
        winreg::RegKey key;
        auto reg_result = key.TryOpen(HKEY_LOCAL_MACHINE, L"Software\\CustomUAC", KEY_SET_VALUE);
        if (reg_result) {
            key.SetDwordValue(L"AllowNetwork",ui->cb_network->isChecked()?1:0);
            key.SetDwordValue(L"AllowLocalStorage",ui->cb_localstorage->isChecked()?1:0);
            key.Close();
        }
    }
    if(ui->cb_service){
        do{
            SC_HANDLE hSC=OpenSCManager(NULL,SERVICES_ACTIVE_DATABASE,SC_MANAGER_CONNECT|SC_MANAGER_CREATE_SERVICE);
            if(hSC==NULL) break;
            BOOST_SCOPE_EXIT(hSC){CloseServiceHandle(hSC);}BOOST_SCOPE_EXIT_END;

            auto service=installation;
            service/="bin/UACService.exe";
            if(boost::filesystem::exists(service))
                break;
            SC_HANDLE hSvc=CreateServiceW(
                        hSC,
                        L"UACService",
                        L"Custom UAC Service",
                        SERVICE_ALL_ACCESS,
                        SERVICE_WIN32_OWN_PROCESS,
                        SERVICE_AUTO_START,
                        SERVICE_ERROR_NORMAL,
                        service.wstring().c_str(),
                        NULL,NULL,NULL,NULL,L""
                        );
            if(hSvc==NULL){
                if(ERROR_SERVICE_DOES_NOT_EXIST==GetLastError()){
                    ui->cb_service->setEnabled(true);
                    ui->cb_service->setChecked(false);
                }else
                    qDebug()<<GetLastError();
            }else{
                CloseServiceHandle(hSC);
                ui->cb_service->setEnabled(true);
                ui->cb_service->setChecked(true);
            }
        }while(0);
    }else{
        do{
            SC_HANDLE hSC=OpenSCManager(NULL,SERVICES_ACTIVE_DATABASE,SC_MANAGER_CONNECT);
            if(hSC==NULL) break;
            BOOST_SCOPE_EXIT(hSC){CloseServiceHandle(hSC);}BOOST_SCOPE_EXIT_END;
            SC_HANDLE hSvc = OpenServiceW(hSC,L"UACService",DELETE);
            if (hSvc==NULL)break;
            BOOST_SCOPE_EXIT(hSvc){CloseServiceHandle(hSvc);}BOOST_SCOPE_EXIT_END;
            DeleteService(hSvc);
        }while(0);
    }
    reloadAdvancedSettings();
}

void MainWindow::on_pb_ApplySecureDesktop_clicked()
{
    {
        winreg::RegKey key;
        auto reg_result = key.TryOpen(HKEY_LOCAL_MACHINE, L"Software\\CustomUAC",KEY_SET_VALUE);
        if (reg_result) {
            DWORD v;
            if(ui->rb_sd_y->isChecked())v=1;
            if(ui->rb_sd_n->isChecked())v=0;
            if(ui->rb_sd_undefined->isChecked())v=0xFFFFFFFF;
            key.SetDwordValue(L"PromptOnSecureDesktop",v);
            key.Close();
        }
    }
    reloadSecureDesktopSetting();
}


void MainWindow::on_pb_ApplyTheme_clicked()
{
    {
        winreg::RegKey key;
        auto reg_result = key.TryOpen(HKEY_LOCAL_MACHINE, L"Software\\CustomUAC",KEY_SET_VALUE);
        if (reg_result) {
            QString current_string=ui->combo_theme->currentData().toString();
            key.SetStringValue(L"theme",current_string.toStdWString());
        }
    }
    reloadBasicSettings();
}


void MainWindow::on_pb_DeleteTheme_clicked()
{
    {
        QString current_string=ui->combo_theme->currentData().toString();

        themes.removeAll(current_string);
        winreg::RegKey key;
        auto reg_result = key.TryOpen(HKEY_LOCAL_MACHINE, L"Software\\CustomUAC",KEY_READ|KEY_SET_VALUE);
        if (reg_result) {
			auto get_theme_result = key.TryGetStringValue(L"theme");
			auto theme_str = get_theme_result.IsValid()?get_theme_result.GetValue():std::wstring();
            if(current_string.toStdWString()==theme_str)
                key.SetStringValue(L"theme",L"");
            key.Close();
        }
        QFile file(QString::fromStdWString((installation/"themes/themes.list").wstring()));
        if(file.open(QFile::WriteOnly | QFile::Truncate |QFile::Text)){
            QTextStream stream(&file);
            for(auto item:themes)
                stream<<item<<"\n";
        }
        file.close();
        boost::filesystem::remove(installation/"themes"/(current_string.toStdWString()+L".rcc"));
    }
    reloadBasicSettings();
}


void MainWindow::on_pb_ImportTheme_clicked()
{
    {
        QString selected_theme=
                QFileDialog::getOpenFileName(
                      this,
                      tr("Select a theme file"),
                      QString(),
                      tr("RCC files")+" (*.rcc);;"+tr("All files")+" (*)",
                      nullptr
                      );
        if(selected_theme.isNull() || selected_theme.isEmpty())return;
        if(!QResource::registerResource(selected_theme,"/tmp_new_theme")){
            QMessageBox(QMessageBox::Icon::Warning,
                        tr("Error"),
                        tr("The seected file is not a valid qt resource bundle."),
                        QMessageBox::StandardButton::Ok,
                        this
                        ).exec();
            return;
        }
        QResource::unregisterResource(selected_theme,"/tmp_new_theme");
        QString new_name;
        do{
            new_name=QUuid::createUuid().toString(QUuid::WithoutBraces);
        }while(boost::filesystem::exists(installation/"themes"/(new_name.toStdWString()+L".rcc")));
        QFile::copy(selected_theme,
                    QString::fromStdWString((installation/"themes"/(new_name.toStdWString()+L".rcc")).wstring())
                    );
        themes.push_back(new_name);

        QFile file(QString::fromStdWString((installation/"themes/themes.list").wstring()));
        if(file.open(QFile::WriteOnly | QFile::Append |QFile::Text)){
            QTextStream stream(&file);
            stream<<new_name<<"\n";
        }
        file.close();
    }
    reloadBasicSettings();
}

void MainWindow::reloadAllowlist(){
    ui->g_allowlist->setEnabled(false);
    ui->lw_allowlist->clear();
    {
        allowlist_data.clear();
        unqlite* db;
        int r = unqlite_open(&db, (installation/"conf/allowlist.db").string().c_str(), UNQLITE_OPEN_READONLY);
        if (r != UNQLITE_OK){
            return;
        }
        BOOST_SCOPE_EXIT(db) { unqlite_close(db);}BOOST_SCOPE_EXIT_END;
        unqlite_kv_cursor *cursor;
        if(UNQLITE_OK!=unqlite_kv_cursor_init(db,&cursor))return;
        BOOST_SCOPE_EXIT(db,cursor) { unqlite_kv_cursor_release(db,cursor);}BOOST_SCOPE_EXIT_END;
        for(int status=unqlite_kv_cursor_first_entry(cursor);UNQLITE_OK==status;status=unqlite_kv_cursor_next_entry(cursor)){
            std::vector<unsigned char> keydata;
            keydata.clear();
            if(UNQLITE_OK!=unqlite_kv_cursor_key_callback(
                        cursor,
                        [](const void *pOutput,unsigned int nOutputLen,void *userData)->int
                            {
                                std::vector<unsigned char>& key=*reinterpret_cast<std::vector<unsigned char> *>(userData);
                                size_t oldsize=key.size();
                                key.resize(oldsize+nOutputLen);
                                memcpy(key.data()+oldsize,pOutput,nOutputLen);
                                return UNQLITE_OK;
                            },
                        &keydata))
            {
                continue;
            }

            QListWidgetItem *item=new QListWidgetItem(ui->lw_allowlist);
            item->setData(Qt::UserRole,QVariant::fromValue<std::vector<unsigned char>>(keydata));

            keydata.push_back(0);keydata.push_back(0);
            std::wstring path=reinterpret_cast<wchar_t*>(keydata.data());
            item->setText(QString::fromStdWString(path));
            ui->lw_allowlist->addItem(item);
        }
    }
    ui->lw_allowlist->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ui->g_allowlist->setEnabled(true);
}

void MainWindow::on_pb_removeal_clicked()
{
    {
        auto selected_items=ui->lw_allowlist->selectedItems();
        unqlite* db;
        int r = unqlite_open(&db, (installation/"conf/allowlist.db").string().c_str(), UNQLITE_OPEN_READWRITE|UNQLITE_OPEN_EXCLUSIVE);
        if(UNQLITE_OK!=r)return;
        BOOST_SCOPE_EXIT(db){unqlite_close(db);}BOOST_SCOPE_EXIT_END;
        for(auto& item:selected_items){
            auto keydata=item->data(Qt::UserRole).value<std::vector<unsigned char>>();
            unqlite_kv_delete(db,keydata.data(),keydata.size());
        }
    }
    reloadAllowlist();
}



