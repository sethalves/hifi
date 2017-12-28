//
//  FBXDumpApp.cpp
//  tools/fbx-dump/src
//
//  Created by Seth Alves on 2017-12-24
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QtGlobal>
#include <QDataStream>
#include <QTextStream>
#include <QThread>
#include <QFile>
#include <QLoggingCategory>
#include <QCommandLineParser>

#include <SharedLogging.h>
#include <gpu/GPULogging.h>
#include <ModelFormatLogging.h>
#include <FBXReader.h>
#include "FBXDumpApp.h"

QTextStream& qStdOut()
{
    static QTextStream ts(stdout);
    return ts;
}


FBXDumpApp::FBXDumpApp(int argc, char* argv[]) :
    QCoreApplication(argc, argv)
{
    // parse command-line
    QCommandLineParser parser;
    parser.setApplicationDescription("High Fidelity Fbx-Dump");

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

        const_cast<QLoggingCategory*>(&gpulogging())->setEnabled(QtDebugMsg, false);
        const_cast<QLoggingCategory*>(&gpulogging())->setEnabled(QtInfoMsg, false);
        const_cast<QLoggingCategory*>(&gpulogging())->setEnabled(QtWarningMsg, false);

        const_cast<QLoggingCategory*>(&modelformat())->setEnabled(QtDebugMsg, false);
        const_cast<QLoggingCategory*>(&modelformat())->setEnabled(QtInfoMsg, false);
        const_cast<QLoggingCategory*>(&modelformat())->setEnabled(QtWarningMsg, false);
    }

    QStringList posArgs = parser.positionalArguments();
    if (posArgs.size() != 1) {
        qDebug() << "give filename argument";
        parser.showHelp();
        Q_UNREACHABLE();
    }

    QString inputFilename = posArgs[0];
    if (FBXReadFile(inputFilename)) {
        FBXDump();
    }

    QCoreApplication::quit();
}


FBXDumpApp::~FBXDumpApp() {
}


bool FBXDumpApp::FBXReadFile(QString inputFilename) {
    QFile fbxFile(inputFilename);
    if (!fbxFile.open(QIODevice::ReadOnly)) {
        qWarning() << "Error opening " + inputFilename + " for reading";
        return false;
    }

    FBXReader reader;

    _rootNode = reader._rootNode = reader.parseFBX(&fbxFile);
    _geometry = reader.extractFBXGeometry({}, inputFilename);
    _textureContent = reader._textureContent;

    return true;
}

void FBXDumpApp::FBXDump() {
    FBXDumpNode(_rootNode, 0);
}

void FBXDumpApp::FBXDumpNode(FBXNode& node, int depth) {
    qStdOut() << QString(depth * 4, ' ') << QString(node.name) << ":";
    for (QVariant& property : node.properties) {
        qStdOut() << QString(" ") << property.toString();
    }
    qStdOut() << endl;

    for (FBXNode& childNode : node.children) {
        FBXDumpNode(childNode, depth + 1);
    }
}
