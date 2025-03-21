#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStandardItemModel>
#include <QTreeView>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_selectAppPathBtn_clicked();
    void on_buildBtn_clicked();
    void on_packBtn_clicked();

    void on_selectInstallPathBtn_clicked();

private:
    Ui::MainWindow *ui;
    void listFilesAndDirectories(const QString &directoryPath, QStringList &fileList, QStringList &dirList);
    //void listFilesInDirectory(const QString &directoryPath, QStringList &fileList);
    QString mainDirectory;  //主文件夹
    QStringList mainFiles;  //主文件夹下的文件
    QStringList mainDirs;  //主文件下的目录
    QString exeName;//程序名字
    QString installerPath;//安装器导出路径
    //void setupFileTreeView(QTreeView *fileTreeView, const QString &mainDirectory, const QStringList &mainDirs, const QStringList &mainFiles);
    void setupFileTreeView(QTreeView *fileTreeView, const QString &mainDirectory);
    void listFilesAndDirectoriesA(const QString &directoryPath, QStringList &fileList, QStringList &dirList);
    void populateTreeItem(QStandardItem *parentItem, const QString &directory);
};
#endif // MAINWINDOW_H
