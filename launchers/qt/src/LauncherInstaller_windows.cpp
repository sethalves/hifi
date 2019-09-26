#include "LauncherInstaller_windows.h"
#include "Helper.h"

#include <string>

#include <QStandardPaths>
#include <QFileInfo>
#include <QFile>
#include <QDebug>

LauncherInstaller::LauncherInstaller(const QString& applicationFilePath) {
    _launcherInstallDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation) + "/HQ";
    qDebug() << "Launcher install dir: " << _launcherInstallDir.absolutePath();
    _launcherInstallDir.mkpath(_launcherInstallDir.absolutePath());
    QFileInfo fileInfo(applicationFilePath);
    _launcherRunningFilePath = fileInfo.absoluteFilePath();
    _launcherRunningDirPath = fileInfo.absoluteDir().absolutePath();
    qDebug() << "Launcher running file path: " << _launcherRunningFilePath;
    qDebug() << "Launcher running dir path: " << _launcherRunningDirPath;
}

bool LauncherInstaller::runningOutsideOfInstallDir() {
    return (QString::compare(_launcherInstallDir.absolutePath(), _launcherRunningDirPath) != 0);
}

void LauncherInstaller::install() {
    if (runningOutsideOfInstallDir()) {
        qDebug() << "Installing HQ Launcher....";
        QString oldLauncherPath = _launcherInstallDir.absolutePath() + "/HQ Launcher.exe";

        if (QFile::exists(oldLauncherPath)) {
            bool didRemove = QFile::remove(oldLauncherPath);
            qDebug() << "did remove file: " << didRemove;
        }
        bool success = QFile::copy(_launcherRunningFilePath, oldLauncherPath);
        if (success) {
            qDebug() << "successful";
        } else {
            qDebug() << "not successful";
        }

        qDebug() << "LauncherInstaller: create uninstall link";
        QString uninstallLinkPath = _launcherInstallDir.absolutePath() + "/Uninstall HQ.link";
        if (QFile::exists(uninstallLinkPath)) {
            QFile::remove(uninstallLinkPath);
        }

        createSymbolicLink((LPCSTR)oldLauncherPath.toStdString().c_str(), (LPCSTR)uninstallLinkPath.toStdString().c_str(),
                           (LPCSTR)("Click to Uninstall HQ"), (LPCSTR)("--uninstall"));
    }
}

void LauncherInstaller::uninstall() {}
