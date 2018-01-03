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

#include <QApplication>


class VRCAUnpackerApp : public QCoreApplication {
    Q_OBJECT
public:
    VRCAUnpackerApp(int argc, char* argv[]);
    ~VRCAUnpackerApp();

    bool unpackVRCAFile(QString filename);
    bool unpackVRCAByteArray(QByteArray vrcaBlob);

private:
    bool _verbose;

    QString unpackString0(QByteArray vrcaBlob, int& cursor, bool& success);
    uint64_t unpackWord(QByteArray vrcaBlob, int& cursor, bool& success);
    uint64_t unpackDWord(QByteArray vrcaBlob, int& cursor, bool& success);
    uint64_t unpackQWord(QByteArray vrcaBlob, int& cursor, bool& success);
    QUuid unpackUUID(QByteArray vrcaBlob, int& cursor, bool& success);
};

#endif // hifi_VRCAUnpackerApp_h
