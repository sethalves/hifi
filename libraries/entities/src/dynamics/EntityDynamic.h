//
//  EntityDynamic.h
//  libraries/entities/src/dynamics/
//
//  Created by Seth Alves 2015-6-19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  http://bulletphysics.org/Bullet/BulletFull/classbtDynamicInterface.html

#ifndef hifi_EntityDynamic_h
#define hifi_EntityDynamic_h

#include <QUuid>
#include "../EntityItem.h"

#include "EntityDynamicInterface.h"


class EntityDynamic : public EntityDynamicInterface, public ReadWriteLockable {
public:
    EntityDynamic(EntityDynamicType type, const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~EntityDynamic();

    quint64 localTimeToServerTime(quint64 timeValue) const;
    quint64 serverTimeToLocalTime(quint64 timeValue) const;
    qint64 getEntityServerClockSkew() const;

    virtual void remapIDs(QHash<EntityItemID, EntityItemID>& map) override {};

    virtual void removeFromSimulation(EntitySimulationPointer simulation) const override;
    virtual EntityItemWeakPointer getOwnerEntity() const override { return _ownerEntity; }
    virtual void setOwnerEntity(const EntityItemPointer ownerEntity) override { _ownerEntity = ownerEntity; }
    virtual bool updateArguments(QVariantMap arguments) override;
    virtual QVariantMap getArguments() override;

    virtual QByteArray serialize() const override;
    virtual void deserialize(QByteArray serializedArguments) override;

private:
    QByteArray _data;

protected:
    bool _active;
    EntityItemWeakPointer _ownerEntity;
    QString _tag;
    quint64 _expires { 0 }; // in seconds since epoch

    EntityItemID _otherID;
    SpatiallyNestableWeakPointer _other;
};

#endif // hifi_EntityDynamic_h
