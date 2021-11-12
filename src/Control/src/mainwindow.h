#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QList>
#include <QPair>
#include <boost/filesystem.hpp>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_pb_removeal_clicked();

private slots:
    void on_pb_ImportTheme_clicked();

private slots:
    void on_pb_DeleteTheme_clicked();

private slots:
    void on_pb_ApplyTheme_clicked();

private slots:
    void on_pb_ApplySecureDesktop_clicked();

private slots:
    void on_pb_ApplyAdvancedSettings_clicked();

private:
    void reloadBasicSettings();
    void reloadAdvancedSettings();
    void reloadSecureDesktopSetting();
    void reloadAllowlist();
    Ui::MainWindow *ui;
    QStringList themes;
    boost::filesystem::path installation;
    std::vector<std::vector<unsigned char>>allowlist_data;
};
#endif // MAINWINDOW_H
