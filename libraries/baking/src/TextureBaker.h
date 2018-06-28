//
//  TextureBaker.h
//  tools/oven/src
//
//  Created by Stephen Birarda on 4/5/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_TextureBaker_h
#define hifi_TextureBaker_h

#include <QtCore/QObject>
#include <QtCore/QUrl>
#include <QtCore/QRunnable>
#include <QDir>
#include <QImageReader>

#include <image/Image.h>

#include "Baker.h"

extern const QString BAKED_TEXTURE_KTX_EXT;
extern const QString BAKED_META_TEXTURE_SUFFIX;

class TextureBaker : public Baker {
    Q_OBJECT

public:
    TextureBaker(const QUrl& textureURL, image::TextureUsage::Type textureType,
                 const QDir& outputDirectory, const QString& metaTexturePathPrefix = "",
                 const QString& baseFilename = QString(), const QByteArray& textureContent = QByteArray());

    const QByteArray& getOriginalTexture() const { return _originalTexture; }

    QUrl getTextureURL() const { return _textureURL; }

    QString getMetaTextureFileName() const { return _metaTextureFileName; }

    virtual void setWasAborted(bool wasAborted) override;

    static void setCompressionEnabled(bool enabled) { _compressionEnabled = enabled; }

public slots:
    virtual void bake() override;
    virtual void abort() override; 

signals:
    void originalTextureLoaded();

private slots:
    void processTexture();

private:
    void loadTexture();
    void handleTextureNetworkReply();

    QUrl _textureURL;
    QByteArray _originalTexture;
    image::TextureUsage::Type _textureType;

    QString _baseFilename;
    QDir _outputDirectory;
    QString _metaTextureFileName;
    QString _metaTexturePathPrefix;

    std::atomic<bool> _abortProcessing { false };

    static bool _compressionEnabled;
};

#endif // hifi_TextureBaker_h
