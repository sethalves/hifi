"use strict";

//  scabbard.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


/* global Script, Entities, Settings, MyAvatar, RIGHT_HAND, LEFT_HAND, Vec3, makeRunningValues,
   makeDispatcherModuleParameters, enableDispatcherModule, disableDispatcherModule, Messages */

Script.include("/~/system/libraries/controllerDispatcherUtils.js");
Script.include("/~/system/libraries/controllers.js");

(function() {
    var SCABBARD_SETTINGS = "io.highfidelity.scabbard";
    var DOWN = { x: 0, y: -1, z: 0 };

    // these should match what's in equipEntity.js
    var TRIGGER_SMOOTH_RATIO = 0.1; //  Time averaging of trigger - 0.0 disables smoothing
    var TRIGGER_OFF_VALUE = 0.1;
    var TRIGGER_ON_VALUE = TRIGGER_OFF_VALUE + 0.05; //  Squeezed just enough to activate search or near grab
    var BUMPER_ON_VALUE = 0.5;

    function Scabbard(hand) {
        this.hand = hand;
        this.entityInScabbardProps = null; // properties for entity that would be rezzed
        this.targetEntityID = null; // id of dropped entity that might go into scabbard
        this.triggerValue = 0;
        this.rawSecondaryValue = 0;

        this.parameters = makeDispatcherModuleParameters(
            310,
            this.hand === RIGHT_HAND ? ["rightHand", "rightHandEquip"] : ["leftHand", "leftHandEquip"],
            [],
            100);

        try {
            this.entityInScabbardProps = JSON.parse(Settings.getValue(SCABBARD_SETTINGS + "." + this.hand));
        } catch (err) {
            // don't want to spam the logs
        }

        this.detectScabbardGesture = function(controllerData) {
            var neckJointIndex = MyAvatar.getJointIndex("Neck");
            var avatarFrameNeckPos = MyAvatar.getAbsoluteJointTranslationInObjectFrame(neckJointIndex);
            var eyeJointIndex = MyAvatar.getJointIndex("LeftEye");
            var avatarFrameEyePos = MyAvatar.getAbsoluteJointTranslationInObjectFrame(eyeJointIndex);

            var controllerLocation = controllerData.controllerLocations[this.hand];
            var avatarFrameControllerPos = MyAvatar.worldToJointPoint(controllerLocation.position, -1);
            var avatarFrameControllerRot = MyAvatar.worldToJointRotation(controllerLocation.orientation, -1);

            if (avatarFrameControllerPos.y > avatarFrameNeckPos.y && // above the neck and
                avatarFrameControllerPos.z > avatarFrameEyePos.z) { // behind the eyes
                var localHandUpAxis = this.hand === RIGHT_HAND ? { x: 1, y: 0, z: 0 } : { x: -1, y: 0, z: 0 };
                var localHandUp = Vec3.multiplyQbyV(avatarFrameControllerRot, localHandUpAxis);
                if (Vec3.dot(localHandUp, DOWN) > 0.0) {
                    return true; // hand is upside-down vs avatar
                }
            }
            return false;
        };

        this.saveEntityInScabbard = function (controllerData) {
            var props = Entities.getEntityProperties(this.targetEntityID);

            if (!props || !props.position) {
                return;
            }

            print("saving " + this.targetEntityID);

            var jointName = (hand === RIGHT_HAND) ? "RightHand" : "LeftHand";
            var jointIndex = MyAvatar.getJointIndex(jointName);

            props.localPosition = MyAvatar.worldToJointPoint(props.position, jointIndex);
            props.localRotation = MyAvatar.worldToJointPoint(props.rotation, jointIndex);

            delete props.clientOnly;
            delete props.created;
            delete props.lastEdited;
            delete props.lastEditedBy;
            delete props.owningAvatarID;
            delete props.queryAACube;
            delete props.age;
            delete props.ageAsText;
            delete props.naturalDimensions;
            delete props.naturalPosition;
            delete props.acceleration;
            delete props.scriptTimestamp;
            delete props.boundingBox;
            delete props.position;
            delete props.rotation;
            delete props.velocity;
            delete props.angularVelocity;
            delete props.dimensions;
            delete props.renderInfo;
            delete props.parentID;

            this.entityInScabbardProps = props;
            Settings.setValue(SCABBARD_SETTINGS + "." + this.hand, JSON.stringify(props));
            Entities.deleteEntity(this.targetEntityID);
            this.targetEntityID = null;
        };

        this.handleRelease = function(releasedEntityID) {
            this.targetEntityID = releasedEntityID;
            print("got release: " + this.targetEntityID);
        };

        this.triggerSmoothedSqueezed = function() {
            return this.triggerValue > TRIGGER_ON_VALUE;
        };

        this.secondarySmoothedSqueezed = function() {
            return this.rawSecondaryValue > BUMPER_ON_VALUE;
        };

        this.updateSmoothedTrigger = function(controllerData) {
            var triggerValue = controllerData.triggerValues[this.hand];
            // smooth out trigger value
            this.triggerValue = (this.triggerValue * TRIGGER_SMOOTH_RATIO) +
                (triggerValue * (1.0 - TRIGGER_SMOOTH_RATIO));
        };

        this.updateInputs = function (controllerData) {
            this.rawSecondaryValue = controllerData.secondaryValues[this.hand];
            this.updateSmoothedTrigger(controllerData);
        };

        this.isReady = function (controllerData, deltaTime) {
            this.updateInputs(controllerData);
            if (this.detectScabbardGesture(controllerData)) {
                if (this.targetEntityID) {
                    this.saveEntityInScabbard(controllerData);
                } else if ((this.triggerSmoothedSqueezed() || this.secondarySmoothedSqueezed()) &&
                           this.entityInScabbardProps) {

                    var jointName = (hand === RIGHT_HAND) ? "RightHand" : "LeftHand";
                    var jointIndex = MyAvatar.getJointIndex(jointName);
                    var position = MyAvatar.jointToWorldPoint(this.entityInScabbardProps.localPosition, jointIndex);
                    var rotation = MyAvatar.jointToWorldRotation(this.entityInScabbardProps.localRotation, jointIndex);

                    delete this.entityInScabbardProps.localPosition;
                    delete this.entityInScabbardProps.localRotation;
                    this.entityInScabbardProps.position = position;
                    this.entityInScabbardProps.rotation = rotation;

                    var clientOnly = !(Entities.canRez() || Entities.canRezTmp());
                    var entityIDFromScabbard = Entities.addEntity(this.entityInScabbardProps, clientOnly);
                    print("rezzed " + entityIDFromScabbard + " " + JSON.stringify(this.entityInScabbardProps.position));

                    controllerData.nearbyEntityPropertiesByID[entityIDFromScabbard] = this.entityInScabbardProps;
                    this.entityInScabbardProps = null;
                }
            }

            return makeRunningValues(false, [], []);
        };

        this.run = function (controllerData, deltaTime) {
        };

        this.cleanup = function () {
        };
    }

    var handleMessage = function(channel, message, sender) {
        var data;
        if (sender !== MyAvatar.sessionUUID) {
            return;
        }
        if (channel !== "Hifi-Object-Manipulation") {
            return;
        }
        try {
            data = JSON.parse(message);
        } catch (err) {
            return;
        }
        if (data.action != "release") {
            return;
        }
        var releasedEntityID = data.grabbedEntity;
        var scabbardModule = data.joint == "RightHand" ? leftScabbard : rightScabbard;
        scabbardModule.handleRelease(releasedEntityID);
    };

    Messages.subscribe("Hifi-Object-Manipulation");
    Messages.messageReceived.connect(handleMessage);

    var leftScabbard = new Scabbard(LEFT_HAND);
    var rightScabbard = new Scabbard(RIGHT_HAND);

    enableDispatcherModule("LeftScabbard", leftScabbard);
    enableDispatcherModule("RightScabbard", rightScabbard);

    function cleanup() {
        leftScabbard.cleanup();
        rightScabbard.cleanup();
        disableDispatcherModule("LeftScabbard");
        disableDispatcherModule("RightScabbard");
    }
    Script.scriptEnding.connect(cleanup);
}());
