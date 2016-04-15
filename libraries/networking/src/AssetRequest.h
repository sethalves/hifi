//
//  AssetRequest.h
//  libraries/networking/src
//
//  Created by Ryan Huffman on 2015/07/24
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssetRequest_h
#define hifi_AssetRequest_h

#include <memory>

#include <QByteArray>
#include <QObject>
#include <QString>

#include "AssetClient.h"

#include "AssetUtils.h"

class AssetRequest : public QObject {
   Q_OBJECT
public:
    enum State {
        NotStarted = 0,
        WaitingForInfo,
        WaitingForData,
        Finished
    };

    enum Error {
        NoError,
        NotFound,
        InvalidByteRange,
        InvalidHash,
        HashVerificationFailed,
        NetworkError,
        UnknownError
    };

    static AssetRequestPointer factory(const QString& hash);

private:
    AssetRequest(const QString& hash);
public:
    virtual ~AssetRequest() override;

    Q_INVOKABLE void start();

    const QByteArray& getData() const { return _data; }
    const State& getState() const { return _state; }
    const Error& getError() const { return _error; }
    QUrl getUrl() const { return ::getATPUrl(_hash); }

signals:
    void finished(AssetRequestPointer thisRequest);
    void progress(qint64 totalReceived, qint64 total);

private:
    State _state = NotStarted;
    Error _error = NoError;
    AssetInfo _info;
    uint64_t _totalReceived { 0 };
    QString _hash;
    QByteArray _data;
    int _numPendingRequests { 0 };
    MessageID _assetRequestID { AssetClient::INVALID_MESSAGE_ID };
    MessageID _assetInfoRequestID { AssetClient::INVALID_MESSAGE_ID };

    AssetRequestWeakPointer _selfWeakPointer;
    void setSelfPointer(AssetRequestPointer selfPointer) { _selfWeakPointer = selfPointer; }
};

#endif
