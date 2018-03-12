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

#include <shared/ReadWriteLockable.h>

#include "EntityItemID.h"

class EntityItem;
class EntitySimulation;
using EntityItemPointer = std::shared_ptr<EntityItem>;
using EntityItemWeakPointer = std::weak_ptr<EntityItem>;
class EntitySimulation;
using EntitySimulationPointer = std::shared_ptr<EntitySimulation>;
class SpatiallyNestable;
using SpatiallyNestablePointer = std::shared_ptr<SpatiallyNestable>;
using SpatiallyNestableWeakPointer = std::weak_ptr<SpatiallyNestable>;


enum EntityDynamicType {
    // keep these synchronized with dynamicTypeFromString and dynamicTypeToString
    DYNAMIC_TYPE_NONE = 0,
    DYNAMIC_TYPE_OFFSET = 1000,
    DYNAMIC_TYPE_SPRING = 2000,
    DYNAMIC_TYPE_TRACTOR = 2100,
    DYNAMIC_TYPE_HOLD = 3000,
    DYNAMIC_TYPE_TRAVEL_ORIENTED = 4000,
    DYNAMIC_TYPE_HINGE = 5000,
    DYNAMIC_TYPE_FAR_GRAB = 6000,
    DYNAMIC_TYPE_SLIDER = 7000,
    DYNAMIC_TYPE_BALL_SOCKET = 8000,
    DYNAMIC_TYPE_CONE_TWIST = 9000
};


class EntityDynamicInterface : public ReadWriteLockable {
public:
    EntityDynamicInterface(EntityDynamicType type, const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~EntityDynamicInterface() { }
    const QUuid& getID() const { return _id; }
    EntityDynamicType getType() const { return _type; }

    virtual bool isAction() const { return false; }
    virtual bool isConstraint() const { return false; }
    virtual bool isReadyForAdd() const { return true; }

    bool isActive() { return _active; }
    virtual quint64 getExpires() { return _expires; }

    virtual void removeFromSimulation(EntitySimulationPointer simulation) const = 0;

    virtual EntityItemWeakPointer getOwnerEntity() const { return _ownerEntity; }
    virtual void setOwnerEntity(const EntityItemPointer ownerEntity) { _ownerEntity = ownerEntity; }

    virtual bool updateArguments(QVariantMap arguments);
    virtual QVariantMap getArguments();

    virtual QByteArray serialize() const = 0;
    virtual void deserialize(QByteArray serializedArguments) = 0;

    static EntityDynamicType dynamicTypeFromString(QString dynamicTypeString);
    static QString dynamicTypeToString(EntityDynamicType dynamicType);

    virtual bool lifetimeIsOver();

    virtual bool isMine() { return _isMine; }
    virtual void setIsMine(bool value) { _isMine = value; }

    virtual bool shouldSuppressLocationEdits() { return false; }

    virtual void prepareForPhysicsSimulation() { }

    virtual void remapIDs(QHash<EntityItemID, EntityItemID>& map) = 0;

    // these look in the arguments map for a named argument.  if it's not found or isn't well formed,
    // ok will be set to false (note that it's never set to true -- set it to true before calling these).
    // if required is true, failure to extract an argument will cause a warning to be printed.
    static glm::vec3 extractVec3Argument (QString objectName, QVariantMap arguments,
                                          QString argumentName, bool& ok, bool required = true);
    static glm::quat extractQuatArgument (QString objectName, QVariantMap arguments,
                                          QString argumentName, bool& ok, bool required = true);
    static float extractFloatArgument(QString objectName, QVariantMap arguments,
                                      QString argumentName, bool& ok, bool required = true);
    static int extractIntegerArgument(QString objectName, QVariantMap arguments,
                                      QString argumentName, bool& ok, bool required = true);
    static QString extractStringArgument(QString objectName, QVariantMap arguments,
                                         QString argumentName, bool& ok, bool required = true);
    static bool extractBooleanArgument(QString objectName, QVariantMap arguments,
                                       QString argumentName, bool& ok, bool required = true);

protected:
    QUuid _id;
    EntityDynamicType _type;
    bool _active { false };
    bool _isMine { false }; // did this interface create / edit this dynamic?
    quint64 _expires { 0 }; // in seconds since epoch
    QString _tag;
    EntityItemWeakPointer _ownerEntity;

    SpatiallyNestableWeakPointer _other;
    SpatiallyNestablePointer getOther();
    EntityItemID _otherID;
};


typedef std::shared_ptr<EntityDynamicInterface> EntityDynamicPointer;

QDataStream& operator<<(QDataStream& stream, const EntityDynamicType& entityDynamicType);
QDataStream& operator>>(QDataStream& stream, EntityDynamicType& entityDynamicType);

QString serializedDynamicsToDebugString(QByteArray data);

#endif // hifi_EntityDynamicInterface_h
