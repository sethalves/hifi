//
//  AssetScriptingInterface.cpp
//  libraries/script-engine/src
//
//  Created by Stephen Birarda on 2016-03-08.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssetScriptingInterface.h"

#include <QtScript/QScriptEngine>

#include <AssetRequest.h>
#include <AssetUpload.h>
#include <MappingRequest.h>
#include <NetworkLogging.h>
#include <SharedUtil.h>

AssetScriptingInterface::AssetScriptingInterface(QScriptEngine* engine) :
    _engine(engine)
{

}

void AssetScriptingInterface::uploadData(QString data, QScriptValue callback) {
    QByteArray dataByteArray = data.toUtf8();
    auto upload = DependencyManager::get<AssetClient>()->createUpload(dataByteArray);

    QObject::connect(upload, &AssetUpload::finished, this, [this, callback](AssetUpload* upload, const QString& hash) mutable {
        if (callback.isFunction()) {
            QString url = "atp:" + hash;
            QScriptValueList args { url };
            callback.call(_engine->currentContext()->thisObject(), args);
        }
    });
    upload->start();
}

void AssetScriptingInterface::downloadData(QString urlString, QScriptValue callback) {
    const QString ATP_SCHEME { "atp:" };

    if (!urlString.startsWith(ATP_SCHEME)) {
        return;
    }

    // Make request to atp
    auto path = urlString.right(urlString.length() - ATP_SCHEME.length());
    auto parts = path.split(".", QString::SkipEmptyParts);
    auto hash = parts.length() > 0 ? parts[0] : "";

    auto assetClient = DependencyManager::get<AssetClient>();
    auto assetRequest = assetClient->createRequest(hash);

    _pendingRequests << assetRequest;

    connect(assetRequest.data(), &AssetRequest::finished, this, [this, callback](AssetRequestPointer request) mutable {
        Q_ASSERT(request->getState() == AssetRequest::Finished);

        if (request->getError() == AssetRequest::Error::NoError) {
            if (callback.isFunction()) {
                QString data = QString::fromUtf8(request->getData());
                QScriptValueList args { data };
                callback.call(_engine->currentContext()->thisObject(), args);
            }
        }

        _pendingRequests.remove(request);
    });

    assetRequest->start();
}
