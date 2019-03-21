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

#include <EntityItemID.h>

/**jsdoc
 * The EntityScriptFilter API enables you to export and import entities to and from JSON files.
 *
 * @namespace EntityScriptFilter
 *
 * @hifi-interface
 * @hifi-client-entity
 * @hifi-avatar
 */
class EntityScriptFilterScriptingInterface : public QObject {
    Q_OBJECT
public:
    EntityScriptFilterScriptingInterface();

public:
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

    Q_INVOKABLE void addRegexToScriptURLWhitelist(QString regexPattern);
    Q_INVOKABLE void removeRegexFromScriptURLWhitelist(QString regexPattern);

    /**jsdoc
     * Get the list of white-listed regular-expressions against which entity-script URLs are compared.
     * @function Entities.getEntityScriptURLWhitelistRegexs
     * @returns {string[]} List of regular-expressions which will allow a script to run (as long as they don't also
     * match the blacklist).
     */
    Q_INVOKABLE QStringList getEntityScriptURLWhitelistRegexs() const;

    /**jsdoc
     * Get the list of black-listed regular-expressions against which entity-script URLs are compared.
     * @function Entities.getEntityScriptURLBlacklistRegexs
     * @returns {string[]} List of regular-expressions which will deny a script to run
     */
    Q_INVOKABLE QStringList getEntityScriptURLBlacklistRegexs() const;
};

#endif // hifi_EntityScriptFilterScriptingInterface_h
