//
//  EntityScriptFilterScriptingInterface.h
//  interface/src/scripting
//
//  Created by Seth Alves on 2019-3-25.
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityScriptFilterScriptingInterface_h
#define hifi_EntityScriptFilterScriptingInterface_h

#include <QObject>
#include <DependencyManager.h>
#include <EntityItemID.h>

/**jsdoc
 * The EntityScriptFilter API enables you to manage a list of regular-expressions which determine
 * which entity-scripts are loaded and which are forbidden.
 *
 * @namespace EntityScriptFilter
 *
 * @hifi-interface
 */
class EntityScriptFilterScriptingInterface : public QObject, public Dependency {
    Q_OBJECT
public:
    EntityScriptFilterScriptingInterface();

public:

    Q_INVOKABLE QStringList getServerScriptWhitelist() const;
    Q_INVOKABLE QStringList getServerBlockedScripts() const;

    /**jsdoc
     * Add to the list of regular-expressions which control which entity-script URLs will not be allowed to run.
     * @function Entities.addRegexToScriptURLBlacklist
     * @param {string} regexPattern - regular-expression against which entity-script URLs will be matched and
     * forbidden to run.
     */

    Q_INVOKABLE QStringList getServerBlockedScriptEditors(QString blockedScriptURL) const;
    Q_INVOKABLE void addToServerScriptPrefixWhitelist(QString prefixToAdd);
    Q_INVOKABLE void removeFromServerScriptPrefixWhitelist(QString prefixToRemove);
};

#endif // hifi_EntityScriptFilterScriptingInterface_h
