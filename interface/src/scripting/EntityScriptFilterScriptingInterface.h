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
    void loadSettings();
    void saveSettings();

    /**jsdoc
     * Get the URLs of all the entity-scripts which have been allowed to run
     * @function Entities.getEntityScriptURLsInWhitelist
     * @returns {string[]} The URLs of entity-scripts which have been allowed to run.
     */
    Q_INVOKABLE QStringList getEntityScriptURLsInWhitelist() const;

    /**jsdoc
     * Get the URLs of all the entity-scripts which have not been allowed to run
     * @function Entities.getEntityScriptURLsInBlacklist
     * @returns {string[]} The URLs of entity-scripts which have not been allowed to run.
     */
    Q_INVOKABLE QStringList getEntityScriptURLsInBlacklist() const;

    /**jsdoc
     * Get the URLs of all the entity-scripts which have not yet been allowed to run
     * @function Entities.getEntityScriptURLsInPurgatory
     * @returns {string[]} The URLs of entity-scripts which have not yet been allowed to run.
     */
    Q_INVOKABLE QStringList getEntityScriptURLsInPurgatory();

    /**jsdoc
     * Add to the list of regular-expressions which control which entity-script URLs will be allowed to run.
     * @function Entities.addRegexToScriptURLWhitelist
     * @param {string} regexPattern - regular-expression against which entity-script URLs will be matched and permitted.
     */
    Q_INVOKABLE void addRegexToScriptURLWhitelist(QString regexPattern);

    /**jsdoc
     * Remove from the list of regular-expressions which control which entity-script URLs will be allowed to run.
     * @function Entities.removeRegexFromScriptURLWhitelist
     * @param {string} regexPattern - regular-expression against which entity-script URLs will no longer be matched.
     */
    Q_INVOKABLE void removeRegexFromScriptURLWhitelist(QString regexPattern);

    /**jsdoc
     * Get the list of white-listed regular-expressions against which entity-script URLs are compared.
     * @function Entities.getEntityScriptURLWhitelistRegexs
     * @returns {string[]} List of regular-expressions which will allow a script to run (as long as they don't also
     * match the blacklist).
     */
    Q_INVOKABLE QStringList getEntityScriptURLWhitelistRegexs() const;

    /**jsdoc
     * Add to the list of regular-expressions which control which entity-script URLs will not be allowed to run.
     * @function Entities.addRegexToScriptURLBlacklist
     * @param {string} regexPattern - regular-expression against which entity-script URLs will be matched and
     * forbidden to run.
     */
    Q_INVOKABLE void addRegexToScriptURLBlacklist(QString regexPattern);

    /**jsdoc
     * Remove from the list of regular-expressions which control which entity-script URLs will not be allowed to run.
     * @function Entities.removeRegexFromScriptURLBlacklist
     * @param {string} regexPattern - regular-expression against which entity-script URLs will no longer be matched.
     */
    Q_INVOKABLE void removeRegexFromScriptURLBlacklist(QString regexPattern);

    /**jsdoc
     * Get the list of black-listed regular-expressions against which entity-script URLs are compared.
     * @function Entities.getEntityScriptURLBlacklistRegexs
     * @returns {string[]} List of regular-expressions which will deny a script to run
     */
    Q_INVOKABLE QStringList getEntityScriptURLBlacklistRegexs() const;


    Q_INVOKABLE QStringList getServerScriptWhitelist() const;
    Q_INVOKABLE QStringList getServerBlockedScripts() const;
    Q_INVOKABLE QStringList getServerBlockedScriptEditors(QString blockedScriptURL) const;
    Q_INVOKABLE void addToServerScriptPrefixWhitelist(QString prefixToAdd);
    Q_INVOKABLE void removeFromServerScriptPrefixWhitelist(QString prefixToRemove);
};

#endif // hifi_EntityScriptFilterScriptingInterface_h
