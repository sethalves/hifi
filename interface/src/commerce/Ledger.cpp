//
//  Ledger.cpp
//  interface/src/commerce
//
//  Created by Howard Stearns on 8/4/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QJsonObject>
#include <QJsonArray>
#include <QTimeZone>
#include <QJsonDocument>
#include "Wallet.h"
#include "Ledger.h"
#include "CommerceLogging.h"
#include <NetworkingConstants.h>
#include <AddressManager.h>

// inventory answers {status: 'success', data: {assets: [{id: "guid", title: "name", preview: "url"}....]}}
// balance answers {status: 'success', data: {balance: integer}}
// buy and receive_at answer {status: 'success'}
// account synthesizes a result {status: 'success', data: {keyStatus: "preexisting"|"conflicting"|"ok"}}


QJsonObject Ledger::apiResponse(const QString& label, QNetworkReply& reply) {
    QByteArray response = reply.readAll();
    QJsonObject data = QJsonDocument::fromJson(response).object();
    qInfo(commerce) << label << "response" << QJsonDocument(data).toJson(QJsonDocument::Compact);
    return data;
}
// Non-200 responses are not json:
QJsonObject Ledger::failResponse(const QString& label, QNetworkReply& reply) {
    QString response = reply.readAll();
    qWarning(commerce) << "FAILED" << label << response;
    QJsonObject result
    {
        { "status", "fail" },
        { "message", response }
    };
    return result;
}
#define ApiHandler(NAME) void Ledger::NAME##Success(QNetworkReply& reply) { emit NAME##Result(apiResponse(#NAME, reply)); }
#define FailHandler(NAME) void Ledger::NAME##Failure(QNetworkReply& reply) { emit NAME##Result(failResponse(#NAME, reply)); }
#define Handler(NAME) ApiHandler(NAME) FailHandler(NAME)
Handler(buy)
Handler(receiveAt)
Handler(balance)
Handler(inventory)
Handler(transferHfcToNode)
Handler(transferHfcToUsername)
Handler(alreadyOwned)

void Ledger::send(const QString& endpoint, const QString& success, const QString& fail, QNetworkAccessManager::Operation method, AccountManagerAuth::Type authType, QJsonObject request) {
    auto accountManager = DependencyManager::get<AccountManager>();
    const QString URL = "/api/v1/commerce/";
    JSONCallbackParameters callbackParams(this, success, this, fail);
    qCInfo(commerce) << "Sending" << endpoint << QJsonDocument(request).toJson(QJsonDocument::Compact);
    accountManager->sendRequest(URL + endpoint,
        authType,
        method,
        callbackParams,
        QJsonDocument(request).toJson());
}

void Ledger::signedSend(const QString& propertyName, const QByteArray& text, const QString& key, const QString& endpoint, const QString& success, const QString& fail, const bool controlled_failure) {
    auto wallet = DependencyManager::get<Wallet>();
    QString signature = wallet->signWithKey(text, key);
    QJsonObject request;
    request[propertyName] = QString(text);
    if (!controlled_failure) {
        request["signature"] = signature;
    } else {
        request["signature"] = QString("controlled failure!");
    }
    send(endpoint, success, fail, QNetworkAccessManager::PutOperation, AccountManagerAuth::Required, request);
}

void Ledger::keysQuery(const QString& endpoint, const QString& success, const QString& fail, QJsonObject& requestParams) {
    auto wallet = DependencyManager::get<Wallet>();
    QStringList cachedPublicKeys = wallet->listPublicKeys();
    if (!cachedPublicKeys.isEmpty()) {
        requestParams["public_keys"] = QJsonArray::fromStringList(cachedPublicKeys);
        send(endpoint, success, fail, QNetworkAccessManager::PostOperation, AccountManagerAuth::Required, requestParams);
    } else {
        qDebug(commerce) << "User attempted to call keysQuery, but cachedPublicKeys was empty!";
    }
}

void Ledger::keysQuery(const QString& endpoint, const QString& success, const QString& fail) {
    QJsonObject requestParams;
    keysQuery(endpoint, success, fail, requestParams);
}

void Ledger::buy(const QString& hfc_key, int cost, const QString& asset_id, const QString& inventory_key, const bool controlled_failure) {
    QJsonObject transaction;
    transaction["hfc_key"] = hfc_key;
    transaction["cost"] = cost;
    transaction["asset_id"] = asset_id;
    transaction["inventory_key"] = inventory_key;
    QJsonDocument transactionDoc{ transaction };
    auto transactionString = transactionDoc.toJson(QJsonDocument::Compact);
    signedSend("transaction", transactionString, hfc_key, "buy", "buySuccess", "buyFailure", controlled_failure);
}

bool Ledger::receiveAt(const QString& hfc_key, const QString& signing_key) {
    auto accountManager = DependencyManager::get<AccountManager>();
    if (!accountManager->isLoggedIn()) {
        qCWarning(commerce) << "Cannot set receiveAt when not logged in.";
        QJsonObject result{ { "status", "fail" }, { "message", "Not logged in" } };
        emit receiveAtResult(result);
        return false; // We know right away that we will fail, so tell the caller.
    }

    signedSend("public_key", hfc_key.toUtf8(), signing_key, "receive_at", "receiveAtSuccess", "receiveAtFailure");
    return true; // Note that there may still be an asynchronous signal of failure that callers might be interested in.
}

void Ledger::balance(const QStringList& keys) {
    keysQuery("balance", "balanceSuccess", "balanceFailure");
}

void Ledger::inventory(const QStringList& keys) {
    keysQuery("inventory", "inventorySuccess", "inventoryFailure");
}

QString hfcString(const QJsonValue& sentValue, const QJsonValue& receivedValue) {
    int sent = sentValue.toInt();
    int received = receivedValue.toInt();
    if (sent <= 0 && received <= 0) {
        return QString("0 HFC");
    }
    QString result;
    if (sent > 0) {
        result += QString("<font color='#B70A37'><b>-%1 HFC</b></font>").arg(sent);
        if (received > 0) {
            result += QString("<br>");
        }
    }
    if (received > 0) {
        result += QString("<font color='#3AA38F'><b>%1 HFC</b></font>").arg(received);
    }
    return result;
}
static const QString USER_PAGE_BASE_URL = NetworkingConstants::METAVERSE_SERVER_URL().toString() + "/users/";
static const QString PLACE_PAGE_BASE_URL = NetworkingConstants::METAVERSE_SERVER_URL().toString() + "/places/";
static const QStringList KNOWN_USERS(QStringList() << "highfidelity" << "marketplace");
QString userLink(const QString& username, const QString& placename) {
    if (username.isEmpty()) {
        if (placename.isEmpty()) {
            return QString("someone");
        } else {
            return QString("someone <a href=\"%1%2\">nearby</a>").arg(PLACE_PAGE_BASE_URL, placename);
        }
    }
    if (KNOWN_USERS.contains(username)) {
        return username;
    }
    return QString("<a href=\"%1%2\">%2</a>").arg(USER_PAGE_BASE_URL, username);
}

QString transactionString(const QJsonObject& valueObject) {
    int sentCerts = valueObject["sent_certs"].toInt();
    int receivedCerts = valueObject["received_certs"].toInt();
    int sent = valueObject["sent_money"].toInt();
    int dateInteger = valueObject["created_at"].toInt();
    QString message = valueObject["message"].toString();
    QDateTime createdAt(QDateTime::fromSecsSinceEpoch(dateInteger, Qt::UTC));
    QString result;

    if (sentCerts <= 0 && receivedCerts <= 0 && !KNOWN_USERS.contains(valueObject["sender_name"].toString())) {
        // this is an hfc transfer.
        if (sent > 0) {
            QString recipient = userLink(valueObject["recipient_name"].toString(), valueObject["place_name"].toString());
            result += QString("Money sent to %1").arg(recipient);
        } else {
            QString sender = userLink(valueObject["sender_name"].toString(), valueObject["place_name"].toString());
            result += QString("Money from %1").arg(sender);
        }
        if (!message.isEmpty()) {
            result += QString("<br>with memo: <i>\"%1\"</i>").arg(message);
        }
    } else {
        result += valueObject["message"].toString();
    }

    // no matter what we append a smaller date to the bottom of this...
    result += QString("<br><font size='-2' color='#1080B8'>%1").arg(createdAt.toLocalTime().toString(Qt::DefaultLocaleShortDate));
    return result;
}

static const QString MARKETPLACE_ITEMS_BASE_URL = NetworkingConstants::METAVERSE_SERVER_URL().toString() + "/marketplace/items/";
void Ledger::historySuccess(QNetworkReply& reply) {
    // here we send a historyResult with some extra stuff in it
    // Namely, the styled text we'd like to show.  The issue is the
    // QML cannot do that easily since it doesn't know what the wallet
    // public key(s) are.  Let's keep it that way
    QByteArray response = reply.readAll();
    QJsonObject data = QJsonDocument::fromJson(response).object();
    qInfo(commerce) << "history" << "response" << QJsonDocument(data).toJson(QJsonDocument::Compact);

    // we will need the array of public keys from the wallet
    auto wallet = DependencyManager::get<Wallet>();
    auto keys = wallet->listPublicKeys();

    // now we need to loop through the transactions and add fancy text...
    auto historyArray = data.find("data").value().toObject().find("history").value().toArray();
    QJsonArray newHistoryArray;

    // TODO: do this with 0 copies if possible
    for (auto it = historyArray.begin(); it != historyArray.end(); it++) {
        // We have 2 text fields to synthesize, the one on the left is a listing
        // of the HFC in/out of your wallet.  The one on the right contains an explaination
        // of the transaction.  That could be just the memo (if it is a regular purchase), or
        // more text (plus the optional memo) if an hfc transfer
        auto valueObject = (*it).toObject();
        valueObject["hfc_text"] = hfcString(valueObject["sent_money"], valueObject["received_money"]);
        valueObject["transaction_text"] = transactionString(valueObject);
        newHistoryArray.push_back(valueObject);
    }
    // now copy the rest of the json -- this is inefficient
    // TODO: try to do this without making copies
    QJsonObject newData;
    newData["status"] = "success";
    QJsonObject newDataData;
    newDataData["history"] = newHistoryArray;
    newData["data"] = newDataData;
    newData["current_page"] = data["current_page"].toInt();
    emit historyResult(newData);
}

void Ledger::historyFailure(QNetworkReply& reply) {
    failResponse("history", reply);
}

void Ledger::history(const QStringList& keys, const int& pageNumber) {
    QJsonObject params;
    params["per_page"] = 100;
    params["page"] = pageNumber;
    keysQuery("history", "historySuccess", "historyFailure", params);
}

void Ledger::accountSuccess(QNetworkReply& reply) {
    // lets set the appropriate stuff in the wallet now
    auto wallet = DependencyManager::get<Wallet>();
    QByteArray response = reply.readAll();
    QJsonObject data = QJsonDocument::fromJson(response).object()["data"].toObject();

    auto salt = QByteArray::fromBase64(data["salt"].toString().toUtf8());
    auto iv = QByteArray::fromBase64(data["iv"].toString().toUtf8());
    auto ckey = QByteArray::fromBase64(data["ckey"].toString().toUtf8());
    QString remotePublicKey = data["public_key"].toString();
    bool isOverride = wallet->wasSoftReset();

    wallet->setSalt(salt);
    wallet->setIv(iv);
    wallet->setCKey(ckey);

    QString keyStatus = "ok";
    QStringList localPublicKeys = wallet->listPublicKeys();
    if (remotePublicKey.isEmpty() || isOverride) {
        if (!localPublicKeys.isEmpty()) {
            QString key = localPublicKeys.first();
            receiveAt(key, key);
        }
    } else {
        if (localPublicKeys.isEmpty()) {
            keyStatus = "preexisting";
        } else if (localPublicKeys.first() != remotePublicKey) {
            keyStatus = "conflicting";
        }
    }

    // none of the hfc account info should be emitted
    QJsonObject json;
    QJsonObject responseData{ { "status", "success"} };
    json["keyStatus"] = keyStatus;
    responseData["data"] = json;
    emit accountResult(responseData);
}

void Ledger::accountFailure(QNetworkReply& reply) {
    failResponse("account", reply);
}
void Ledger::account() {
    send("hfc_account", "accountSuccess", "accountFailure", QNetworkAccessManager::PutOperation, AccountManagerAuth::Required, QJsonObject());
}

// The api/failResponse is called just for the side effect of logging.
void Ledger::updateLocationSuccess(QNetworkReply& reply) { apiResponse("updateLocation", reply); }
void Ledger::updateLocationFailure(QNetworkReply& reply) { failResponse("updateLocation", reply); }
void Ledger::updateLocation(const QString& asset_id, const QString location, const bool controlledFailure) {
    auto wallet = DependencyManager::get<Wallet>();
    auto walletScriptingInterface = DependencyManager::get<WalletScriptingInterface>();
    uint walletStatus = walletScriptingInterface->getWalletStatus();

    if (walletStatus != (uint)wallet->WALLET_STATUS_READY) {
        emit walletScriptingInterface->walletNotSetup();
        qDebug(commerce) << "User attempted to update the location of a certificate, but their wallet wasn't ready. Status:" << walletStatus;
    } else {
        QStringList cachedPublicKeys = wallet->listPublicKeys();
        if (!cachedPublicKeys.isEmpty()) {
            QString key = cachedPublicKeys[0];
            QJsonObject transaction;
            transaction["certificate_id"] = asset_id;
            transaction["place_name"] = location;
            QJsonDocument transactionDoc{ transaction };
            auto transactionString = transactionDoc.toJson(QJsonDocument::Compact);
            signedSend("transaction", transactionString, key, "location", "updateLocationSuccess", "updateLocationFailure", controlledFailure);
        } else {
            qDebug(commerce) << "User attempted to update the location of a certificate, but cachedPublicKeys was empty!";
        }
    }
}

void Ledger::certificateInfoSuccess(QNetworkReply& reply) {
    auto wallet = DependencyManager::get<Wallet>();
    auto accountManager = DependencyManager::get<AccountManager>();

    QByteArray response = reply.readAll();
    QJsonObject replyObject = QJsonDocument::fromJson(response).object();

    QStringList keys = wallet->listPublicKeys();
    if (keys.count() != 0) {
        QJsonObject data = replyObject["data"].toObject();
        if (data["transfer_recipient_key"].toString() == keys[0]) {
            replyObject.insert("isMyCert", true);
        }
    }
    qInfo(commerce) << "certificateInfo" << "response" << QJsonDocument(replyObject).toJson(QJsonDocument::Compact);
    emit certificateInfoResult(replyObject);
}
void Ledger::certificateInfoFailure(QNetworkReply& reply) { failResponse("certificateInfo", reply); }
void Ledger::certificateInfo(const QString& certificateId) {
    QString endpoint = "proof_of_purchase_status/transfer";
    QJsonObject request;
    request["certificate_id"] = certificateId;
    send(endpoint, "certificateInfoSuccess", "certificateInfoFailure", QNetworkAccessManager::PutOperation, AccountManagerAuth::None, request);
}

void Ledger::transferHfcToNode(const QString& hfc_key, const QString& nodeID, const int& amount, const QString& optionalMessage) {
    QJsonObject transaction;
    transaction["public_key"] = hfc_key;
    transaction["node_id"] = nodeID;
    transaction["quantity"] = amount;
    transaction["message"] = optionalMessage;
    transaction["place_name"] = DependencyManager::get<AddressManager>()->getPlaceName();
    QJsonDocument transactionDoc{ transaction };
    auto transactionString = transactionDoc.toJson(QJsonDocument::Compact);
    signedSend("transaction", transactionString, hfc_key, "transfer_hfc_to_node", "transferHfcToNodeSuccess", "transferHfcToNodeFailure");
}

void Ledger::transferHfcToUsername(const QString& hfc_key, const QString& username, const int& amount, const QString& optionalMessage) {
    QJsonObject transaction;
    transaction["public_key"] = hfc_key;
    transaction["username"] = username;
    transaction["quantity"] = amount;
    transaction["message"] = optionalMessage;
    QJsonDocument transactionDoc{ transaction };
    auto transactionString = transactionDoc.toJson(QJsonDocument::Compact);
    signedSend("transaction", transactionString, hfc_key, "transfer_hfc_to_user", "transferHfcToUsernameSuccess", "transferHfcToUsernameFailure");
}

void Ledger::alreadyOwned(const QString& marketplaceId) {
    auto wallet = DependencyManager::get<Wallet>();
    QString endpoint = "already_owned";
    QJsonObject request;
    QStringList cachedPublicKeys = wallet->listPublicKeys();
    if (!cachedPublicKeys.isEmpty()) {
        request["public_keys"] = QJsonArray::fromStringList(wallet->listPublicKeys());
        request["marketplace_item_id"] = marketplaceId;
        send(endpoint, "alreadyOwnedSuccess", "alreadyOwnedFailure", QNetworkAccessManager::PutOperation, AccountManagerAuth::Required, request);
    } else {
        qDebug(commerce) << "User attempted to use the alreadyOwned endpoint, but cachedPublicKeys was empty!";
    }
}
