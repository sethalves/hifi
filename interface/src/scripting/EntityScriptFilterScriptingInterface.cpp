//
//  EntityScriptFilterScriptingInterface.cpp
//  interface/src/scripting
//
//  Created by Seth Alves on 2019-3-25.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "EntityScriptFilterScriptingInterface.h"
#include <SettingHandle.h>
#include "../Application.h"

QString EntityScriptBlockerSettings = "io.highfidelity.entity-script-blocker";

EntityScriptFilterScriptingInterface::EntityScriptFilterScriptingInterface() {
}

void EntityScriptFilterScriptingInterface::loadSettings() {
    QSharedPointer<EntityTreeRenderer> entityTree = qApp->getEntities();
    if (entityTree) {
        Setting::Handle<QStringList> whitelistHandle { EntityScriptBlockerSettings + ".whitelist", QStringList() };
        Setting::Handle<QStringList> blacklistHandle { EntityScriptBlockerSettings + ".blacklist", QStringList() };
        entityTree->clearRegexsFromScriptURLWhitelist();
        entityTree->clearRegexsFromScriptURLBlacklist();

        for (auto& whitelistRegex : whitelistHandle.get()) {
            entityTree->addRegexToScriptURLWhitelist(whitelistRegex);
        }
        for (auto& blacklistRegex : blacklistHandle.get()) {
            entityTree->addRegexToScriptURLBlacklist(blacklistRegex);
        }
    }
}

void EntityScriptFilterScriptingInterface::saveSettings() {
    QSharedPointer<EntityTreeRenderer> entityTree = qApp->getEntities();
    if (entityTree) {
        Setting::Handle<QStringList> whitelistHandle { EntityScriptBlockerSettings + ".whitelist", QStringList() };
        Setting::Handle<QStringList> blacklistHandle { EntityScriptBlockerSettings + ".blacklist", QStringList() };

        whitelistHandle.set(getEntityScriptURLWhitelistRegexs());
        blacklistHandle.set(getEntityScriptURLBlacklistRegexs());
    }
}


QStringList EntityScriptFilterScriptingInterface::getEntityScriptURLsInWhitelist() const {
    QSharedPointer<EntityTreeRenderer> entityTree = qApp->getEntities();
    if (entityTree) {
        return entityTree->getEntityScriptURLsInWhitelist();
    } else {
        return QStringList();
    }
}

QStringList EntityScriptFilterScriptingInterface::getEntityScriptURLsInBlacklist() const {
    QSharedPointer<EntityTreeRenderer> entityTree = qApp->getEntities();
    if (entityTree) {
        return entityTree->getEntityScriptURLsInBlacklist();
    } else {
        return QStringList();
    }
}

QStringList EntityScriptFilterScriptingInterface::getEntityScriptURLsInPurgatory() {
    QSharedPointer<EntityTreeRenderer> entityTree = qApp->getEntities();
    if (entityTree) {
        return entityTree->getEntityScriptURLsInPurgatory();
    } else {
        return QStringList();
    }
}

void EntityScriptFilterScriptingInterface::addRegexToScriptURLWhitelist(QString regexPattern) {
    QSharedPointer<EntityTreeRenderer> entityTree = qApp->getEntities();
    if (entityTree) {
        entityTree->addRegexToScriptURLWhitelist(regexPattern);
    }
}

void EntityScriptFilterScriptingInterface::removeRegexFromScriptURLWhitelist(QString regexPattern) {
    QSharedPointer<EntityTreeRenderer> entityTree = qApp->getEntities();
    if (entityTree) {
        entityTree->removeRegexFromScriptURLWhitelist(regexPattern);
    }
}

QStringList EntityScriptFilterScriptingInterface::getEntityScriptURLWhitelistRegexs() const {
    QSharedPointer<EntityTreeRenderer> entityTree = qApp->getEntities();
    if (entityTree) {
        return entityTree->getEntityScriptURLWhitelistRegexs();
    } else {
        return QStringList();
    }
}

void EntityScriptFilterScriptingInterface::addRegexToScriptURLBlacklist(QString regexPattern) {
    QSharedPointer<EntityTreeRenderer> entityTree = qApp->getEntities();
    if (entityTree) {
        entityTree->addRegexToScriptURLBlacklist(regexPattern);
    }
}

void EntityScriptFilterScriptingInterface::removeRegexFromScriptURLBlacklist(QString regexPattern) {
    QSharedPointer<EntityTreeRenderer> entityTree = qApp->getEntities();
    if (entityTree) {
        entityTree->removeRegexFromScriptURLBlacklist(regexPattern);
    }
}

QStringList EntityScriptFilterScriptingInterface::getEntityScriptURLBlacklistRegexs() const {
    QSharedPointer<EntityTreeRenderer> entityTree = qApp->getEntities();
    if (entityTree) {
        return entityTree->getEntityScriptURLBlacklistRegexs();
    } else {
        return QStringList();
    }
}
