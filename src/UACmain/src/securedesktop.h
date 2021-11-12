#ifndef QML_TEST_SECUREDESKTOP_H
#define QML_TEST_SECUREDESKTOP_H

#include <Windows.h>
#include <cstdint>
#include <QQuickImageProvider>

class SecureDesktop {
private:
    SecureDesktop();
    bool is_on = false;
    HDESK saved_desktop = NULL;
    RECT rect = { 0 };
    HANDLE mutex;
    void grab_desktop();
protected:
    friend class ImageProvider;
    HBITMAP hbitmap = NULL;
public:
    bool has_switched();
    HDESK get_saved();
    static SecureDesktop& instance();
    void switch_desktop();
    void switch_back();
};

class ImageProvider :public QQuickImageProvider {
private:
    static ImageProvider* instance;
    ImageProvider();
protected:
    friend class SecureDesktop;
    QImage img;
public:
    QRect virtual_desktop;
    static bool has_instance();
    static ImageProvider* get();
    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize)override;
};
#endif //QML_TEST_SECUREDESKTOP_H
