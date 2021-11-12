#ifndef UACDATA_H
#define UACDATA_H

#include <QObject>
#include <QQmlEngine>
#include "uacparser.h"
class UACPermissions :public QObject {
    Q_OBJECT
    Q_PROPERTY(bool allow_network READ allow_network CONSTANT)
    Q_PROPERTY(bool allow_local_storage READ allow_local_storage CONSTANT)
    Q_PROPERTY(bool allow_opengl READ allow_opengl CONSTANT)
    QML_ELEMENT
public:
    static UACPermissions* get();
    bool allow_network() const;
    bool allow_local_storage() const;
    bool allow_opengl() const;
    /*
    * themes can of course check for opengl support from other paths.
    * this is only provided for convenience.
    */

    bool network;
    bool local_storage;
    bool opengl;
private:
    static UACPermissions* pinstance;
};
class UACData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString path READ path CONSTANT)
    Q_PROPERTY(QString MSIVersion READ MSIVersion CONSTANT)
    Q_PROPERTY(QString description READ description CONSTANT)
    Q_PROPERTY(QString file_description READ file_description CONSTANT)
    Q_PROPERTY(QString signer READ signer CONSTANT)

    Q_PROPERTY(QStringList arguments READ arguments CONSTANT)
    Q_PROPERTY(QStringList administrators READ administrators CONSTANT)

    Q_PROPERTY(unsigned int type READ type CONSTANT)
    Q_PROPERTY(bool isadmin READ isadmin CONSTANT)
    QML_ELEMENT
public:
    explicit UACData(UACParser& parser,QObject *parent = nullptr);
    QString path() const;
    QString MSIVersion() const;
    
    QString description() const;
    QString file_description() const;
    QString signer() const;
    QStringList arguments() const;
    QStringList administrators() const;
    unsigned int type() const;
    bool isadmin() const;
    Q_INVOKABLE void allowlist();
    Q_INVOKABLE void login(const QString &username,const QString password);
    Q_INVOKABLE void login();
    Q_INVOKABLE void reject();
private:
    UACParser& uac_parser;

    //QString m_path,m_description,m_signer,m_filedescription;
    //QStringList m_arguments, m_administrators;
    //causing performance loss for conversion. bearable.

};

#endif // UACDATA_H
