//
//  FBXDumpApp.h
//  tools/fbx-dump/src
//
//  Created by Seth Alves on 2017-12-24
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_FBXDumpApp_h
#define hifi_FBXDumpApp_h

#include <QApplication>

#include <gpu/Texture.h>
#include <FBX.h>


class FBXDumpApp : public QCoreApplication {
    Q_OBJECT
public:
    FBXDumpApp(int argc, char* argv[]);
    ~FBXDumpApp();
    bool FBXReadFile(QString inputFileName);
    void FBXDump();
    void FBXDumpNode(FBXNode& node, int depth);

private:
    bool _verbose;

    FBXNode _rootNode;
    FBXGeometry* _geometry;
    QHash<QByteArray, QByteArray> _textureContent;
};

#endif // hifi_FBXDumpApp_h
