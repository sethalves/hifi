//
//  VRCAUnpackerApp.h
//  tools/vrca-unpacker/src
//
//  Created by Seth Alves on 2018-1-2
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_VRCAUnpackerApp_h
#define hifi_VRCAUnpackerApp_h

#include <memory>
#include <QApplication>


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

class TypeTree;
using TypeTreePointer = std::shared_ptr<TypeTree>;
class TypeTree {
public:
    TypeTree() { }

    QString type;
    QString name;
    uint64_t version;
    bool isArray;
    uint64_t size;
    uint64_t index;
    uint64_t flags;

    std::vector<TypeTreePointer> children;
};


class TypeMetadata;
using TypeMetadataPointer = std::shared_ptr<TypeMetadata>;
class TypeMetadata {
public:
    TypeMetadata() { }
    std::vector<int> classIDs;
    std::map<int, TypeTreePointer> typeTrees;
    std::map<int, QByteArray> hashes;
    QByteArray asset;
    QString generatorVersion;
    QString targetPlatform;
};


class VRCAUnpackerApp : public QCoreApplication {
    Q_OBJECT
public:
    VRCAUnpackerApp(int argc, char* argv[]);
    ~VRCAUnpackerApp();

    bool unpackVRCAFile(QString filename);
    bool unpackVRCAByteArray(QByteArray vrcaBlob);

private:
    bool _verbose;

    TypeMetadataPointer extractTypeMetaData(QByteArray asset, int& assetCursor, int format, bool littleEndian, bool& success);
    TypeTreePointer extractTree(QByteArray asset, int& assetCursor, int format, bool littleEndian, bool& success);
    TypeTreePointer extractOldStyleTree(QByteArray asset, int& assetCursor, int format, bool littleEndian, bool& success);
    TypeTreePointer extractNewStyleTree(QByteArray asset, int& assetCursor, int format, bool littleEndian, bool& success);
    void extractObject(QByteArray asset, int& assetCursor, int format, bool littleEndian, bool longObjectIDs,
                       uint64_t dataOffset, TypeMetadataPointer typeMeta, bool& success);
    void alignCursor(int& cursor);

    QString unpackString0(QByteArray vrcaBlob, int& cursor, bool& success);
    QString unpackStringN(QByteArray vrcaBlob, int& cursor, int length, bool& success);
    uint64_t unpackByte(QByteArray vrcaBlob, int& cursor, bool& success);
    uint64_t unpackWord(QByteArray vrcaBlob, int& cursor, bool littleEndian, bool& success);
    uint64_t unpackWordBigEnd(QByteArray vrcaBlob, int& cursor, bool& success);
    uint64_t unpackWordLittleEnd(QByteArray vrcaBlob, int& cursor, bool& success);
    uint64_t unpackDWord(QByteArray vrcaBlob, int& cursor, bool littleEndian, bool& success);
    int32_t unpackInt(QByteArray vrcaBlob, int& cursor, bool littleEndian, bool& success);
    uint64_t unpackDWordBigEnd(QByteArray vrcaBlob, int& cursor, bool& success);
    uint64_t unpackDWordLittleEnd(QByteArray vrcaBlob, int& cursor, bool& success);
    uint64_t unpackQWord(QByteArray vrcaBlob, int& cursor, bool littleEndian, bool& success);
    uint64_t unpackQWordBigEnd(QByteArray vrcaBlob, int& cursor, bool& success);
    uint64_t unpackQWordLittleEnd(QByteArray vrcaBlob, int& cursor, bool& success);
    QByteArray unpackBytes(QByteArray vrcaBlob, int& cursor, int size, bool& success);
    QUuid unpackUUID(QByteArray vrcaBlob, int& cursor, bool& success);
    QString unpackDATString(int offset, QByteArray data, bool& success);
};

#endif // hifi_VRCAUnpackerApp_h
