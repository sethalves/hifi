//
//  WebTablet.js
//
//  Created by Anthony J. Thibault on 8/8/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/* global Quat, MyAvatar, WebTablet, HMD, Vec3, Entities, Overlays */

var RAD_TO_DEG = 180 / Math.PI;
var X_AXIS = {x: 1, y: 0, z: 0};
var Y_AXIS = {x: 0, y: 1, z: 0};
var DEFAULT_DPI = 30;
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
WebTablet = function (url, width, dpi, location, clientOnly) {

    var ASPECT = 4.0 / 3.0;
    var WIDTH = width || DEFAULT_WIDTH;
    var TABLET_HEIGHT_SCALE = 640 / 680;  //  Screen size of tablet entity isn't quite the desired aspect.
    var HEIGHT = WIDTH * ASPECT * TABLET_HEIGHT_SCALE;
    var DEPTH = 0.025;
    var DPI = dpi || DEFAULT_DPI;
    var _this = this;
    var SENSOR_TO_ROOM_MATRIX = -2;

    var tabletProperties = {
        name: "WebTablet Tablet",
        type: "Model",
        modelURL: TABLET_URL,
        userData: JSON.stringify({
            "grabbableKey": {"grabbable": true}
        }),
        dimensions: {x: WIDTH, y: HEIGHT, z: DEPTH},
        parentID: MyAvatar.sessionUUID,
        parentJointIndex: SENSOR_TO_ROOM_MATRIX
    };

    if (location) {
        tabletProperties.localPosition = location.localPosition;
        tabletProperties.localRotation = location.localRotation;
    } else {
        var spawnInfo = calcSpawnInfo();
        tabletProperties.position = spawnInfo.position;
        tabletProperties.rotation = spawnInfo.rotation;
    }

    this.tabletEntityID = Entities.addEntity(tabletProperties, clientOnly);

    var WEB_ENTITY_Z_OFFSET = -0.01;
    if (this.webOverlayID) {
        Overlays.deleteOverlay(this.webOverlayID);
    }

    this.webOverlayID = Overlays.addOverlay("web3d", {
        name: "WebTablet Web",
        url: url,
        localPosition: { x: 0, y: 0, z: WEB_ENTITY_Z_OFFSET },
        localRotation: Quat.angleAxis(180, Y_AXIS),
        resolution: { x: 480, y: 640 },
        dpi: DPI,
        color: { red: 255, green: 255, blue: 255 },
        alpha: 1.0,
        parentID: this.tabletEntityID,
        parentJointIndex: -1
    });

    this.state = "idle";

    this.getRoot = function() {
        // return Entities.getWebViewRoot(_this.webEntityID);
        return Overlays.getWebViewRoot(_this.webEntityID);
    };

    this.getLocation = function() {
        return Entities.getEntityProperties(_this.tabletEntityID, ["localPosition", "localRotation"]);
    };
};

WebTablet.prototype.setURL = function (url) {
    Overlays.editOverlay(this.webOverlayID, { url: url });
};

WebTablet.prototype.setScriptURL = function (scriptURL) {
    Overlays.editOverlay(this.webOverlayID, { scriptURL: scriptURL });
};

WebTablet.prototype.getOverlayObject = function () {
    return Overlays.getOverlayObject(this.webOverlayID);
};

WebTablet.prototype.destroy = function () {
    Overlays.deleteOverlay(this.webOverlayID);
    Entities.deleteEntity(this.tabletEntityID);
};

WebTablet.prototype.pickle = function () {
    return JSON.stringify({ webOverlayID: this.webOverlayID, tabletEntityID: this.tabletEntityID });
};

WebTablet.unpickle = function (string) {
    if (!string) {
        return;
    }
    var tablet = JSON.parse(string);
    tablet.__proto__ = WebTablet.prototype;
    return tablet;
};
