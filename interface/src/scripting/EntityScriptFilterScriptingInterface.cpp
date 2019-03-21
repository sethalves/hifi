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

#include "../Application.h"

#include "EntityScriptFilterScriptingInterface.h"

EntityScriptFilterScriptingInterface::EntityScriptFilterScriptingInterface() {
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

QStringList EntityScriptFilterScriptingInterface::getEntityScriptURLBlacklistRegexs() const {
    QSharedPointer<EntityTreeRenderer> entityTree = qApp->getEntities();
    if (entityTree) {
        return entityTree->getEntityScriptURLBlacklistRegexs();
    } else {
        return QStringList();
    }
}
