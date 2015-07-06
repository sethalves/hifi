//
//  ShapeColliderTests.h
//  tests/physics/src
//
//  Created by Andrew Meadows on 02/21/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ShapeColliderTests_h
#define hifi_ShapeColliderTests_h

#include <QtTest/QtTest>
#include <QtGlobal>

// Add additional qtest functionality (the include order is important!)
#include "BulletTestUtils.h"
#include "GlmTestUtils.h"
#include "../QTestExtensions.h"


class ShapeColliderTests : public QObject {
    Q_OBJECT

private slots:
    void initTestCase();
    
    void sphereMissesSphere();
    void sphereTouchesSphere();

    void sphereMissesCapsule();
    void sphereTouchesCapsule();

    void capsuleMissesCapsule();
    void capsuleTouchesCapsule();

    void sphereTouchesAACubeFaces();
    void sphereTouchesAACubeEdges();
    void sphereTouchesAACubeCorners();
    void sphereMissesAACube();

    void capsuleMissesAACube();
    void capsuleTouchesAACube();

    void rayHitsSphere();
    void rayBarelyHitsSphere();
    void rayBarelyMissesSphere();
    void rayHitsCapsule();
    void rayMissesCapsule();
    void rayHitsPlane();
    void rayMissesPlane();
    void rayHitsAACube();
    void rayMissesAACube();

    void measureTimeOfCollisionDispatch();
};

#endif // hifi_ShapeColliderTests_h
