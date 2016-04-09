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
    spheres

    function printDebug(message) {
        if (wantDebug) {
            print(message);
        }
    }

    function greatestDimension(dimensions) {
        return Math.max(Math.max(dimensions.x, dimensions.y), dimensions.z);
    }

    function mass2(dimensions) {
        return dimensions.x * dimensions.y * dimensions.z;
    }

    function findSpheres(position) {
        spheres = Entities.findEntities(position, GRAVITY_RANGE);
    }

    function applyGravity() { 
        if (!simulate) {
            return;
        }
        var deltaTime = timer.elapsed() / 1000.0;
        if (deltaTime == 0.0) {
            return;
        }
        properties = Entities.getEntityProperties(entityID);
        timeSinceLastSearch += CHECK_INTERVAL;
        if (timeSinceLastSearch >= SEARCH_INTERVAL) {
            findSpheres(properties.position);
            timeSinceLastSearch = 0;
        }
        var spheres = Entities.findEntities(properties.position, GRAVITY_RANGE);
        var deltaVelocity = { x: 0, y: 0, z: 0 };
        var otherCount = 0;
        var mass = mass2(properties.dimensions);

        for (var i = 0; i < spheres.length; i++) {
            if (entityID != spheres[i]) {
                otherProperties = Entities.getEntityProperties(spheres[i]);
                if (otherProperties.type == "Sphere") {
                    otherCount++;
                    var radius = Vec3.distance(properties.position, otherProperties.position);
                    var otherMass = mass2(otherProperties.dimensions);
                    if (radius > (greatestDimension(properties.dimensions) + greatestDimension(otherProperties.dimensions)) / 2) {
                        deltaVelocity = Vec3.sum(deltaVelocity, 
                                            Vec3.multiply(deltaTime * GRAVITY_STRENGTH * otherMass / (radius * radius), 
                                            Vec3.normalize(Vec3.subtract(otherProperties.position, properties.position))));
                    }
                }
            }
        }
        Entities.editEntity(entityID, { velocity: Vec3.sum(properties.velocity, deltaVelocity) });
        if (Vec3.length(properties.velocity) < MIN_VELOCITY) {
            print("Gravity simulation stopped.");
            simulate = false;
        } else {
            timer.start();
            timeoutID = Script.setTimeout(applyGravity, CHECK_INTERVAL); 
        }
    }

    this.startNearGrab = function() { 
    }

    this.releaseGrab = function() { 
        printDebug("Gravity simulation started."); 
        properties = Entities.getEntityProperties(entityID);
        findSpheres(properties.position);
        timer.start();
        timeoutID = Script.setTimeout(applyGravity, CHECK_INTERVAL); 
        simulate = true;  
    }

    this.preload = function (givenEntityID) {
        printDebug("load gravity...");
        entityID = givenEntityID;
    };

    this.unload = function () {
        printDebug("Unload gravity...");
        if (timeoutID !== undefined) {
            Script.clearTimeout(timeoutID);
        }
        if (simulate) {
            Entities.editEntity(entityID, { velocity: { x: 0, y: 0, z: 0 }});
        }
    };

});