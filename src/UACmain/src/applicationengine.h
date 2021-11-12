#ifndef APPLICATIONENGINE_H
#define APPLICATIONENGINE_H
#include <QQmlApplicationEngine>
#include <QUrl>
#include "uacparser.h"
class ApplicationEngine:
    private QQmlApplicationEngine
{
	Q_OBJECT
private:
	QUrl main_url;
	QUrl background_url;
public:
	ApplicationEngine(bool network, bool prompt_on_secure_desktop,UACParser& parser,const std::wstring& local_storage);
};
#endif // !APPLICATIONENGINE_H
