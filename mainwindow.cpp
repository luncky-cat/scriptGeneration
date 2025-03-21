#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include<QFileDialog>
#include <QTextStream>
#include <QProcess>
#include <QStandardItemModel>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_selectAppPathBtn_clicked()
{
    mainDirectory = QFileDialog::getExistingDirectory(this, tr("Open Directory"),"/",QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);

    if(mainDirectory.isEmpty()){
        return;
    }

    listFilesAndDirectories(mainDirectory, mainFiles, mainDirs);  // 获取主目录下的所有文件和文件夹的路径

    qDebug() << "主目录下的文件夹\n";
    for (const QString &dirPath : mainDirs)
    {
        qDebug() << dirPath<<"\n";
    }

    qDebug() << "主目录下的文件\n";
    for (const QString &filePath : mainFiles)
    {
        qDebug() << filePath<<"\n";
    }

    //获取程序名
    for (const QString &filePath : mainFiles) {
        QFileInfo fileInfo(filePath);
        QString suffix = fileInfo.suffix(); // 获取后缀名
        if (suffix.compare("exe", Qt::CaseInsensitive) == 0) {
            exeName=fileInfo.baseName(); // 获取不带后缀的文件名;
            break;
        }
    }

    // 输出文件名和文件路径
    qDebug() << "文件名:" << exeName;

    ui->appPathLineE->setText(mainDirectory);

    setupFileTreeView(ui->fileTreeView, mainDirectory);   //设置文件树
}


void MainWindow::on_buildBtn_clicked()    //生成
{
    if(mainDirectory.isEmpty()){   //不存在路径
        return;
    }

    QString filePath =mainDirectory+"\\make.nsi";    // 主路径下生成脚本

    QFile file(filePath);   // 创建 QFile 对象

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::System);  // 使用系统默认编码（通常是 ANSI）

    // 设置输出文件名
    out << "Outfile \"" <<installerPath<< exeName << "_Installer.exe\""<<"\n\n";

    // 设置安装目录
    out << "InstallDir \"$PROGRAMFILES\\" << exeName << "\""<<"\n\n";

    // 在注册表中记录安装路径
    out << "InstallDirRegKey HKLM \"Software\\" << exeName << "\" \"Install_Dir\""<<"\n\n";

    out<<"Page directory"<<"\n\n";

    out<<"Page instfiles"<<"\n\n";

    out<<"Section \"Install\""<<"\n\n";

    out<<"SetOutPath \"$INSTDIR\""<<"\n\n";


    for (const QString &filePath : mainFiles) {   //复制程序
        QFileInfo fileInfo(filePath);
        if (fileInfo.suffix().compare("exe", Qt::CaseInsensitive) == 0) {
            out << "File \"" <<filePath<<"\"\n\n";
        }
    }

    for (const QString &filePath : mainFiles) {    //复制dll
        QFileInfo fileInfo(filePath);
        if (fileInfo.suffix().compare("dll", Qt::CaseInsensitive) == 0) {
            out << "File \"" << filePath<<"\"\n\n";
        }
    }

    for(const QString &dirPath:mainDirs){
        out <<"SetOutPath "<<"\"$INSTDIR\\"<< QFileInfo(dirPath).fileName()<<"\"\n\n";      //主文件下的目录
        QDir subDir(dirPath);
        QFileInfoList subDirFiles = subDir.entryInfoList(QDir::Files);
        for (QFileInfo &subFileInfo : subDirFiles) {
            if (subFileInfo.suffix().compare("dll", Qt::CaseInsensitive) == 0) {
                out << "File \"" << subFileInfo.absoluteFilePath().replace('/','\\') <<"\"\n\n";
            }
        }
    }

    out << "WriteRegStr HKLM \"Software\\"<<exeName<<"\" \"Install_Dir\" \"$INSTDIR\"\n\n";

    out << "CreateShortCut \"$DESKTOP\\"<<exeName<<".lnk\" \"$INSTDIR\\"<<exeName<<".exe\"\n\n";

    out << "CreateDirectory \"$SMPROGRAMS\\"<<exeName<<"\"\n\n";

    out << "CreateShortCut \"$SMPROGRAMS\\"<<exeName<<"\\"<<exeName<<".lnk\" \"$INSTDIR\\"<<exeName<<".exe\"\n\n";

    out << "WriteUninstaller \"$INSTDIR\\uninstall.exe\"\n\n";

    out<<"SectionEnd\n\n";

    out<<"Section \"Uninstall\""<<"\n\n";

    out<<"ExecWait 'taskkill /F /IM "<<exeName<<".exe /T'"<<"\n\n";

    out<<"Sleep 1000"<<"\n\n";

    out<<"System::Call 'kernel32::OpenProcess(i 0x400, i 0, i $INSTDIR\\"<<exeName<<".exe) i .r0'"<<"\n\n";

    out<<"StrCmp $0 0 done"<<"\n\n";

    out<<"MessageBox MB_OK \""<<exeName<<"仍在运行，请先关闭程序后再继续卸载。\""<<"\n\n";

    out<<"Abort\n\n";

    out<<"done:\n\n";

    out <<"Delete \"$INSTDIR\\"<<exeName<<".exe\"\n\n";//移除程序

    for (const QString &filePath : mainFiles) {   //移除dll
        QFileInfo fileInfo(filePath);
        if (fileInfo.suffix().compare("dll", Qt::CaseInsensitive) == 0) {
            out <<"Delete \"$INSTDIR\\"<<fileInfo.fileName()<<"\""<<"\n\n";
        }
    }

    for(const QString &dirPath:mainDirs){   //移除文件夹
        out<<"RMDir /r \"$INSTDIR\\"<<QFileInfo(dirPath).fileName()<<"\"\n\n";
    }

    out<<"Delete \"$INSTDIR\\uninstall.exe\""<<"\n\n";

    out<<"RMDir \"$INSTDIR\"\n\n";

    out<<"Delete \"$DESKTOP\\"<<exeName<<".lnk\"\n\n";

    out<<"Delete \"$SMPROGRAMS\\"<<exeName<<"\\"<<exeName<<".lnk\""<<"\n\n";

    out<<"RMDir \"$SMPROGRAMS\\"<<exeName<<"\""<<"\n\n";

    out<<"DeleteRegKey HKCU \"Software\\"<<exeName<<"\"\n\n";

    out<<"SectionEnd"<<"\n\n";

    file.close();
}


void MainWindow::on_packBtn_clicked()
{
    on_buildBtn_clicked();//生成脚本
    QString nsisCompiler = QDir::cleanPath(QCoreApplication::applicationDirPath() + "/nsis/makensis.exe");

    QString scriptPath = QDir::toNativeSeparators(mainDirectory + "/make.nsi");

    // 确保 makensis.exe 存在
    if (!QFileInfo::exists(nsisCompiler)) {
        qDebug() << "NSIS Compiler not found at:" << nsisCompiler;
        return;
    }

    // 传递参数
    QStringList arguments;
    arguments << "/V4" << "/DMyVar=QtInstaller" << scriptPath;

    // 运行 makensis.exe
    QProcess process;
    process.start(nsisCompiler, arguments);

    if (!process.waitForStarted()) {
        qDebug() << "Failed to start NSIS Compiler!";
        qDebug() << "Error String: " << process.errorString();
        return;
    }

    process.waitForFinished();

    // 获取输出
    QString output = process.readAllStandardOutput();
    QString errorOutput = process.readAllStandardError();
    qDebug() << "NSIS Output:\n" << output;
    if (!errorOutput.isEmpty()) {
        qDebug() << "NSIS Errors:\n" << errorOutput;
    }
}

// 函数：获取指定目录下的所有文件和文件夹路径，并保存
void MainWindow::listFilesAndDirectories(const QString &directoryPath, QStringList &fileList, QStringList &dirList)
{
    QDir dir(directoryPath);
    // 获取目录下的所有条目（文件和文件夹），不包括 "." 和 ".."
    QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
    for (const QFileInfo &entry : entries)
    {
        if (entry.isDir())
        {
            dirList.append(entry.absoluteFilePath().replace('/','\\'));
        }
        else if (entry.isFile())
        {
            fileList.append(entry.absoluteFilePath().replace('/','\\'));
        }
    }
}

void listFilesAndDirectoriesA(const QString &path, QStringList &filelist, QStringList &dirlist) {
    QDir dir(path);
    if (!dir.exists()) return;

    QFileInfoList list = dir.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot);
    for (const QFileInfo &info : list) {
        if (info.isDir()) {
            dirlist.append(info.filePath());  // 绝对路径
        } else {
            filelist.append(info.fileName()); // 仅文件名
        }
    }
}

void MainWindow::populateTreeItem(QStandardItem *parentItem, const QString &directory) {
    QStringList filelist;
    QStringList dirlist;
    listFilesAndDirectories(directory, filelist, dirlist);

    // 递归添加子目录
    for (const QString &dir : dirlist) {
        QStandardItem *dirItem = new QStandardItem(QFileInfo(dir).fileName());
        parentItem->appendRow(dirItem);
        populateTreeItem(dirItem, dir);  // 递归填充子目录
    }

    // 添加文件
    for (const QString &file : filelist) {
        QStandardItem *fileItem = new QStandardItem(QFileInfo(file).fileName());
        parentItem->appendRow(fileItem);
    }
}

void MainWindow::setupFileTreeView(QTreeView *fileTreeView, const QString &mainDirectory) {
    // 创建模型
    QStandardItemModel *model = new QStandardItemModel();
    model->setHorizontalHeaderLabels(QStringList() << "项目结构");

    // 创建根节点
    QStandardItem *rootItem = new QStandardItem(QFileInfo(mainDirectory).fileName());
    model->appendRow(rootItem);

    // 递归填充目录结构
    populateTreeItem(rootItem, mainDirectory);

    // 设置模型到 TreeView
    fileTreeView->setModel(model);
    fileTreeView->expand(rootItem->index());
}

void MainWindow::on_selectInstallPathBtn_clicked()   //设置导出的路径
{

    installerPath.clear();
    installerPath = QFileDialog::getExistingDirectory(this, tr("Open Directory"),"/",QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);

    if(installerPath.isEmpty()){
        return;
    }

    ui->installPathLineE->setText(installerPath);
    installerPath.replace('/','\\');
    installerPath+="\\";
}

