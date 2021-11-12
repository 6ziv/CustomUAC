#include <QGuiApplication>
#include <QApplication>
#include <QResource>
#include <QFile>
#include <QQuickWindow>
#include <QOpenGLContext>
#include <QSGRendererInterface>
#include <QTranslator>
#include <processenv.h>
#include <QQuickRenderControl>
#include "uacparser.h"
#include "uacdata.h"
#include "securedesktop.h"
#include "urlinterceptor.h"
#include "allowlist.h"
#include "settings.h"
#include "applicationengine.h"
#include <QOpenGLVersionProfile>
#include <QSurfaceFormat>
#include <QLibrary>
#include <QEventLoop>
#include "emergencyhandler.h"
int main(int argc, char *argv[])
{
    SetUnhandledExceptionFilter(ExceptionHandler);

    parse_args(argc,argv);
    argc=4;
    
    char env[] = "";
    SetEnvironmentStringsA(env);
    UACParser parser(argc, argv);
    if (!parser.is_ok) {
        ExitProcess(1223);
    }

    Settings settings;
    if (error_level < 2) {
        settings.load();
        auto allowlist = settings.get_allowlist();
        if (parser.is_admin && !allowlist.empty()) {
            set_db(allowlist);
            if (check_allowlist(parser.path))parser.accept(L"", L"");
        }

        if (settings.prompt_on_secure_desktop) {

            if (error_level < 1) {
                WaitForSingleObject(CreateMutexA(NULL, FALSE, "CustomUAC"), INFINITE);
            }

            SecureDesktop::instance().switch_desktop();
        }

        QApplication::setAttribute(Qt::AA_UseSoftwareOpenGL);
    }

	QApplication app(argc, argv);

    /*
    * D3D11 is known to give faulty results on winlogon desktop during development.
    * So we prefer software opengl.
    * When software opengl is not available, the application will automatically fall back to software rendering.
	* On user desktop, use default to get best performance (as I personally believe that Qt stuff have tested enough on this).
    */

    if(error_level<2 && settings.prompt_on_secure_desktop){
		//QOpenGLContext context;
		//context.create();
		
		/*
		 * In later mesa versions, context.create() does not work with a default surface format, and always returns NULL.
		 * so directly detect the dll.
		 */
		bool have_opengl = false;
		if(!settings.get_softgl().empty()){
			QLibrary lib(QString::fromStdWString(settings.get_softgl()));
			if (lib.load()) {
				//Do not unload so that Windows loads this directly without searching.
				//destroying QLibrary will not free the lib.
				have_opengl=true;
			}
		}
		if(have_opengl){
			QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);
            UACPermissions::get()->opengl = true;
		}
        else {
            QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
            UACPermissions::get()->opengl = false;
        }
    }
    if(error_level>=2){
        QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
        UACPermissions::get()->opengl = false;
        UACPermissions::get()->network = false;
        UACPermissions::get()->local_storage = false;
    }
    bool theme_loaded = false;
    if(error_level<1){
        auto theme = settings.get_theme();
        if (!theme.empty()) {
            if(QFile::exists(QString::fromStdWString(theme)))
                theme_loaded = QResource::registerResource(QString::fromStdWString(theme), "/uac-content");
        };
    }
    if(!theme_loaded)
        theme_loaded = QResource::registerResource(":/uac-internal/default.rcc", "/uac-content");
    if(error_level<2){
        QTranslator* translator=new QTranslator;
        if (translator->load(QLocale(), QString(), QString(), ":/uac-content/translations", QString()))
            app.installTranslator(translator);
        new ApplicationEngine(settings.network, settings.prompt_on_secure_desktop, parser, settings.get_local_storage());
    }else{
        new ApplicationEngine(false, false, parser, L"<**NO LOCALSTORAGE IN SAFE MODE**>");
    }
    app.exec();
    //control never reaches here.
    return 1223;
}
