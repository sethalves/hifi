//
//  AddressBarDialog.cpp
//  interface/src/ui
//
//  Created by Bradley Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "AddressBarDialog.h"

#include <QMessageBox>

#include "DependencyManager.h"
#include "AddressManager.h"

HIFI_QML_DEF(AddressBarDialog)

AddressBarDialog::AddressBarDialog(QQuickItem* parent) : OffscreenQmlDialog(parent) {
    auto addressManager = DependencyManager::get<AddressManager>();
    connect(addressManager.data(), &AddressManager::lookupResultIsOffline, this, &AddressBarDialog::displayAddressOfflineMessage);
    connect(addressManager.data(), &AddressManager::lookupResultIsNotFound, this, &AddressBarDialog::displayAddressNotFoundMessage);
    connect(addressManager.data(), &AddressManager::lookupResultsFinished, this, &AddressBarDialog::hide);
    connect(addressManager.data(), &AddressManager::goBackPossible, this, [this] (bool isPossible) {
        if (isPossible != _backEnabled) {
            _backEnabled = isPossible;
            emit backEnabledChanged();
        }
    });
    connect(addressManager.data(), &AddressManager::goForwardPossible, this, [this] (bool isPossible) {
        if (isPossible != _forwardEnabled) {
            _forwardEnabled = isPossible;
            emit forwardEnabledChanged();
        }
    });
    _backEnabled = !(DependencyManager::get<AddressManager>()->getBackStack().isEmpty());
    _forwardEnabled = !(DependencyManager::get<AddressManager>()->getForwardStack().isEmpty());
}

void AddressBarDialog::hide() {
    ((QQuickItem*)parent())->setEnabled(false);
}

void AddressBarDialog::loadAddress(const QString& address) {
    qDebug() << "Called LoadAddress with address " << address;
    if (!address.isEmpty()) {
        DependencyManager::get<AddressManager>()->handleLookupString(address);
    }
}

void AddressBarDialog::loadBack() {
    qDebug() << "Called LoadBack";
    DependencyManager::get<AddressManager>()->goBack();
}

void AddressBarDialog::loadForward() {
    qDebug() << "Called LoadForward";
    DependencyManager::get<AddressManager>()->goForward();
}

void AddressBarDialog::displayAddressOfflineMessage() {
    OffscreenUi::error("That user or place is currently offline");
}

void AddressBarDialog::displayAddressNotFoundMessage() {
    OffscreenUi::error("There is no address information for that user or place");
}

