//
//  ObjectDynamicUtils.h
//  libraries/physcis/src
//
//  Created by Seth Alves 2015-6-2
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  http://bulletphysics.org/Bullet/BulletFull/classbtDynamicInterface.html

#ifndef hifi_ObjectDynamic_h
#define hifi_ObjectDynamic_h

#include <QUuid>

#include <btBulletDynamicsCommon.h>

#include <shared/ReadWriteLockable.h>

#include "ObjectMotionState.h"
#include "BulletUtil.h"


btRigidBody* getDynamicRigidBody(EntityDynamicPointer dynamic);
void activateDynamicBody(EntityDynamicPointer dynamic, bool forceActivation = false);
void forceBodyNonStatic(EntityDynamicPointer dynamic);
EntityItemPointer getEntityByID(EntityItemID entityID);
btRigidBody* getRigidBody(EntityDynamicPointer dynamic);
btRigidBody* getOtherRigidBody(EntityDynamicPointer dynamic);
QList<btRigidBody*> getRigidBodies(EntityDynamicPointer dynamic);

#endif // hifi_ObjectDynamic_h
