//
//  EntityDynamicInterface.h
//  libraries/entities/src
//
//  Created by Seth Alves on 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_EntityDynamicInterface_h
#define hifi_EntityDynamicInterface_h

#include <memory>
#include <QUuid>
#include <glm/glm.hpp>

#include "EntityDynamicType.h"

class EntityItem;
class EntityItemID;
class EntitySimulation;
using EntityItemPointer = std::shared_ptr<EntityItem>;
using EntityItemWeakPointer = std::weak_ptr<EntityItem>;
class EntitySimulation;
using EntitySimulationPointer = std::shared_ptr<EntitySimulation>;

class EntityDynamicInterface {
public:
    EntityDynamicInterface(EntityDynamicType type, const QUuid& id) : _id(id), _type(type) { }
    virtual ~EntityDynamicInterface() { }
    EntityDynamicType getType() const { return _type; }

    virtual void remapIDs(QHash<EntityItemID, EntityItemID>& map) = 0;

    virtual bool isAction() const { return false; }
    virtual bool isConstraint() const { return false; }
    virtual bool isReadyForAdd() const { return true; }

    bool isActive() { return _active; }

    virtual void removeFromSimulation(EntitySimulationPointer simulation) const = 0;
    virtual EntityItemWeakPointer getOwnerEntity() const = 0;
    virtual void setOwnerEntity(const EntityItemPointer ownerEntity) = 0;
    virtual bool updateArguments(QVariantMap arguments) = 0;
    virtual QVariantMap getArguments() = 0;

    virtual QByteArray serialize() const = 0;
    virtual void deserialize(QByteArray serializedArguments) = 0;

    static EntityDynamicType dynamicTypeFromString(QString dynamicTypeString);
    static QString dynamicTypeToString(EntityDynamicType dynamicType);

    virtual bool lifetimeIsOver() { return false; }
    virtual quint64 getExpires() { return 0; }

    virtual bool isMine() { return _isMine; }
    virtual void setIsMine(bool value) { _isMine = value; }

    virtual bool shouldSuppressLocationEdits() { return false; }

    virtual void prepareForPhysicsSimulation() { }

protected:
    EntityDynamicType _type;
    bool _active { false };
    bool _isMine { false }; // did this interface create / edit this dynamic?
};


typedef std::shared_ptr<EntityDynamicInterface> EntityDynamicPointer;

QDataStream& operator<<(QDataStream& stream, const EntityDynamicType& entityDynamicType);
QDataStream& operator>>(QDataStream& stream, EntityDynamicType& entityDynamicType);

QString serializedDynamicsToDebugString(QByteArray data);

#endif // hifi_EntityDynamicInterface_h
