//
//  QmlCommerce.h
//  interface/src/commerce
//
//  Guard for safe use of Commerce (Wallet, Ledger) by authorized QML.
//
//  Created by Howard Stearns on 8/4/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#pragma once
#ifndef hifi_QmlCommerce_h
#define hifi_QmlCommerce_h

#include <QJsonObject>

#include <QPixmap>

class QmlCommerce : public QObject {
    Q_OBJECT

public:
    QmlCommerce();

signals:
    void walletStatusResult(uint walletStatus);

    void loginStatusResult(bool isLoggedIn);
    void keyFilePathIfExistsResult(const QString& path);
    void securityImageResult(bool exists);
    void walletAuthenticatedStatusResult(bool isAuthenticated);
    void changePassphraseStatusResult(bool changeSuccess);

    void buyResult(QJsonObject result);
    // Balance and Inventory are NOT properties, because QML can't change them (without risk of failure), and
    // because we can't scalably know of out-of-band changes (e.g., another machine interacting with the block chain).
    void balanceResult(QJsonObject result);
    void inventoryResult(QJsonObject result);
    void historyResult(QJsonObject result);
    void accountResult(QJsonObject result);
    void certificateInfoResult(QJsonObject result);
    void alreadyOwnedResult(QJsonObject result);
    void availableUpdatesResult(QJsonObject result);
    void updateItemResult(QJsonObject result);

    void updateCertificateStatus(const QString& certID, uint certStatus);

    void transferHfcToNodeResult(QJsonObject result);
    void transferHfcToUsernameResult(QJsonObject result);

    void contentSetChanged(const QString& contentSetHref);

    void appInstalled(const QString& appHref);
    void appUninstalled(const QString& appHref);

protected:
    Q_INVOKABLE void getWalletStatus();

    Q_INVOKABLE void getLoginStatus();
    Q_INVOKABLE void getKeyFilePathIfExists();
    Q_INVOKABLE void getSecurityImage();
    Q_INVOKABLE void getWalletAuthenticatedStatus();
    Q_INVOKABLE bool copyKeyFileFrom(const QString& pathname);

    Q_INVOKABLE void chooseSecurityImage(const QString& imageFile);
    Q_INVOKABLE void setPassphrase(const QString& passphrase);
    Q_INVOKABLE void changePassphrase(const QString& oldPassphrase, const QString& newPassphrase);
    Q_INVOKABLE void setSoftReset();
    Q_INVOKABLE void clearWallet();

    Q_INVOKABLE void buy(const QString& assetId, int cost, const bool controlledFailure = false);
    Q_INVOKABLE void balance();
    Q_INVOKABLE void inventory();
    Q_INVOKABLE void history(const int& pageNumber);
    Q_INVOKABLE void generateKeyPair();
    Q_INVOKABLE void account();

    Q_INVOKABLE void certificateInfo(const QString& certificateId);
    Q_INVOKABLE void alreadyOwned(const QString& marketplaceId);

    Q_INVOKABLE void transferHfcToNode(const QString& nodeID, const int& amount, const QString& optionalMessage);
    Q_INVOKABLE void transferHfcToUsername(const QString& username, const int& amount, const QString& optionalMessage);

    Q_INVOKABLE void replaceContentSet(const QString& itemHref, const QString& certificateID);

    Q_INVOKABLE QString getInstalledApps();
    Q_INVOKABLE bool installApp(const QString& appHref);
    Q_INVOKABLE bool uninstallApp(const QString& appHref);
    Q_INVOKABLE bool openApp(const QString& appHref);

    Q_INVOKABLE void getAvailableUpdates(const QString& itemId = "");
    Q_INVOKABLE void updateItem(const QString& certificateId);

private:
    QString _appsPath;
};

#endif // hifi_QmlCommerce_h
