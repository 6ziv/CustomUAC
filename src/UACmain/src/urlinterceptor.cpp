#include "UrlInterceptor.h"
#include <QUrl>
#include "qdebug.h"
UrlInterceptor::UrlInterceptor(bool network):
    QQmlAbstractUrlInterceptor()
{
    allow_network=network;
}
QUrl UrlInterceptor::intercept(const QUrl &path, DataType type){
    Q_UNUSED(type)
    if(path.scheme()=="http" || path.scheme()=="https")
        return allow_network?path:QUrl();
    if (path.scheme() != "qrc" && path.scheme() != "image") {
        return QUrl();
    }
    return path;
}
