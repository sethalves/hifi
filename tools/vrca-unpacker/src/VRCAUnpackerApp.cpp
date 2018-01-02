//
//  VRCAUnpackerApp.cpp
//  tools/vrca-unpacker/src
//
//  Created by Seth Alves on 2018-1-2
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QDataStream>
#include <QTextStream>
#include <QThread>
#include <QFile>
#include <QLoggingCategory>
#include <QCommandLineParser>

#include <NetworkLogging.h>
#include <NetworkingConstants.h>
#include <SharedLogging.h>
#include <AddressManager.h>
#include <DependencyManager.h>
#include <SettingHandle.h>
#include <AssetUpload.h>
#include <StatTracker.h>

#include "VRCAUnpackerApp.h"

VRCAUnpackerApp::VRCAUnpackerApp(int argc, char* argv[]) :
    QCoreApplication(argc, argv)
{
    // parse command-line
    QCommandLineParser parser;
    parser.setApplicationDescription("High Fidelity vrca-unpacker");

    const QCommandLineOption helpOption = parser.addHelpOption();

    const QCommandLineOption verboseOutput("v", "verbose output");
    parser.addOption(verboseOutput);

    if (!parser.parse(QCoreApplication::arguments())) {
        qCritical() << parser.errorText() << endl;
        parser.showHelp();
        Q_UNREACHABLE();
    }

    if (parser.isSet(helpOption)) {
        parser.showHelp();
        Q_UNREACHABLE();
    }

    _verbose = parser.isSet(verboseOutput);
    if (!_verbose) {
        QLoggingCategory::setFilterRules("qt.network.ssl.warning=false");

        const_cast<QLoggingCategory*>(&networking())->setEnabled(QtDebugMsg, false);
        const_cast<QLoggingCategory*>(&networking())->setEnabled(QtInfoMsg, false);
        const_cast<QLoggingCategory*>(&networking())->setEnabled(QtWarningMsg, false);

        const_cast<QLoggingCategory*>(&shared())->setEnabled(QtDebugMsg, false);
        const_cast<QLoggingCategory*>(&shared())->setEnabled(QtInfoMsg, false);
        const_cast<QLoggingCategory*>(&shared())->setEnabled(QtWarningMsg, false);
    }

    QStringList posArgs = parser.positionalArguments();
    if (posArgs.size() != 1) {
        qDebug() << "vrca input filename";
        parser.showHelp();
        Q_UNREACHABLE();
    }

    bool success = unpackVRCAFile(posArgs[0]);

    QCoreApplication::exit(success ? 0 : -1);
}

VRCAUnpackerApp::~VRCAUnpackerApp() {
}

bool VRCAUnpackerApp::unpackVRCAFile(QString filename) {
    QFile inputHandle(filename);
    if (inputHandle.open(QIODevice::ReadOnly)) {
        QByteArray vrcaBlob = inputHandle.readAll();
        return unpackVRCAByteArray(vrcaBlob);
    }
    qWarning() << "Unable to open file" << filename << "for reading";
    return false;
}

bool VRCAUnpackerApp::unpackVRCAByteArray(QByteArray vrcaBlob) {

        QByteArray::at() returns a (signed) char and you assign it to an unsigned short. You want the value 0xbe to be treated as unsigned, so you should cast it : c3 = (unsigned char) iContents.at(2);

    return false;
}
