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
#include "lzma.h"
#include "VRCAUnpackerApp.h"

QByteArray lzmaUncompress(QByteArray data, uint64_t uncompressedSize, uint64_t props, uint64_t dictSize, bool& success) {
    QByteArray result(uncompressedSize, 0);
    uint32_t preset_number = LZMA_PRESET_DEFAULT;
    lzma_options_lzma opt_lzma;
    lzma_filter filters[LZMA_FILTERS_MAX + 1];
    lzma_stream strm = LZMA_STREAM_INIT;
    lzma_ret ret_xz;
    // uint32_t flags = LZMA_TELL_UNSUPPORTED_CHECK | LZMA_CONCATENATED;

    if (lzma_lzma_preset(&opt_lzma, preset_number)) {
        qDebug() << "lzma_lzma_preset failed";
    }

    opt_lzma.dict_size = dictSize;
    opt_lzma.lc = props % 9;
    props = props / 9;
    opt_lzma.pb = props / 5;
    opt_lzma.lp = props % 5;

    qDebug() << "dict_size=" << dictSize << "lc=" << opt_lzma.lc << "lp=" << opt_lzma.lp << "pb=" << opt_lzma.pb;


    filters[0].id = LZMA_FILTER_LZMA1;
    filters[0].options = &opt_lzma;
    filters[1].id = LZMA_VLI_UNKNOWN;

    strm.next_in = (unsigned char*)data.data();
    strm.avail_in = data.size();

    ret_xz = lzma_raw_decoder(&strm, filters);
    if (ret_xz != 0) {
        qWarning() << "lzma_raw_decoder ret_xz =" << ret_xz;
        success = false;
        return result;
    }

    strm.next_out = (unsigned char*)result.data();
    strm.avail_out = result.size();

    ret_xz = lzma_code(&strm, LZMA_FINISH);
    qDebug() << "lzma_code ret_xz =" << ret_xz;

    success = (ret_xz == 1); // LZMA_STREAM_END -- all the data was successfully decoded

    lzma_end(&strm);

    return result;
}


// https://stackoverflow.com/questions/6486745/c-lzma-compression-and-decompression-of-large-stream-by-parts#
QByteArray lzmaUncompressX(QByteArray data) {
    static const int OUT_BUF_MAX { 409600 };
    lzma_stream strm = LZMA_STREAM_INIT; /* alloc and init lzma_stream struct */
    const uint32_t flags = LZMA_TELL_NO_CHECK; // LZMA_TELL_UNSUPPORTED_CHECK | LZMA_CONCATENATED;
    const uint64_t memory_limit = UINT64_MAX; /* no memory limit */
    uint8_t *in_buf;
    uint8_t out_buf[OUT_BUF_MAX];
    size_t in_len; /* length of useful data in in_buf */
    size_t out_len; /* length of useful data in out_buf */
    lzma_ret ret_xz;
    QByteArray arr;

    ret_xz = lzma_auto_decoder (&strm, memory_limit, flags);
    qDebug() << "ret_xz =" << ret_xz;
    if (ret_xz != LZMA_OK) {
        qDebug() << "lzma failed";
        return QByteArray();
    }

    in_len = data.size();
    in_buf = (uint8_t*)data.data();

    strm.next_in = in_buf;
    strm.avail_in = in_len;
    do {
        strm.next_out = out_buf;
        strm.avail_out = OUT_BUF_MAX;
        ret_xz = lzma_code (&strm, LZMA_FINISH);

        out_len = OUT_BUF_MAX - strm.avail_out;
        qDebug() << "in loop, ret_xz =" << ret_xz << "out_len =" << out_len;
        arr.append((char*)out_buf, out_len);
        out_buf[0] = 0;
    } while (strm.avail_out == 0);
    lzma_end (&strm);
    return arr;
}



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

            if (compressionType == 1) { // LZMA
                uint64_t props = unpackByte(vrcaBlob, cursor, success);
                if (!success) {
                    qWarning() << "can't get props in lzma compressed block";
                    return false;
                }
                uint64_t dictSize = unpackDWordLittleEnd(vrcaBlob, cursor, success);
                if (!success) {
                    qWarning() << "can't get dictSize in lzma compressed block";
                    return false;
                }
                QByteArray comped = vrcaBlob.mid(cursor, blockInfo.compressedSize - 5);
                qDebug() << "comped.size() ==" << comped.size();
                cursor += blockInfo.compressedSize - 5;
                QByteArray dst = lzmaUncompress(comped, blockInfo.uncompressedSize, props, dictSize, success);
                if (!success) {
                    qWarning() << "lzmaUncompress failed";
                    return false;
                }
                blockStorage.append(dst);
                qDebug() << "uncompressed size = " << dst.size();
            } else if (compressionType == 2 || compressionType == 3) { // LZ4 || LZ4HC
                QByteArray comped = vrcaBlob.mid(cursor, blockInfo.compressedSize);
                qDebug() << "comped.size() ==" << comped.size();
                cursor += blockInfo.compressedSize;
                int lz4Result = LZ4_decompress_safe_partial(comped.data(), dst.data(), blockInfo.compressedSize,
                                                            blockInfo.uncompressedSize, blockInfo.uncompressedSize);
                qDebug() << "lz4Result = " << lz4Result << " vs " << blockInfo.uncompressedSize;
                blockStorage.append(dst);
            } else if (compressionType == 4) { // LZHAM
                // XXX
                cursor += blockInfo.compressedSize;
            }
        } else {
            blockStorage.append(vrcaBlob.mid(cursor, blockInfo.uncompressedSize));
            cursor += blockInfo.compressedSize;
        }
    }

    // std::list<QByteArray> assets;
    for (NodeInfo nodeInfo : nodeInfos) {
        qDebug() << "------ node: "
                 << "offset =" << nodeInfo.offset
                 << "size =" << nodeInfo.uncompressedSize
                 << "flags =" << nodeInfo.flags
                 << "name =" << nodeInfo.name;
        QByteArray asset = blockStorage.mid(nodeInfo.offset, nodeInfo.uncompressedSize);

        if (nodeInfo.name.endsWith(".resource")) {
            continue;
        }

        int assetCursor = 0;
        // assets.push_back(asset);
        uint64_t metadataSize = unpackDWordBigEnd(asset, assetCursor, success);
        if (!success) {
            qWarning() << "can't get metadataSize from node";
            return false;
        }

        uint64_t fileSize = unpackDWordBigEnd(asset, assetCursor, success);
        if (!success) {
            qWarning() << "can't get fileSize from node";
            return false;
        }

        uint64_t format = unpackDWordBigEnd(asset, assetCursor, success);
        if (!success) {
            qWarning() << "can't get format from node";
            return false;
        }

        uint64_t dataOffset = unpackDWordBigEnd(asset, assetCursor, success);
        if (!success) {
            qWarning() << "can't get dataOffset from node";
            return false;
        }

        qDebug() << "metadataSize =" << metadataSize
                 << "fileSize =" << fileSize
                 << "format =" << format
                 << "dataOffset = " << dataOffset;

        bool littleEndian { false };
        if (format >= 9) {
            uint64_t endian = unpackDWordBigEnd(asset, assetCursor, success);
            if (endian == 0) {
                littleEndian = true;
            }
        }
        qDebug() << "little endian = " << littleEndian;


        /// ???  asset.py 111

        TypeMetadataPointer typeMetadata = extractTypeMetaData(asset, assetCursor, format, littleEndian, success);

        // XXX asset.py 118

        bool longObjectIDs { false };
        if (7 <= format && format <= 13) {
            longObjectIDs = unpackDWord(asset, assetCursor, littleEndian, success) > 0;
        }

        qDebug() << "after tree" << assetCursor;

        uint64_t numObjects = unpackDWord(asset, assetCursor, littleEndian, success);
        qDebug() << "numObjects = " << numObjects;
        for (int i = 0; i < (int)numObjects; i++) {
            if (format >= 14) {
                alignCursor(assetCursor); // asset.py 124
            }

            extractObject(asset, assetCursor, format, littleEndian, longObjectIDs, dataOffset, typeMetadata, success);
        }

        // asset.py 132
        if (format >= 11) {
            uint64_t numAdds = unpackDWord(asset, assetCursor, littleEndian, success);
            for (int i = 0; i < numAdds; i++) {
                if (format >= 14) {
                    alignCursor(assetCursor);
                }
                quint64 ID;
                if (longObjectIDs || format >= 14) {
                    ID = unpackQWord(asset, assetCursor, littleEndian, success);
                } else {
                    ID = unpackDWord(asset, assetCursor, littleEndian, success);
                }

                /*int add = */ unpackInt(asset, assetCursor, littleEndian, success);
                // self.adds.append((id, buf.read_int()))
            }
        }

        if (format >= 6) {
            uint64_t numRefs = unpackDWord(asset, assetCursor, littleEndian, success);
            for (int i = 0; i < numRefs; i++) {
                QString assetPath = unpackString0(asset, assetCursor, success);
                QUuid guid = unpackUUID(asset, assetCursor, success);
                int type = unpackInt(asset, assetCursor, littleEndian, success);
                QString filePath = unpackString0(asset, assetCursor, success);

                qDebug() << "REF " << assetPath << guid << type << filePath;
                // self.asset_refs.append(ref)
            }
        }

        QString unkString = unpackString0(asset, assetCursor, success);
    }

    return true;
}

void VRCAUnpackerApp::alignCursor(int& cursor) {
    int oldCursor = cursor;
    int newCursor = ((int32_t)(oldCursor + 3)) & (int32_t)(-4);
    if (newCursor > oldCursor) {
        cursor = newCursor;
    }
}

void VRCAUnpackerApp::extractObject(QByteArray asset, int& assetCursor, int format,
                                    bool littleEndian, bool longObjectIDs, uint64_t dataOffset,
                                    TypeMetadataPointer typeMetadata, bool& success) {
    // object.py 55
    uint64_t ID;
    if (longObjectIDs || format >= 14) {
        ID = unpackQWord(asset, assetCursor, littleEndian, success);
    } else {
        ID = unpackDWord(asset, assetCursor, littleEndian, success);
    }

    dataOffset += unpackDWord(asset, assetCursor, littleEndian, success);
    uint64_t size = unpackDWord(asset, assetCursor, littleEndian, success);

    int32_t typeID;
    int32_t classID;
    if (format < 17) {
        typeID = unpackInt(asset, assetCursor, littleEndian, success);
        classID = unpackInt(asset, assetCursor, littleEndian, success);
    } else {
        typeID = unpackInt(asset, assetCursor, littleEndian, success);
        classID = typeMetadata->classIDs[typeID];
        typeID = classID;
    }

    qDebug() << "QQQQ typeID =" << typeID << "classID =" << classID;

    // object.py 67

    bool isDestroyed { false };
    if (format <= 10) {
        isDestroyed = unpackDWord(asset, assetCursor, littleEndian, success) > 0;
    }

    uint64_t unk0;
    if (format >= 11 && format <= 16) {
        unk0 = unpackDWord(asset, assetCursor, littleEndian, success);
    }
    uint64_t unk1;
    if (format >= 15 && format <= 16) {
        unpackByte(asset, assetCursor, success);
    }
}

TypeMetadataPointer VRCAUnpackerApp::extractTypeMetaData(QByteArray asset, int& assetCursor,
                                                     int format, bool littleEndian, bool& success) {
    TypeMetadataPointer typeMetadata = std::make_shared<TypeMetadata>();

    // type.py 128

    QString generatorVersion = unpackString0(asset, assetCursor, success);
    qDebug() << "generatorVersion =" << generatorVersion;
    typeMetadata->generatorVersion = generatorVersion;

    uint64_t targetPlatform = unpackDWord(asset, assetCursor, littleEndian, success);
    qDebug() << "targetPlatform =" << targetPlatform; // RuntimePlatform: WindowsWebPlayer = 5
    typeMetadata->targetPlatform = targetPlatform;

    TypeTreePointer typeTree;
    if (format >= 13) {
        bool hasTypeTrees = unpackByte(asset, assetCursor, success) > 0;
        uint64_t numTypes = unpackDWord(asset, assetCursor, littleEndian, success);
        qDebug() << "hasTypeTrees =" << hasTypeTrees << "numTypes =" << numTypes;

        for (int i = 0; i < (int)numTypes; i++) {
            int classID = unpackInt(asset, assetCursor, littleEndian, success);
            qDebug() << "classID =" << classID;
            if (format >= 17) {
                unpackByte(asset, assetCursor, success); // unknown byte
                int scriptID = (int)unpackWord(asset, assetCursor, littleEndian, success);
                qDebug() << "scriptID =" << scriptID;
                if (classID == 114) {
                    if (scriptID >= 0) {
                        // make up a fake negative class_id to work like the
                        // old system.  class_id of -1 is taken to mean that
                        // the MonoBehaviour base class was serialized; that
                        // shouldn't happen, but it's easy to account for.
                        classID = -2 - scriptID;
                    } else {
                        classID = -1;
                    }
                }
            }

            typeMetadata->classIDs.push_back(classID);

            QByteArray hash;
            if (classID < 0) {
                hash = unpackBytes(asset, assetCursor, 0x20, success);
            } else {
                hash = unpackBytes(asset, assetCursor, 0x10, success);
            }
            typeMetadata->hashes[classID] = hash;

            if (hasTypeTrees) {
                typeTree = extractTree(asset, assetCursor, format, littleEndian, success);
                typeMetadata->typeTrees[classID] = typeTree;
            }
        }
    } else {
        uint64_t numFields = unpackDWord(asset, assetCursor, littleEndian, success);
        for (int i = 0; i < (int)numFields; i++) {
            int classID = unpackInt(asset, assetCursor, littleEndian, success);
            typeTree = extractTree(asset, assetCursor, format, littleEndian, success);
            typeMetadata->typeTrees[classID] = typeTree;
        }
    }

    return typeMetadata;
}

TypeTreePointer VRCAUnpackerApp::extractTree(QByteArray asset, int& assetCursor, int format, bool littleEndian, bool& success) {
    // type.py 31
    if (format == 10 || format >= 12) {
        return extractNewStyleTree(asset, assetCursor, format, littleEndian, success);
    } else {
        return extractOldStyleTree(asset, assetCursor, format, littleEndian, success);
    }
}

TypeTreePointer VRCAUnpackerApp::extractOldStyleTree(QByteArray asset, int& assetCursor,
                                                     int format, bool littleEndian, bool& success) {
    // QString type = getDATString(asset, assetCursor, success);
    // if (!success) {
    //     return;
    // }
    // QString name = getDATString(asset, assetCursor, success);
    // if (!success) {
    //     return;
    // }
    // uint64_t size = unpackDWord(asset, assetCursor, littleEndian, success);
    // uint64_t index = unpackDWord(asset, assetCursor, littleEndian, success);
    // bool isArray = unpackDWord(asset, assetCursor, littleEndian, success) > 0;
    // uint64_t version = unpackDWord(asset, assetCursor, littleEndian, success);
    // uint64_t flags = unpackDWord(asset, assetCursor, littleEndian, success);
    // uint64_t numFields = unpackDWord(asset, assetCursor, littleEndian, success);

    // for (int i = 0; i < numFields; i ++) {
    //     // XXX type.py 47
    // }

    qDebug() << "old-tree";
    return nullptr;
}

TypeTreePointer VRCAUnpackerApp::extractNewStyleTree(QByteArray asset, int& assetCursor,
                                                     int format, bool littleEndian, bool& success) {
    uint64_t numNodes = unpackDWord(asset, assetCursor, littleEndian, success);
    qDebug() << "numNodes =" << numNodes;
    uint64_t bufferBytes = unpackDWord(asset, assetCursor, littleEndian, success);
    QByteArray nodeData = asset.mid(assetCursor, 24 * numNodes);
    assetCursor += 24 * numNodes;
    QByteArray data = asset.mid(assetCursor, bufferBytes);
    assetCursor += bufferBytes;

    qDebug() << "new-tree: numNodes =" << numNodes << " size=" << bufferBytes;

    int nodeDataCursor { 0 };

    std::list<TypeTreePointer> parents;

    TypeTreePointer top = std::make_shared<TypeTree>();
    parents.push_back(top);
    TypeTreePointer curr = top;

    for (int i = 0; i < (int)numNodes; i++) {
        uint64_t version = unpackWord(nodeData, nodeDataCursor, littleEndian, success);
        uint64_t depth = unpackByte(nodeData, nodeDataCursor, success);

        if (depth == 0) {
            curr = top;
        } else {
            while (parents.size() > depth) {
                parents.pop_back();
            }
            curr = std::make_shared<TypeTree>();
            parents.back()->children.push_back(curr);
        }


        bool isArray = unpackByte(nodeData, nodeDataCursor, success) > 0;

        int typeOffset = unpackInt(nodeData, nodeDataCursor, littleEndian, success);
        QString type = unpackDATString(typeOffset, data, success);

        int nameOffset = unpackInt(nodeData, nodeDataCursor, littleEndian, success);
        QString name = unpackDATString(nameOffset, data, success);

        uint64_t size = unpackDWord(nodeData, nodeDataCursor, littleEndian, success);
        uint64_t index = unpackDWord(nodeData, nodeDataCursor, littleEndian, success);
        uint64_t flags = unpackDWord(nodeData, nodeDataCursor, littleEndian, success);

        // qDebug() << "version =" << version << "depth =" << depth << "isArray =" << isArray
        //          << "type =" << type << "name =" << name << "size =" << size << "index =" << index << "flags =" << flags;

        curr->type = type;
        curr->name = name;
        curr->version = version;
        curr->isArray = isArray;
        curr->size = size;
        curr->index = index;
        curr->flags = flags;
    }

    return top;
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


QString VRCAUnpackerApp::unpackStringN(QByteArray vrcaBlob, int& cursor, int length, bool& success) {
    QByteArray data = unpackBytes(vrcaBlob, cursor, length, success);
    if (success) {
        return QString(data);
    }
    return "";
}


uint64_t VRCAUnpackerApp::unpackByte(QByteArray vrcaBlob, int& cursor, bool& success) {
    uint64_t result { 0 };

    if (cursor + 1 > vrcaBlob.size()) {
        success = false;
        return result;
    }

    unsigned char* v = (unsigned char*)vrcaBlob.data() + cursor;
    result = (uint64_t)v[0];

    success = true;
    cursor += 1;
    return result;
}


uint64_t VRCAUnpackerApp::unpackWordBigEnd(QByteArray vrcaBlob, int& cursor, bool& success) {
    uint64_t result { 0 };

    if (cursor + 2 > vrcaBlob.size()) {
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

uint64_t VRCAUnpackerApp::unpackWordLittleEnd(QByteArray vrcaBlob, int& cursor, bool& success) {
    uint64_t result { 0 };

    if (cursor + 2 > vrcaBlob.size()) {
        success = false;
        return result;
    }

    unsigned char* v = (unsigned char*)vrcaBlob.data() + cursor;
    result =
        ((uint64_t)v[0] << 0) +
        ((uint64_t)v[1] << 8);

    success = true;
    cursor += 2;
    return result;
}

uint64_t VRCAUnpackerApp::unpackWord(QByteArray vrcaBlob, int& cursor, bool littleEndian, bool& success) {
    if (littleEndian) {
        return unpackWordLittleEnd(vrcaBlob, cursor, success);
    } else {
        return unpackWordBigEnd(vrcaBlob, cursor, success);
    }
}

uint64_t VRCAUnpackerApp::unpackDWordBigEnd(QByteArray vrcaBlob, int& cursor, bool& success) {
    uint64_t result { 0 };

    if (cursor + 4 > vrcaBlob.size()) {
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

uint64_t VRCAUnpackerApp::unpackDWordLittleEnd(QByteArray vrcaBlob, int& cursor, bool& success) {
    uint64_t result { 0 };

    if (cursor + 4 > vrcaBlob.size()) {
        success = false;
        return result;
    }

    unsigned char* v = (unsigned char*)vrcaBlob.data() + cursor;
    result =
        ((uint64_t)v[0] << 0) +
        ((uint64_t)v[1] << 8) +
        ((uint64_t)v[2] << 16) +
        ((uint64_t)v[3] << 24);

    success = true;
    cursor += 4;
    return result;
}

uint64_t VRCAUnpackerApp::unpackDWord(QByteArray vrcaBlob, int& cursor, bool littleEndian, bool& success) {
    if (littleEndian) {
        return unpackDWordLittleEnd(vrcaBlob, cursor, success);
    } else {
        return unpackDWordBigEnd(vrcaBlob, cursor, success);
    }
}

int32_t VRCAUnpackerApp::unpackInt(QByteArray vrcaBlob, int& cursor, bool littleEndian, bool& success) {
    if (littleEndian) {
        return (int32_t)(uint32_t)unpackDWordLittleEnd(vrcaBlob, cursor, success);
    } else {
        return (int32_t)(uint32_t)unpackDWordBigEnd(vrcaBlob, cursor, success);
    }
}


uint64_t VRCAUnpackerApp::unpackQWordBigEnd(QByteArray vrcaBlob, int& cursor, bool& success) {
    uint64_t result { 0 };

    if (cursor + 8 > vrcaBlob.size()) {
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

uint64_t VRCAUnpackerApp::unpackQWordLittleEnd(QByteArray vrcaBlob, int& cursor, bool& success) {
    uint64_t result { 0 };

    if (cursor + 8 > vrcaBlob.size()) {
        success = false;
        return result;
    }

    unsigned char* v = (unsigned char*)vrcaBlob.data() + cursor;
    result =
        ((uint64_t)v[0] << 0) +
        ((uint64_t)v[1] << 8) +
        ((uint64_t)v[2] << 16) +
        ((uint64_t)v[3] << 24) +
        ((uint64_t)v[4] << 32) +
        ((uint64_t)v[5] << 40) +
        ((uint64_t)v[6] << 48) +
        ((uint64_t)v[7] << 56);

    success = true;
    cursor += 8;
    return result;
}

uint64_t VRCAUnpackerApp::unpackQWord(QByteArray vrcaBlob, int& cursor, bool littleEndian, bool& success) {
    if (littleEndian) {
        return unpackQWordLittleEnd(vrcaBlob, cursor, success);
    } else {
        return unpackQWordBigEnd(vrcaBlob, cursor, success);
    }
}


QUuid VRCAUnpackerApp::unpackUUID(QByteArray vrcaBlob, int& cursor, bool& success) {
    // XXX is this right?
    QUuid result = QUuid::fromRfc4122(vrcaBlob.mid(cursor, 16));
    cursor += 16;
    return result;
}

QByteArray VRCAUnpackerApp::unpackBytes(QByteArray vrcaBlob, int& cursor, int size, bool& success) {
    QByteArray result;
    if (cursor + size > vrcaBlob.size()) {
        success = false;
        return result;
    }
    result = vrcaBlob.mid(cursor, size);
    cursor += size;
    return result;
}


QByteArray STRINGS_DAT;

void makeSTRING_DAT() {
    if (STRINGS_DAT.size() != 0) {
        return;
    }

    static const char* strings[] = {
        "AABB", "AnimationClip", "AnimationCurve", "AnimationState", "Array", "Base", "BitField", "bitset",
        "bool", "char", "ColorRGBA", "Component", "data", "deque", "double", "dynamic_array",
        "FastPropertyName", "first", "float", "Font", "GameObject", "Generic Mono", "GradientNEW", "GUID",
        "GUIStyle", "int", "list", "long long", "map", "Matrix4x4f", "MdFour", "MonoBehaviour",
        "MonoScript", "m_ByteSize", "m_Curve", "m_EditorClassIdentifier", "m_EditorHideFlags", "m_Enabled",
        "m_ExtensionPtr", "m_GameObject", "m_Index", "m_IsArray", "m_IsStatic", "m_MetaFlag", "m_Name",
        "m_ObjectHideFlags", "m_PrefabInternal", "m_PrefabParentObject", "m_Script", "m_StaticEditorFlags",
        "m_Type", "m_Version", "Object", "pair", "PPtr<Component>", "PPtr<GameObject>", "PPtr<Material>",
        "PPtr<MonoBehaviour>", "PPtr<MonoScript>", "PPtr<Object>", "PPtr<Prefab>", "PPtr<Sprite>",
        "PPtr<TextAsset>", "PPtr<Texture>", "PPtr<Texture2D>", "PPtr<Transform>", "Prefab", "Quaternionf",
        "Rectf", "RectInt", "RectOffset", "second", "set", "short", "size", "SInt16", "SInt32", "SInt64",
        "SInt8", "staticvector", "string", "TextAsset", "TextMesh", "Texture", "Texture2D", "Transform",
        "TypelessData", "UInt16", "UInt32", "UInt64", "UInt8", "unsigned int", "unsigned long long",
        "unsigned short", "vector", "Vector2f", "Vector3f", "Vector4f", "m_ScriptingClassIdentifier",
        "Gradient"
    };

    int count = sizeof(strings) / sizeof(char*);
    for (int i = 0; i < count; i++) {
        STRINGS_DAT.append(QString(strings[i]));
        STRINGS_DAT.append((char)0);
    }
}

QString VRCAUnpackerApp::unpackDATString(int offset, QByteArray data, bool& success) {
    makeSTRING_DAT();

    if (offset < 0) {
        offset &= 0x7fffffff;
        return unpackString0(STRINGS_DAT, offset, success);
    } else if (offset < data.size()) {
        return unpackString0(data, offset, success);
    } else {
        return "";
    }
}
