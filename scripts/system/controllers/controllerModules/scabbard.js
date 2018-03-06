"use strict";

//  scabbard.js
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

/* global Script, Entities, MyAvatar, RIGHT_HAND, LEFT_HAND, Settings, Vec3, Messages, makeRunningValues,
   enableDispatcherModule, disableDispatcherModule, makeDispatcherModuleParameters */

Script.include("/~/system/libraries/controllerDispatcherUtils.js");

(function() {
    var SCABBARD_SETTINGS = "io.highfidelity.scabbard";
    var DOWN = { x: 0, y: -1, z: 0 };

    var TRIGGER_OFF_VALUE = 0.1;
    var TRIGGER_ON_VALUE = TRIGGER_OFF_VALUE + 0.05; //  Squeezed just enough to activate search or near grab
    var BUMPER_ON_VALUE = 0.5;

    function cleanProperties(props) {
        delete props.id;
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
        delete props.parentJointIndex;
        delete props.localPosition;
        delete props.localRotation;
        delete props.lifetime;
        delete props.actionData;
        delete props.localVelocity;
        delete props.localAngularVelocity;
        return props;
    }

    function Scabbard(hand) {
        this.hand = hand;
        this.entityInScabbardProps = null;
        this.possibleStoreInScabbardID = null;
        this.parameters = makeDispatcherModuleParameters(
            310,
            this.hand === RIGHT_HAND ? ["rightHand", "rightHandScabbard"] : ["leftHand", "leftHandScabbard"],
            [],
            100);

        try {
            this.entityInScabbardProps = JSON.parse(Settings.getValue(SCABBARD_SETTINGS + "." + this.hand));
            cleanProperties(this.entityInScabbardProps);
        }catch (err) {
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

        this.saveEntityInScabbard = function (controllerData, targetEntityID) {
            var props = Entities.getEntityProperties(targetEntityID);
            if (!props || !props.localPosition) {
                return;
            }
            cleanProperties(props);

            this.entityInScabbardProps = props;
            Settings.setValue(SCABBARD_SETTINGS + "." + this.hand, JSON.stringify(props));
            Entities.deleteEntity(targetEntityID);
        };

        this.possiblyStoreInScabbard = function (possibleStoreInScabbardID) {
            this.possibleStoreInScabbardID = possibleStoreInScabbardID;
        };

        this.isReady = function (controllerData, deltaTime) {
            var triggerSqueezed = controllerData.triggerValues[this.hand] > TRIGGER_ON_VALUE;
            var secondarySqueezed = controllerData.secondaryValues[this.hand] > BUMPER_ON_VALUE;
            var gestureDetected = this.detectScabbardGesture(controllerData);

            if (this.possibleStoreInScabbardID) {
                if (gestureDetected) {
                    this.saveEntityInScabbard(controllerData, this.possibleStoreInScabbardID);
                }
                this.possibleStoreInScabbardID = null;
            } else if ((triggerSqueezed || secondarySqueezed) && this.entityInScabbardProps && gestureDetected) {

                var clientOnly = !(Entities.canRez() || Entities.canRezTmp());
                this.entityInScabbardProps.position = controllerData.controllerLocations[this.hand].position;
                var entityID = Entities.addEntity(this.entityInScabbardProps, clientOnly);
                controllerData.nearbyEntityPropertiesByID[entityID] = this.entityInScabbardProps;
                this.entityInScabbardProps = null;
            }

            return makeRunningValues(false, [], []);
        };

        this.run = function (controllerData, deltaTime) {
            return makeRunningValues(true, ["ok"], []);
        };

        this.cleanup = function () {
        };
    }


    var leftScabbard = new Scabbard(LEFT_HAND);
    var rightScabbard = new Scabbard(RIGHT_HAND);

    enableDispatcherModule("LeftScabbard", leftScabbard);
    enableDispatcherModule("RightScabbard", rightScabbard);

    var handleMessage = function(channel, message, sender) {
        var data;
        if (sender === MyAvatar.sessionUUID) {
            if (channel === 'Hifi-Object-Manipulation') {
                try {
                    data = JSON.parse(message);
                    if (data.action == "release") {
                        var scabbardModule = (data.joint === "LeftHand") ? leftScabbard : rightScabbard;
                        var releasedID = data.grabbedEntity;
                        scabbardModule.possiblyStoreInScabbard(releasedID);
                    }
                } catch (e) {
                    print("WARNING: scabbard.js -- error parsing Hifi-Object-Manipulation message: " + message);
                }
            }
        }
    };

    Messages.subscribe('Hifi-Object-Manipulation');
    Messages.messageReceived.connect(handleMessage);

    function cleanup() {
        leftScabbard.cleanup();
        rightScabbard.cleanup();
        disableDispatcherModule("LeftScabbard");
        disableDispatcherModule("RightScabbard");
    }
    Script.scriptEnding.connect(cleanup);
}());
