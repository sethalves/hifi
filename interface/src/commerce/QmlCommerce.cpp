//
//  Commerce.cpp
//  interface/src/commerce
//
//  Created by Howard Stearns on 8/4/17.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "QmlCommerce.h"
#include "Application.h"
#include "DependencyManager.h"
#include "Ledger.h"
#include "Wallet.h"

HIFI_QML_DEF(QmlCommerce)

QmlCommerce::QmlCommerce(QQuickItem* parent) : OffscreenQmlDialog(parent) {
    auto ledger = DependencyManager::get<Ledger>();
    connect(ledger.data(), &Ledger::buyResult, this, &QmlCommerce::buyResult);
    connect(ledger.data(), &Ledger::balanceResult, this, &QmlCommerce::balanceResult);
    connect(ledger.data(), &Ledger::inventoryResult, this, &QmlCommerce::inventoryResult);
}

void QmlCommerce::buy(const QString& assetId, int cost, const QString& buyerUsername) {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    QStringList keys = wallet->listPublicKeys();
    if (keys.count() == 0) {
        return emit buyResult("Uninitialized Wallet.");
    }
    QString key = keys[0];
    // For now, we receive at the same key that pays for it.
    ledger->buy(key, cost, assetId, key, buyerUsername);
    // FIXME: until we start talking to server, report post-transaction balance and inventory so we can see log for testing.
    balance();
    inventory();
}

void QmlCommerce::balance() {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    ledger->balance(wallet->listPublicKeys());
}
void QmlCommerce::inventory() {
    auto ledger = DependencyManager::get<Ledger>();
    auto wallet = DependencyManager::get<Wallet>();
    ledger->inventory(wallet->listPublicKeys());
 }