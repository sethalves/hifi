//
//  WebTablet.js
//
//  Created by Anthony J. Thibault on 8/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var RAD_TO_DEG = 180 / Math.PI;
var X_AXIS = {x: 1, y: 0, z: 0};
var Y_AXIS = {x: 0, y: 1, z: 0};
var DEFAULT_DPI = 32;
var DEFAULT_WIDTH = 0.5;

var TABLET_URL = "https://s3.amazonaws.com/hifi-public/tony/tablet.fbx";

// returns object with two fields:
//    * position - position in front of the user
//    * rotation - rotation of entity so it faces the user.
function calcSpawnInfo() {
    var front;
    var pitchBackRotation = Quat.angleAxis(20.0, X_AXIS);
    if (HMD.active) {
        front = Quat.getFront(HMD.orientation);
        var yawOnlyRotation = Quat.angleAxis(Math.atan2(front.x, front.z) * RAD_TO_DEG, Y_AXIS);
        return {
            position: Vec3.sum(Vec3.sum(HMD.position, Vec3.multiply(0.6, front)), Vec3.multiply(-0.5, Y_AXIS)),
            rotation: Quat.multiply(yawOnlyRotation, pitchBackRotation)
        };
    } else {
        front = Quat.getFront(MyAvatar.orientation);
        return {
            position: Vec3.sum(Vec3.sum(MyAvatar.position, Vec3.multiply(0.6, front)), {x: 0, y: 0.6, z: 0}),
            rotation: Quat.multiply(MyAvatar.orientation, pitchBackRotation)
        };
    }
}

// ctor
WebTablet = function (url, width, dpi, location) {

    var ASPECT = 4.0 / 3.0;
    var WIDTH = width || DEFAULT_WIDTH;
    var HEIGHT = WIDTH * ASPECT;
    var DEPTH = 0.025;
    var DPI = dpi || DEFAULT_DPI;
    var _this = this;

    var tabletProperties = {
        name: "WebTablet Tablet",
        type: "Model",
        modelURL: TABLET_URL,
        userData: JSON.stringify({
            "grabbableKey": {"grabbable": true}
        }),
        dimensions: {x: WIDTH, y: HEIGHT, z: DEPTH},
        parentID: MyAvatar.sessionUUID,
        parentJointIndex: -2
    }

    if (location) {
        tabletProperties.localPosition = location.localPosition;
        tabletProperties.localRotation = location.localRotation;
    } else {
        var spawnInfo = calcSpawnInfo();
        tabletProperties.position = spawnInfo.position;
        tabletProperties.rotation = spawnInfo.rotation;
    }

    this.tabletEntityID = Entities.addEntity(tabletProperties);

    var WEB_ENTITY_REDUCTION_FACTOR = {x: 0.78, y: 0.85};
    var WEB_ENTITY_Z_OFFSET = -0.01;

    this.webEntityID = Entities.addEntity({
        name: "WebTablet Web",
        type: "Web",
        sourceUrl: url,
        dimensions: {x: WIDTH * WEB_ENTITY_REDUCTION_FACTOR.x,
                     y: HEIGHT * WEB_ENTITY_REDUCTION_FACTOR.y,
                     z: 0.1},
        localPosition: { x: 0, y: 0, z: WEB_ENTITY_Z_OFFSET },
        localRotation: Quat.angleAxis(180, Y_AXIS),
        shapeType: "box",
        dpi: DPI,
        parentID: this.tabletEntityID,
        parentJointIndex: -1
    });

    this.state = "idle";

    this.getRoot = function() {
        return Entities.getWebViewRoot(_this.webEntityID);
    }

    this.getLocation = function() {
        return Entities.getEntityProperties(_this.tabletEntityID, ["localPosition", "localRotation"]);
    };
};

WebTablet.prototype.destroy = function () {
    Entities.deleteEntity(this.webEntityID);
    Entities.deleteEntity(this.tabletEntityID);
};

