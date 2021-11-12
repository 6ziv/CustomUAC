#include "applicationengine.h"
#include <QSettings>
#include <QScreen>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQuickView>
#include <QFile>
#include <QLibraryInfo>
#include <QResource>
#include "urlinterceptor.h"
#include "uacdata.h"
#include "securedesktop.h"
ApplicationEngine::ApplicationEngine(bool network, bool prompt_on_secure_desktop, UACParser& parser,const std::wstring& local_storage)
	: QQmlApplicationEngine(nullptr)
{
    {
		QStringList list = importPathList();
		QStringList list2;
        for(size_t i=0;i<static_cast<size_t>(list.size());i++){
			QUrl url(list[i]);
			if(url.scheme()=="qrc")list2.push_back(list[i]);
		}
		setImportPathList(list2);
	}
    if (local_storage.empty()) {
        setOfflineStoragePath(QString::fromStdWString(L"<**LOCAL_STORAGE_DISALLOWED**>"));
        UACPermissions::get()->local_storage = false;
    }
    else {
        setOfflineStoragePath(QString::fromStdWString(local_storage));
        UACPermissions::get()->local_storage = true;
    }
    QFile file(":/uac-content/config.ini");
    QSettings config(":/uac-content/config.ini", QSettings::IniFormat);
    main_url = QUrl(config.value("main_window").canConvert<QString>() ? config.value("main_window").toString() : QString());
    background_url = QUrl(config.value("background").canConvert<QString>() ? config.value("background").toString() : QString());
    bool allow_network = network && (config.value("allow_network").canConvert<bool>() ? config.value("allow_network").toBool() : false);
    UACPermissions::get()->network = allow_network;
    QUrl base_url = QUrl("qrc:///uac-content/");
    if (main_url.isRelative())
        main_url = base_url.resolved(main_url);
    if (background_url.isRelative())
        background_url = base_url.resolved(background_url);
    this->addUrlInterceptor(new UrlInterceptor(allow_network));
    this->rootContext()->setContextProperty("uac", new UACData(parser));
    this->rootContext()->setContextProperty("uac_permissions", UACPermissions::get());
    /*
    *  Set initial location and screen,
    *  as Qt may give faulty 'default' locations with dual monitors attached and secondary monitor on lefttop.
    */
    
    connect(qApp, &QCoreApplication::aboutToQuit, this, &QQmlApplicationEngine::quit);
    if (ImageProvider::has_instance())
        this->addImageProvider("screenshot", ImageProvider::get());
    qDebug()<<"Creating Background Window";
    if (prompt_on_secure_desktop) {
        QQuickView* view = new QQuickView(this, nullptr);
		view->connect(view,&QQuickView::sceneGraphError,view,&QQuickView::update);
        view->setFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnBottomHint);

        /*
        * dirty work: do setGeometry twice to ensure devicePixelRatio is updated.
        */
        view->setGeometry(ImageProvider::get()->virtual_desktop.x() / view->devicePixelRatio(),
            ImageProvider::get()->virtual_desktop.y() / view->devicePixelRatio(),
            ImageProvider::get()->virtual_desktop.width() / view->devicePixelRatio(),
            ImageProvider::get()->virtual_desktop.height() / view->devicePixelRatio()
        );
        view->setGeometry(ImageProvider::get()->virtual_desktop.x() / view->devicePixelRatio(),
            ImageProvider::get()->virtual_desktop.y() / view->devicePixelRatio(),
            ImageProvider::get()->virtual_desktop.width() / view->devicePixelRatio(),
            ImageProvider::get()->virtual_desktop.height() / view->devicePixelRatio()
        );

        view->setResizeMode(QQuickView::ResizeMode::SizeRootObjectToView);
        view->setSource(background_url);
        view->show();
    }
    qDebug()<<"Creating Background Window Done";
    QObject::connect(this, &QQmlApplicationEngine::objectCreated,
        this, [this](QObject* obj, const QUrl& objUrl) {
            if (!obj && main_url == objUrl)
                QCoreApplication::exit(1223);
            if(obj->inherits("QQuickWindow")){
                QQuickWindow* window=reinterpret_cast<QQuickWindow*>(obj);
                //window->connect(window,&QQuickWindow::sceneGraphInitialized,this,[](){qDebug()<<"init";});
                window->connect(window,&QQuickWindow::sceneGraphError,this,[window](){
                    window->update();
                    /*
                     * Strange behavior: createContext sometimes fails.
                     * Cannot locate a certain reason. Cannot reproduce in a simplified application.
                     * But seems caused by calling some opengl functions too early, so redraw after fail will help.
                    */
                    //qDebug()<<"init fail";
                });
            }
        }, Qt::QueuedConnection);
    qDebug()<<"Creating Main Window";
    this->load(main_url);
	qDebug()<<"Creating Main Window Done";
}
