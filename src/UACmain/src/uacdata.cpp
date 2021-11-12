#include "uacdata.h"
#include "uacparser.h"
#include "allowlist.h"
#include <QMessageBox>
#include <QTranslator>
#include <QLocale>
#include <boost/format.hpp>
#include <boost/utility/string_view.hpp>
#include "qdebug.h"

bool UACPermissions::allow_network() const { return network; }
bool UACPermissions::allow_local_storage() const { return local_storage; }
bool UACPermissions::allow_opengl() const { return opengl; }
UACPermissions* UACPermissions::get() {
    if (pinstance == nullptr)
        pinstance = new UACPermissions;
    return pinstance;
}
UACPermissions* UACPermissions::pinstance = nullptr;

QString UACData::path() const {
    return QString::fromStdWString(uac_parser.path);
}
QString UACData::MSIVersion() const {
    return path();
}
QString UACData::description() const {
    return QString::fromStdWString(uac_parser.description);
}
QString UACData::file_description() const {
    return QString::fromStdWString(uac_parser.file_description);
}
QString UACData::signer() const {
    return QString::fromStdWString(uac_parser.signer);
}
unsigned int UACData::type() const {
    return uac_parser.uac_type;
}
bool UACData::isadmin() const {
    return uac_parser.is_admin;
}
QStringList UACData::arguments() const {
    QStringList ret;
    for(auto& str:uac_parser.parameters)
        ret.push_back(QString::fromStdWString(str));
    return ret;
}
QStringList UACData::administrators() const {
    QStringList ret;
    for (auto& str : uac_parser.local_administrators)
        ret.push_back(QString::fromStdWString(str));
    return ret;
}
Q_INVOKABLE void UACData::login(const QString &username,const QString password){
    uac_parser.accept(username.toStdWString(),password.toStdWString());
}
Q_INVOKABLE void UACData::login(){
    login("","");
}
Q_INVOKABLE void UACData::reject() {
    uac_parser.reject();
}
UACData::UACData(UACParser& parser,QObject *parent)
    :QObject(parent),
    uac_parser(parser)
{}
void UACData::allowlist() {
    if (!isadmin())return;
    constexpr boost::string_view msg[2] = {
        "Adding program %1% at location %2% to the allow list. Confirm?",
        "Adding program %1% signed by %3% which is located at %2% to the allow list. Confirm?" 
    };
    static bool inited = false;
    static QString translation[2];
    if (!inited) {
        // Do not install global translators, to prevent mess. 
        
        QTranslator translator;
        if (translator.load(QLocale(), QString(), QString(), ":/application-translations", QString())) {
            for (size_t i = 0; i < 2; i++) {
                translation[i] = translator.translate("allowlist", msg[i].to_string().c_str());
            }
        }
		for (size_t i = 0; i < 2; i++) {
            if (translation[i].isNull() || translation[i].isEmpty())
                translation[i] = QString::fromStdString(msg[i].to_string());
        }
		inited = true;
    }
    
    auto fmt = boost::wformat(translation[signer().isEmpty() ? 1 : 0].toStdWString());
    fmt.exceptions(boost::io::no_error_bits);
    QString text = QString::fromStdWString(
        boost::str(
            fmt
            % (file_description().isEmpty()?description():file_description()).toStdWString().c_str()
            % path().toStdWString().c_str()
            % signer().toStdWString().c_str()
        )
    );

    QMessageBox mb(QMessageBox::Question, "UAC consent", text, QMessageBox::Yes | QMessageBox::No, nullptr);
    
    int result = mb.exec();
    if (result == QMessageBox::Yes) {
        add_to_allowlist(*this);
        this->login();
    }
}