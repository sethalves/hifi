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
QStringList DefaultWhitelist { ".*" };
QStringList DefaultBlacklist;

EntityScriptFilterScriptingInterface::EntityScriptFilterScriptingInterface() {
}

QStringList EntityScriptFilterScriptingInterface::getServerScriptWhitelist() const {
    QSharedPointer<EntityTreeRenderer> entityTree = qApp->getEntities();
    if (entityTree) {
        EntityTreePointer tree = entityTree->getTree();
        return tree ? tree->getEntityScriptSourceWhitelist() : QStringList();
    } else {
        return QStringList();
    }
}

QStringList EntityScriptFilterScriptingInterface::getServerBlockedScripts() const {
    QSharedPointer<EntityTreeRenderer> entityTree = qApp->getEntities();
    if (entityTree) {
        EntityTreePointer tree = entityTree->getTree();
        return tree ? tree->getBlockedScripts() : QStringList();
    } else {
        return QStringList();
    }
}

QStringList EntityScriptFilterScriptingInterface::getServerBlockedScriptEditors(QString blockedScriptURL) const {
    QSharedPointer<EntityTreeRenderer> entityTree = qApp->getEntities();
    if (entityTree) {
        EntityTreePointer tree = entityTree->getTree();
        return tree ? tree->getServerBlockedScriptEditors(blockedScriptURL) : QStringList();
    } else {
        return QStringList();
    }
}

void EntityScriptFilterScriptingInterface::addToServerScriptPrefixWhitelist(QString prefixToAdd) {
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->getDomainHandler().addToServerScriptPrefixWhitelist(prefixToAdd);
}

void EntityScriptFilterScriptingInterface::removeFromServerScriptPrefixWhitelist(QString prefixToRemove) {
    auto nodeList = DependencyManager::get<NodeList>();
    nodeList->getDomainHandler().removeFromServerScriptPrefixWhitelist(prefixToRemove);
}
