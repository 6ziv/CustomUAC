#include "securedesktop.h"
#include <windows.h>
#include <ShellScalingApi.h>
#include <QLabel>
#include <QPixmap>
#include <QGuiApplication>
#include <QScreen>
#include <QImage>
#include <QPainter>
#include <boost/scope_exit.hpp>
/*
 * It is easier and more 'elegant' to use RAII.
 * but Qt shutdown involves destruction of QML, which is less controllable.
 * so we switch back desktop manually and force shutdown to avoid error.
 * QPixmap requires QApplication (as it is intended to be optimized for drawing), so use QImage.
 */
#define UNUSED(x) (void)(x)
void OnDesktopSwitched(
    HWINEVENTHOOK hWinEventHook,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD idEventThread,
    DWORD dwmsEventTime
)
{
    UNUSED(hWinEventHook);UNUSED(event);UNUSED(hwnd);UNUSED(idObject);UNUSED(idChild);UNUSED(idEventThread);UNUSED(dwmsEventTime);
    SecureDesktop::instance().switch_back();
    ExitProcess(1223);
}
SecureDesktop::SecureDesktop(){};
bool SecureDesktop::has_switched(){
    return is_on;
}
HDESK SecureDesktop::get_saved(){return saved_desktop;}
void SecureDesktop::switch_desktop() {
    grab_desktop();
    saved_desktop=GetThreadDesktop(GetCurrentThreadId());
    HDESK winlogon_desktop=OpenDesktopA("Winlogon",0,FALSE,GENERIC_ALL);
    BOOL ret = SetThreadDesktop(winlogon_desktop) && SwitchDesktop(winlogon_desktop);
    if (!ret) { CloseDesktop(winlogon_desktop); return; }
    SetWinEventHook(EVENT_SYSTEM_DESKTOPSWITCH, EVENT_SYSTEM_DESKTOPSWITCH, NULL, OnDesktopSwitched, 0, 0, WINEVENT_SKIPOWNPROCESS);
    is_on=true;
}
void SecureDesktop::switch_back(){
    if(!is_on)return;
    SwitchDesktop(saved_desktop);
}
SecureDesktop& SecureDesktop::instance(){
    static SecureDesktop i;
    return i;
}
void SecureDesktop::grab_desktop() {
    UINT dpi = GetDpiForSystem();
    rect.left = GetSystemMetricsForDpi(SM_XVIRTUALSCREEN,dpi);
    rect.top = GetSystemMetricsForDpi(SM_YVIRTUALSCREEN,dpi);
    rect.right = rect.left + GetSystemMetricsForDpi(SM_CXVIRTUALSCREEN,dpi);
    rect.bottom = rect.top + GetSystemMetricsForDpi(SM_CYVIRTUALSCREEN, dpi);
    //GetClientRect(GetDesktopWindow(), &rect);
    HDC hDC = GetDC(NULL);
    HDC newDC = CreateCompatibleDC(hDC);
    hbitmap = CreateCompatibleBitmap(hDC, rect.right - rect.left, rect.bottom - rect.top);
    SelectObject(newDC, hbitmap);
    BitBlt(newDC, 0, 0, rect.right - rect.left, rect.bottom - rect.top, hDC, rect.left, rect.top, SRCCOPY);
    
    DeleteDC(newDC);
    ReleaseDC(NULL,hDC);
    ImageProvider::get()->img = QImage::fromHBITMAP(hbitmap);
    ImageProvider::get()->virtual_desktop = QRect(
        GetSystemMetrics(SM_XVIRTUALSCREEN),
        GetSystemMetrics(SM_YVIRTUALSCREEN),
        GetSystemMetrics(SM_CXVIRTUALSCREEN),
        GetSystemMetrics(SM_CYVIRTUALSCREEN)
    );
    DeleteObject(hbitmap);
}
QImage ImageProvider::requestImage(const QString& id, QSize* size, const QSize& requestedSize){
    img.save("E:\\screenshot.jpg");
    bool ok;
    double opacity=id.toDouble(&ok);
    if (!ok)opacity = 1;
    *size = img.size();
    QImage img2 = img;
    QPainter painter;
    painter.begin(&img2);
    painter.setOpacity(1 - opacity);
    painter.fillRect(img2.rect(), Qt::black);
    painter.end();
    if (requestedSize.width() > 0 && requestedSize.height() > 0) {
        return img2.scaled(requestedSize);
    }
    else {
        return img2;
    }
}
ImageProvider::ImageProvider()
    :QQuickImageProvider(QQmlImageProviderBase::ImageType::Image, QQmlImageProviderBase::Flags(NULL)){}

ImageProvider* ImageProvider::get() {
    if (!instance)instance = new ImageProvider;
    return instance;
}
ImageProvider* ImageProvider::instance = nullptr;
bool ImageProvider::has_instance() {
    return instance;
}
