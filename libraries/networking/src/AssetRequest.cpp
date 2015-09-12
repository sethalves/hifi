//
//  AssetRequest.cpp
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/24
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AssetRequest.h"

#include <algorithm>

#include <QThread>

#include "AssetClient.h"
#include "NetworkLogging.h"
#include "NodeList.h"

AssetRequest::AssetRequest(QObject* parent, const QString& hash, const QString& extension) :
    QObject(parent),
    _hash(hash),
    _extension(extension)
{
    
}

void AssetRequest::start() {
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(this, "start", Qt::AutoConnection);
        return;
    }

    if (_state != NOT_STARTED) {
        qCWarning(networking) << "AssetRequest already started.";
        return;
    }
    
    _state = WAITING_FOR_INFO;
    
    auto assetClient = DependencyManager::get<AssetClient>();
    assetClient->getAssetInfo(_hash, _extension, [this](AssetServerError serverError, AssetInfo info) {
        _info = info;
        
        if (serverError != AssetServerError::NoError) {
            qCDebug(networking) << "Got error retrieving asset info for" << _hash;
            
            _state = FINISHED;
            emit finished(this);
            
            _error = (serverError == AssetServerError::AssetNotFound) ? NotFound : UnknownError;
            
            return;
        }
        
        _state = WAITING_FOR_DATA;
        _data.resize(info.size);
        
        qCDebug(networking) << "Got size of " << _hash << " : " << info.size << " bytes";
        
        int start = 0, end = _info.size;
        
        auto assetClient = DependencyManager::get<AssetClient>();
        assetClient->getAsset(_hash, _extension, start, end, [this, start, end](AssetServerError serverError,
                                                                                const QByteArray& data) {
            Q_ASSERT(data.size() == (end - start));
            
            if (serverError == AssetServerError::NoError) {
                
                // we need to check the hash of the received data to make sure it matches what we expect
                if (hashData(data).toHex() == _hash) {
                    memcpy(_data.data() + start, data.constData(), data.size());
                    _totalReceived += data.size();
                    emit progress(_totalReceived, _info.size);
                } else {
                    // hash doesn't match - we have an error
                    _error = HashVerificationFailed;
                }
                
            } else {
                switch (serverError) {
                    case AssetServerError::AssetNotFound:
                        _error = NotFound;
                        break;
                    case AssetServerError::InvalidByteRange:
                        _error = InvalidByteRange;
                        break;
                    default:
                        _error = UnknownError;
                        break;
                }
            }
            
            if (_error != NoError) {
                qCDebug(networking) << "Got error retrieving asset" << _hash << "- error code" << _error;
            }
            
            _state = FINISHED;
            emit finished(this);
        });
    });
}
