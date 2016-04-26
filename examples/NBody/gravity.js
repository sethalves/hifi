//  gravity.js
//
//  Created by Philip Rosedale on March 29, 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  This entity script causes the object to move with gravitational force and be attracted to other spheres nearby.
//  The force is scaled by GRAVITY_STRENGTH, and only entities of type "Sphere" within GRAVITY_RANGE will affect it.
//  The person who has most recently grabbed this object will simulate it.
//

function Timer() {
    Script.include("http://headache.hungry.com/~seth/hifi/baton-client.js");
    var time;
    var count = 0;
    var totalTime = 0;
    this.reset = function() {
        count = 0;
        totalTime = 0;
    }
    this.start = function() {
        time = new Date().getTime();
    }
    this.record = function() {
        var elapsed =  new Date().getTime() - time;
        totalTime += elapsed;
        count++;
        return elapsed;
    }
    this.count = function() {
        return count;
    }
    this.average = function() {
        return (count == 0) ? 0 : totalTime / count;
    }
    this.elapsed = function() {
        return new Date().getTime() - time;
    }
}

(function () {
    var entityID,
        _this = this,
        wantDebug = true,
        CHECK_INTERVAL = 10.00,
        SEARCH_INTERVAL = 1000,
        GRAVITY_RANGE = 10.0,
        GRAVITY_STRENGTH = 1.0,
        MIN_VELOCITY = 0.1,
        timeoutID = null,
        timeSinceLastSearch = 0,
        timer = new Timer(),
        simulate = false,
        properties,
        spheres,
        batonName,
        baton,
        batonTimeout

    this.printDebug = function(message) {
        if (wantDebug) {
            print(message);
        }
    }

    this.greatestDimension = function(dimensions) {
        return Math.max(Math.max(dimensions.x, dimensions.y), dimensions.z);
    }

    this.mass2 = function(dimensions) {
        return dimensions.x * dimensions.y * dimensions.z;
    }

    this.findSpheres = function(position) {
        spheres = Entities.findEntities(position, GRAVITY_RANGE);
    }

    this.maintainOwnership = function() {
        if (Date.now() + 5000 > batonTimeout) {
            print("maintaining ownership");
            baton.reclaim(null, null, function() { simulate = false; });
            batonTimeout = Date.now() + 10000;
        }
    }

    this.applyGravity = function() {
        if (!simulate) {
            return;
        }

        _this.maintainOwnership();

        var deltaTime = timer.elapsed() / 1000.0;
        if (deltaTime == 0.0) {
            return;
        }
        properties = Entities.getEntityProperties(entityID);
        timeSinceLastSearch += CHECK_INTERVAL;
        if (timeSinceLastSearch >= SEARCH_INTERVAL) {
            _this.findSpheres(properties.position);
            timeSinceLastSearch = 0;
        }
        var spheres = Entities.findEntities(properties.position, GRAVITY_RANGE);
        var deltaVelocity = { x: 0, y: 0, z: 0 };
        var otherCount = 0;
        var mass = _this.mass2(properties.dimensions);

        for (var i = 0; i < spheres.length; i++) {
            if (entityID != spheres[i]) {
                otherProperties = Entities.getEntityProperties(spheres[i]);
                if (otherProperties.type == "Sphere") {
                    otherCount++;
                    var radius = Vec3.distance(properties.position, otherProperties.position);
                    var otherMass = _this.mass2(otherProperties.dimensions);
                    var r = (_this.greatestDimension(properties.dimensions) +
                             _this.greatestDimension(otherProperties.dimensions)) / 2;
                    if (radius > r) {
                        var n0 = Vec3.normalize(Vec3.subtract(otherProperties.position, properties.position));
                        var n1 = Vec3.multiply(deltaTime * GRAVITY_STRENGTH * otherMass / (radius * radius), n0);
                        deltaVelocity = Vec3.sum(deltaVelocity, n1);
                    }
                }
            }
        }
        Entities.editEntity(entityID, { velocity: Vec3.sum(properties.velocity, deltaVelocity) });
        if (Vec3.length(properties.velocity) < MIN_VELOCITY) {
            print("Gravity simulation stopped.");
            simulate = false;
            baton.release();
        } else {
            timer.start();
            timeoutID = Script.setTimeout(_this.applyGravity, CHECK_INTERVAL);
        }
    }

    this.startNearGrab = function() {
    }

    this.releaseGrab = function() {
        this.printDebug("Gravity simulation started.");
        baton.claim(
            function() {
                properties = Entities.getEntityProperties(entityID);
                batonTimeout = Date.now() + 10000;
                _this.findSpheres(properties.position);
                timer.start();
                timeoutID = Script.setTimeout(_this.applyGravity, CHECK_INTERVAL);
                simulate = true;
            });
    }

    this.preload = function (givenEntityID) {
        this.printDebug("load gravity...");
        entityID = givenEntityID;
        batonName = 'io.highfidelity.philip.gravity:' + entityID;
        baton = acBaton({
            batonName: batonName,
            timeScale: 10000
        });
    };

    this.unload = function () {
        this.printDebug("Unload gravity...");
        if (timeoutID !== undefined) {
            Script.clearTimeout(timeoutID);
        }
        if (simulate) {
            Entities.editEntity(entityID, { velocity: { x: 0, y: 0, z: 0 }});
        }
        baton.release();
    };
});
