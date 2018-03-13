//
//  ObjectActionOffset.h
//  libraries/physics/src
//
//  Created by Seth Alves 2015-6-17
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ObjectActionOffset_h
#define hifi_ObjectActionOffset_h

#include <QUuid>

#include <EntityItem.h>
#include <EntityActionOffset.h>

#include "ObjectAction.h"

class ObjectActionOffset : public ObjectAction, public EntityActionOffset {
public:
    ObjectActionOffset(const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~ObjectActionOffset();

    virtual QByteArray serialize() const override {
        return EntityActionOffset::serializeED();
    }
    virtual void deserialize(QByteArray serializedArguments) override {
        return EntityActionOffset::deserializeED(serializedArguments);
    }

    virtual void updateActionWorker(float deltaTimeStep) override;
};

#endif // hifi_ObjectActionOffset_h
