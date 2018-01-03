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

#include "lz4.h"
#include "VRCAUnpackerApp.h"

class ArchiveBlockInfo {
public:
    ArchiveBlockInfo(uint64_t uncompressedSize, uint64_t compressedSize, uint64_t flags) :
        uncompressedSize(uncompressedSize),
        compressedSize(compressedSize),
        flags(flags) { }

    uint64_t uncompressedSize;
    uint64_t compressedSize;
    uint64_t flags;
};

class NodeInfo {
public:
    NodeInfo(uint64_t offset, uint64_t uncompressedSize, uint64_t flags, QString name) :
        offset(offset),
        uncompressedSize(uncompressedSize),
        flags(flags),
        name(name) { }

    uint64_t offset;
    uint64_t uncompressedSize;
    uint64_t flags;
    QString name;
};


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
    int cursor { 0 };
    bool success { false };

    QString signature = unpackString0(vrcaBlob, cursor, success);
    if (!success || signature != "UnityFS") {
        qWarning() << "Signature doesn't match: " << signature;
        return false;
    }

    int endian = unpackDWordBigEnd(vrcaBlob, cursor, success);
    if (!success || endian != 6) {
        qWarning() << "can't handle non-big-endian ordering:" << endian;
        return false;
    }

    QString minPlayerVersion = unpackString0(vrcaBlob, cursor, success);
    if (!success) {
        qWarning() << "failed to parse minimum player version";
    }
    qDebug() << "minPlayerVersion =" << minPlayerVersion;

    QString fileEngineVersion = unpackString0(vrcaBlob, cursor, success);
    if (!success) {
        qWarning() << "failed to parse engine version";
    }
    qDebug() << "fileEngineVersion =" << fileEngineVersion;

    uint64_t totalFileSize = unpackQWordBigEnd(vrcaBlob, cursor, success);
    if (!success) {
        qWarning() << "failed to parse total file size";
    }
    qDebug() << "totalFileSize =" << totalFileSize;
    if (totalFileSize != (uint64_t)vrcaBlob.size()) {
        qDebug() << "totalFileSize doesn't match: " << vrcaBlob.size();
        return false;
    }

    uint64_t ciBlockSize = unpackDWordBigEnd(vrcaBlob, cursor, success); // compressed size
    if (!success) {
        qWarning() << "failed to parse compressed size";
        return false;
    }
    qDebug() << "ciBlockSize =" << ciBlockSize;

    uint64_t uiBlockSize = unpackDWordBigEnd(vrcaBlob, cursor, success); // decompressed size
    if (!success) {
        qWarning() << "failed to parse decompressed size";
        return false;
    }
    qDebug() << "uiBlockSize =" << uiBlockSize;

    uint64_t flags = unpackDWordBigEnd(vrcaBlob, cursor, success);
    if (!success) {
        qWarning() << "failed to parse decompressed size";
        return false;
    }

    int compressionMode = (flags & 0x3f); // NONE = 0, LZMA = 1, LZ4 = 2, LZ4HC = 3, LZHAM = 4
    bool containsDirectory = (flags & 0x40) > 0;
    bool directoryAtEnd = (flags & 0x80) > 0;

    qDebug() << "flags compression-mode =" << compressionMode;
    qDebug() << "flags contains directory =" << (containsDirectory ? "yes" : "no");
    qDebug() << "flags directory at end =" << (directoryAtEnd ? "yes" : "no");

    if (!containsDirectory) {
        qWarning() << "unable to handle files with no directory";
        return false;
    }

    if (directoryAtEnd) {
        qWarning() << "unable to handle files with directory at end";
    }

    QByteArray comped = vrcaBlob.mid(cursor, ciBlockSize);
    cursor += ciBlockSize;
    // QByteArray decomped = decompressL4Z(comped, success);
    // qDebug() << "decomped.size() =" << decomped.size();

    QByteArray directoryData(uiBlockSize, 0);
    int lz4Result = LZ4_decompress_safe_partial(comped.data(), directoryData.data(), ciBlockSize, uiBlockSize, uiBlockSize);
    qDebug() << "lz4Result =" << lz4Result;

    if ((uint64_t)lz4Result != uiBlockSize) {
        qWarning() << "lz4 decompression failed:" << lz4Result << uiBlockSize;
        return false;
    }

    int directoryCursor = 0;

    QUuid id = unpackUUID(directoryData, directoryCursor, success);
    if (!success) {
        qWarning() << "failed to parse id";
        return false;
    }
    qDebug() << "id =" << id;


    int numBlocks = (int)unpackDWordBigEnd(directoryData, directoryCursor, success);
    if (!success) {
        qWarning() << "failed to parse decompressed size";
        return false;
    }
    qDebug() << "numBlocks =" << numBlocks;

    std::list<ArchiveBlockInfo> blockInfos;
    for (int i = 0; i < numBlocks; i++) {
        uint64_t blockUncompressedSize = unpackDWordBigEnd(directoryData, directoryCursor, success);
        if (!success) {
            qDebug() << "failed to get blockUncompressedSize for" << i;
            return false;
        }
        uint64_t blockCompressedSize = unpackDWordBigEnd(directoryData, directoryCursor, success);
        if (!success) {
            qDebug() << "failed to get blockCompressedSize for" << i;
            return false;
        }
        uint64_t blockFlags = unpackWordBigEnd(directoryData, directoryCursor, success);
        if (!success) {
            qDebug() << "failed to get blockFlags for" << i;
            return false;
        }

        qDebug() << "block -- compressed =" << blockCompressedSize
                 << "uncompressed =" << blockUncompressedSize
                 << "flags =" << blockFlags;

        blockInfos.push_back(ArchiveBlockInfo(blockUncompressedSize, blockCompressedSize, blockFlags));
    }


    int numNodes = (int)unpackDWordBigEnd(directoryData, directoryCursor, success);
    if (!success) {
        qWarning() << "failed to parse decompressed size";
        return false;
    }
    qDebug() << "numNodes =" << numNodes;

    std::list<NodeInfo> nodeInfos;
    for (int i = 0; i < numNodes; i++) {
        uint64_t nodeOffset = unpackQWordBigEnd(directoryData, directoryCursor, success);
        if (!success) {
            qDebug() << "failed to get nodeOffset for" << i;
            return false;
        }
        uint64_t nodeUncompressedSize = unpackQWordBigEnd(directoryData, directoryCursor, success);
        if (!success) {
            qDebug() << "failed to get nodeUncompressedSize for" << i;
            return false;
        }
        uint64_t nodeFlags = unpackDWordBigEnd(directoryData, directoryCursor, success);
        if (!success) {
            qDebug() << "failed to get nodeFlags for" << i;
            return false;
        }
        QString nodeName = unpackString0(directoryData, directoryCursor, success);
        if (!success) {
            qWarning() << "failed to nodeName for" << i;
        }

        qDebug() << "node -- offset =" << nodeOffset
                 << "uncompressed =" << nodeUncompressedSize
                 << "flags =" << nodeFlags
                 << "name =" << nodeName;

        nodeInfos.push_back(NodeInfo(nodeOffset, nodeUncompressedSize, nodeFlags, nodeName));
    }

    qDebug() << "directoryCursor =" << directoryCursor;
    qDebug() << "cursor =" << cursor;

    QByteArray blockStorage;
    for (ArchiveBlockInfo& blockInfo : blockInfos) {
        qDebug() << "-------- block, compressedSize =" << blockInfo.compressedSize
                 << "uncompressedSize =" << blockInfo.uncompressedSize
                 << "blockInfo.flags =" << blockInfo.flags;
        int compressionType = blockInfo.flags & 0x3f;
        if (blockInfo.compressedSize != blockInfo.uncompressedSize) {
            qDebug() << "block compression type =" << compressionType;

            QByteArray dst(blockInfo.uncompressedSize, 0);
            QByteArray comped = vrcaBlob.mid(cursor, blockInfo.compressedSize);


            if (compressionType == 1) { // LZMA
                // XXX
            } else if (compressionType == 2 || compressionType == 3) { // LZ4 || LZ4HC
                int lz4Result = LZ4_decompress_safe_partial(comped.data(), dst.data(), blockInfo.compressedSize,
                                                            blockInfo.uncompressedSize, blockInfo.uncompressedSize);
                qDebug() << "lz4Result = " << lz4Result << " vs " << blockInfo.uncompressedSize;
                blockStorage.append(dst);
            } else if (compressedSize == 4) { // LZHAM
                // XXX
            }
        } else {
            blockStorage.append(vrcaBlob.mid(cursor, blockInfo.uncompressedSize));
        }
        cursor += blockInfo.compressedSize;
    }

    return true;
}


QString VRCAUnpackerApp::unpackString0(QByteArray vrcaBlob, int& cursor, bool& success) {
    QString result;

    for (;
         cursor < vrcaBlob.size() && vrcaBlob.at(cursor) != '\0';
         cursor++) {
        result.append(vrcaBlob.at(cursor));
    }

    if (cursor < vrcaBlob.size() && vrcaBlob.at(cursor) == '\0') {
        cursor++; // step over null terminator
    }

    success = true;
    return result;
}

uint64_t VRCAUnpackerApp::unpackWordBigEnd(QByteArray vrcaBlob, int& cursor, bool& success) {
    uint64_t result { 0 };

    if (cursor + 2 >= vrcaBlob.size()) {
        success = false;
        return result;
    }

    unsigned char* v = (unsigned char*)vrcaBlob.data() + cursor;
    result =
        ((uint64_t)v[0] << 8) +
        ((uint64_t)v[1] << 0);

    success = true;
    cursor += 2;
    return result;
}

uint64_t VRCAUnpackerApp::unpackDWordBigEnd(QByteArray vrcaBlob, int& cursor, bool& success) {
    uint64_t result { 0 };

    if (cursor + 4 >= vrcaBlob.size()) {
        success = false;
        return result;
    }

    unsigned char* v = (unsigned char*)vrcaBlob.data() + cursor;
    result =
        ((uint64_t)v[0] << 24) +
        ((uint64_t)v[1] << 16) +
        ((uint64_t)v[2] << 8) +
        ((uint64_t)v[3] << 0);

    success = true;
    cursor += 4;
    return result;
}

uint64_t VRCAUnpackerApp::unpackQWordBigEnd(QByteArray vrcaBlob, int& cursor, bool& success) {
    uint64_t result { 0 };

    if (cursor + 8 >= vrcaBlob.size()) {
        success = false;
        return result;
    }

    unsigned char* v = (unsigned char*)vrcaBlob.data() + cursor;
    result =
        ((uint64_t)v[0] << 56) +
        ((uint64_t)v[1] << 48) +
        ((uint64_t)v[2] << 40) +
        ((uint64_t)v[3] << 32) +
        ((uint64_t)v[4] << 24) +
        ((uint64_t)v[5] << 16) +
        ((uint64_t)v[6] << 8) +
        ((uint64_t)v[7] << 0);

    success = true;
    cursor += 8;
    return result;
}

QUuid VRCAUnpackerApp::unpackUUID(QByteArray vrcaBlob, int& cursor, bool& success) {
    // XXX is this right?
    QUuid result = QUuid::fromRfc4122(vrcaBlob.mid(cursor, 16));
    cursor += 16;
    return result;
}
