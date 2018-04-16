//
//  EntityConstraint.h
//  libraries/physcis/src
//
//  Created by Seth Alves 2018-4-15
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  http://bulletphysics.org/Bullet/BulletFull/classbtConstraintInterface.html

#ifndef hifi_EntityConstraint_h
#define hifi_EntityConstraint_h

#include <QUuid>
#include <btBulletDynamicsCommon.h>
#include <dynamics/EntityDynamic.h>

class EntityConstraint : public EntityDynamic
{
public:
    EntityConstraint(EntityDynamicType type, const QUuid& id, EntityItemPointer ownerEntity)  :
        EntityDynamic(type, id, ownerEntity) {}
    virtual ~EntityConstraint() {}

    virtual btTypedConstraint* getConstraint() = 0;

    virtual bool isConstraint() const override { return true; }
    virtual void invalidate() override { _constraint = nullptr; }

protected:
    btTypedConstraint* _constraint { nullptr };
};

#endif // hifi_EntityConstraint_h
