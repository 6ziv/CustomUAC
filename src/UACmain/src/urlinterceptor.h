#ifndef QML_TEST_URLINTERCEPTOR_H
#define QML_TEST_URLINTERCEPTOR_H
#include <QQmlAbstractUrlInterceptor>

class UrlInterceptor:public QQmlAbstractUrlInterceptor
{
private:
    bool allow_network;
public:
    UrlInterceptor(bool network);
    QUrl intercept(const QUrl &path, DataType type) override;
};


#endif //QML_TEST_URLINTERCEPTOR_H
