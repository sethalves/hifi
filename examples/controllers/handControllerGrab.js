//  handControllerGrab.js
//  examples
//
//  Created by Eric Levin on  9/2/15
//  Additions by James B. Pollack @imgntn on 9/24/2015
//  Additions By Seth Alves on 10/20/2015
//  Some code borrowed from Howard's shakeHands.js
//  Copyright 2015 High Fidelity, Inc.
//
//  Grabs physically moveable entities with hydra-like controllers; it works for either near or far objects.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt, pointInExtents, vec3equal, setEntityCustomData, getEntityCustomData */

Script.include("../libraries/utils.js");


//
// add lines where the hand ray picking is happening
//
var WANT_DEBUG = false;

//
// these tune time-averaging and "on" value for analog trigger
//

var TRIGGER_SMOOTH_RATIO = 0.1; // 0.0 disables smoothing of trigger value
var TRIGGER_ON_VALUE = 0.4;
var TRIGGER_OFF_VALUE = 0.15;

var BUMPER_ON_VALUE = 0.5;

//
// distant manipulation
//

var DISTANCE_HOLDING_RADIUS_FACTOR = 5; // multiplied by distance between hand and object
var DISTANCE_HOLDING_ACTION_TIMEFRAME = 0.1; // how quickly objects move to their new position
var DISTANCE_HOLDING_ROTATION_EXAGGERATION_FACTOR = 2.0; // object rotates this much more than hand did
var NO_INTERSECT_COLOR = { red: 10, green: 10, blue: 255}; // line color when pick misses
var INTERSECT_COLOR = { red: 250, green: 10, blue: 10}; // line color when pick hits
var LINE_ENTITY_DIMENSIONS = { x: 1000, y: 1000,z: 1000};
var LINE_LENGTH = 500;
var PICK_MAX_DISTANCE = 500; // max length of pick-ray

//
// near grabbing
//

var GRAB_RADIUS = 0.03; // if the ray misses but an object is this close, it will still be selected
var NEAR_GRABBING_ACTION_TIMEFRAME = 0.05; // how quickly objects move to their new position
var NEAR_GRABBING_VELOCITY_SMOOTH_RATIO = 1.0; // adjust time-averaging of held object's velocity.  1.0 to disable.
var NEAR_PICK_MAX_DISTANCE = 0.3; // max length of pick-ray for close grabbing to be selected
var RELEASE_VELOCITY_MULTIPLIER = 1.5; // affects throwing things
var PICK_BACKOFF_DISTANCE = 0.2; // helps when hand is intersecting the grabble object
var NEAR_GRABBING_KINEMATIC = true; // force objects to be kinematic when near-grabbed

//
// equip
//

var EQUIP_SPRING_SHUTOFF_DISTANCE = 0.05;
var EQUIP_SPRING_TIMEFRAME = 0.4; // how quickly objects move to their new position

//
// other constants
//

var RIGHT_HAND = 1;
var LEFT_HAND = 0;

var ZERO_VEC = {
    x: 0,
    y: 0,
    z: 0
};
var NULL_ACTION_ID = "{00000000-0000-0000-000000000000}";
var MSEC_PER_SEC = 1000.0;

// these control how long an abandoned pointer line or action will hang around
var LIFETIME = 10;
var ACTION_TTL = 15; // seconds
var ACTION_TTL_REFRESH = 5;
var PICKS_PER_SECOND_PER_HAND = 5;
var MSECS_PER_SEC = 1000.0;
var GRABBABLE_PROPERTIES = ["position",
                            "rotation",
                            "gravity",
                            "ignoreForCollisions",
                            "collisionsWillMove",
                            "locked",
                            "name"];


var GRABBABLE_DATA_KEY = "grabbableKey"; // shared with grab.js
var GRAB_USER_DATA_KEY = "grabKey"; // shared with grab.js

var DEFAULT_GRABBABLE_DATA = {
    grabbable: true,
    invertSolidWhileHeld: false
};

var disabledHand ='none';


// states for the state machine
var STATE_OFF = 0;
var STATE_SEARCHING = 1;
var STATE_DISTANCE_HOLDING = 2;
var STATE_CONTINUE_DISTANCE_HOLDING = 3;
var STATE_NEAR_GRABBING = 4;
var STATE_CONTINUE_NEAR_GRABBING = 5;
var STATE_NEAR_TRIGGER = 6;
var STATE_CONTINUE_NEAR_TRIGGER = 7;
var STATE_FAR_TRIGGER = 8;
var STATE_CONTINUE_FAR_TRIGGER = 9;
var STATE_RELEASE = 10;
var STATE_EQUIP_SEARCHING = 11;
var STATE_EQUIP = 12
var STATE_CONTINUE_EQUIP_BD = 13; // equip while bumper is still held down
var STATE_CONTINUE_EQUIP = 14;
var STATE_WAITING_FOR_BUMPER_RELEASE = 15;
var STATE_EQUIP_SPRING = 16;


function stateToName(state) {
    switch (state) {
    case STATE_OFF:
        return "off";
    case STATE_SEARCHING:
        return "searching";
    case STATE_DISTANCE_HOLDING:
        return "distance_holding";
    case STATE_CONTINUE_DISTANCE_HOLDING:
        return "continue_distance_holding";
    case STATE_NEAR_GRABBING:
        return "near_grabbing";
    case STATE_CONTINUE_NEAR_GRABBING:
        return "continue_near_grabbing";
    case STATE_NEAR_TRIGGER:
        return "near_trigger";
    case STATE_CONTINUE_NEAR_TRIGGER:
        return "continue_near_trigger";
    case STATE_FAR_TRIGGER:
        return "far_trigger";
    case STATE_CONTINUE_FAR_TRIGGER:
        return "continue_far_trigger";
    case STATE_RELEASE:
        return "release";
    case STATE_EQUIP_SEARCHING:
        return "equip_searching";
    case STATE_EQUIP:
        return "equip";
    case STATE_CONTINUE_EQUIP_BD:
        return "continue_equip_bd";
    case STATE_CONTINUE_EQUIP:
        return "continue_equip";
    case STATE_WAITING_FOR_BUMPER_RELEASE:
        return "waiting_for_bumper_release";
    case STATE_EQUIP_SPRING:
        return "state_equip_spring";
    }

    return "unknown";
}

function getTag() {
    return "grab-" + MyAvatar.sessionUUID;
}

function entityIsGrabbedByOther(entityID) {
    // by convention, a distance grab sets the tag of its action to be grab-*owner-session-id*.
    var actionIDs = Entities.getActionIDs(entityID);
    for (var actionIndex = 0; actionIndex < actionIDs.length; actionIndex++) {
        var actionID = actionIDs[actionIndex];
        var actionArguments = Entities.getActionArguments(entityID, actionID);
        var tag = actionArguments["tag"];
        if (tag == getTag()) {
            // we see a grab-*uuid* shaped tag, but it's our tag, so that's okay.
            continue;
        }
        if (tag.slice(0, 5) == "grab-") {
            // we see a grab-*uuid* shaped tag and it's not ours, so someone else is grabbing it.
            return true;
        }
    }
    return false;
}

function findJointIndex(avatar, jointName) { // return joint index for that name.
    // FIXME: Currently by exact name, expected standard avatars. Should look for best match among
    // avatar.jointNames(), or use parent structure topoloogy.
    return avatar.getJointIndex(jointName);
}

// For transforming between world space and our avatar's model space.
var myHipsJointIndex = findJointIndex(MyAvatar, 'Hips');
// N.B.: Our C++ angleAxis takes radians, while our javascript angleAxis takes degrees!
var avatarToModelRotation = Quat.angleAxis(180, {x: 0, y: 1, z: 0});
// Flip 180 gives same result without inverse, but being explicit to track the math.
var modelToAvatarRotation = Quat.inverse(avatarToModelRotation);

var hipJointTrans = MyAvatar.getJointTranslation(myHipsJointIndex);

// Just math.
function modelToWorld(modelPoint) {
    var avatarPoint = Vec3.subtract(Vec3.multiplyQbyV(modelToAvatarRotation, modelPoint), hipJointTrans);
    return Vec3.sum(Vec3.multiplyQbyV(MyAvatar.orientation, avatarPoint), MyAvatar.position);
}

function worldToModel(worldPoint) {
    var avatarPoint = Vec3.multiplyQbyV(Quat.inverse(MyAvatar.orientation), Vec3.subtract(worldPoint, MyAvatar.position));
    var avPlusJoint = Vec3.sum(avatarPoint, hipJointTrans);
    return Vec3.multiplyQbyV(avatarToModelRotation, avPlusJoint);
}


var dragEntityID = {}
var dragOffsetPosition = {}
var dragOffsetRotation = {}
var HAND_TO_PALM_OFFSET = {x: 0.0, y: 0.12, z: 0.08}; // see Avatar.cpp
var HAND_DRAG_EPSILON = 0.01;

dragHand = function(hand, animationProperties) {
    var animVarName = (hand == RIGHT_HAND ? 'rightHandPosition' : 'leftHandPosition');
    var grabbedProperties = Entities.getEntityProperties(dragEntityID[hand], ["position", "rotation"]);
    var itemWorldPosition = grabbedProperties.position;

    var oldHandModelPosition = animationProperties[animVarName];

    var palmWorldRotation = (hand == RIGHT_HAND) ? MyAvatar.getRightPalmRotation() : MyAvatar.getLeftPalmRotation();
    var rotation = Quat.multiply(palmWorldRotation, dragOffsetRotation[hand]);

    // see AvatarActionHold::getTarget
    var relativePosition = dragOffsetPosition[hand]; // in hand-space

    var offset = Vec3.multiplyQbyV(rotation, relativePosition);

    var newPalmWorldPosition = Vec3.subtract(itemWorldPosition, offset);

    var handToPalmWorldOffset = Vec3.multiplyQbyV(palmWorldRotation, HAND_TO_PALM_OFFSET);
    var newHandWorldPosition = Vec3.subtract(newPalmWorldPosition, handToPalmWorldOffset);

    var newHandModelPosition = worldToModel(newHandWorldPosition);

    var result = {};

    if (Vec3.distance(newHandModelPosition, oldHandModelPosition) > HAND_DRAG_EPSILON) {
        result[animVarName] = newHandModelPosition;
    } else {
        result[animVarName] = oldHandModelPosition;
    }

    return result;
};

dragHandLeft = function(animationProperties) {
    return dragHand(LEFT_HAND, animationProperties);
};

dragHandRight = function(animationProperties) {
    return dragHand(RIGHT_HAND, animationProperties);
};


function MyController(hand) {
    this.hand = hand;
    if (this.hand === RIGHT_HAND) {
        this.getHandPosition = MyAvatar.getRightPalmPosition;
        this.getHandRotation = MyAvatar.getRightPalmRotation;
        this.animVarName = 'rightHandPosition';
    } else {
        this.getHandPosition = MyAvatar.getLeftPalmPosition;
        this.getHandRotation = MyAvatar.getLeftPalmRotation;
        this.animVarName = 'leftHandPosition';
    }

    var SPATIAL_CONTROLLERS_PER_PALM = 2;
    var TIP_CONTROLLER_OFFSET = 1;
    this.palm = SPATIAL_CONTROLLERS_PER_PALM * hand;
    this.tip = SPATIAL_CONTROLLERS_PER_PALM * hand + TIP_CONTROLLER_OFFSET;

    this.actionID = null; // action this script created...
    this.grabbedEntity = null; // on this entity.
    this.state = STATE_OFF;
    this.pointer = null; // entity-id of line object
    this.triggerValue = 0; // rolling average of trigger value
    this.rawTriggerValue = 0;
    this.rawBumperValue = 0;

    this.offsetPosition = { x: 0.0, y: 0.0, z: 0.0 };
    this.offsetRotation = { x: 0.0, y: 0.0, z: 0.0, w: 1.0 };

    var _this = this;

    this.update = function() {

        this.updateSmoothedTrigger();

        switch (this.state) {
            case STATE_OFF:
                this.off();
                this.touchTest();
                break;
            case STATE_SEARCHING:
                this.search();
                break;
            case STATE_EQUIP_SEARCHING:
                this.search();
                break;
            case STATE_DISTANCE_HOLDING:
                this.distanceHolding();
                break;
            case STATE_CONTINUE_DISTANCE_HOLDING:
                this.continueDistanceHolding();
                break;
            case STATE_NEAR_GRABBING:
            case STATE_EQUIP:
                this.nearGrabbing();
                break;
            case STATE_WAITING_FOR_BUMPER_RELEASE:
                this.waitingForBumperRelease();
                break;
            case STATE_EQUIP_SPRING:
                this.pullTowardEquipPosition()
                break;
            case STATE_CONTINUE_NEAR_GRABBING:
            case STATE_CONTINUE_EQUIP_BD:
            case STATE_CONTINUE_EQUIP:
                this.continueNearGrabbing();
                break;
            case STATE_NEAR_TRIGGER:
                this.nearTrigger();
                break;
            case STATE_CONTINUE_NEAR_TRIGGER:
                this.continueNearTrigger();
                break;
            case STATE_FAR_TRIGGER:
                this.farTrigger();
                break;
            case STATE_CONTINUE_FAR_TRIGGER:
                this.continueFarTrigger();
                break;
            case STATE_RELEASE:
                this.release();
                break;
        }
    };

    this.setState = function(newState) {
        if (WANT_DEBUG) {
            print("STATE: " + stateToName(this.state) + " --> " + stateToName(newState) + ", hand: " + this.hand);
        }
        this.state = newState;
    }

    this.debugLine = function(closePoint, farPoint, color){
        Entities.addEntity({
            type: "Line",
            name: "Grab Debug Entity",
            dimensions: LINE_ENTITY_DIMENSIONS,
            visible: true,
            position: closePoint,
            linePoints: [ZERO_VEC, farPoint],
            color: color,
            lifetime: 0.1
        });
    }

    this.lineOn = function(closePoint, farPoint, color) {
        // draw a line
        if (this.pointer === null) {
            this.pointer = Entities.addEntity({
                type: "Line",
                name: "grab pointer",
                dimensions: LINE_ENTITY_DIMENSIONS,
                visible: true,
                position: closePoint,
                linePoints: [ZERO_VEC, farPoint],
                color: color,
                lifetime: LIFETIME
            });
        } else {
            var age = Entities.getEntityProperties(this.pointer, "age").age;
            this.pointer = Entities.editEntity(this.pointer, {
                position: closePoint,
                linePoints: [ZERO_VEC, farPoint],
                color: color,
                lifetime: age + LIFETIME
            });
        }
    };

    this.lineOff = function() {
        if (this.pointer !== null) {
            Entities.deleteEntity(this.pointer);
        }
        this.pointer = null;
    };

    this.triggerPress = function (value) {
        _this.rawTriggerValue = value;
    };

    this.bumperPress = function (value) {
        _this.rawBumperValue = value;
    };


    this.updateSmoothedTrigger = function () {
        var triggerValue = this.rawTriggerValue;
        // smooth out trigger value
        this.triggerValue = (this.triggerValue * TRIGGER_SMOOTH_RATIO) +
            (triggerValue * (1.0 - TRIGGER_SMOOTH_RATIO));
    };

    this.triggerSmoothedSqueezed = function() {
        return this.triggerValue > TRIGGER_ON_VALUE;
    };

    this.triggerSmoothedReleased = function() {
        return this.triggerValue < TRIGGER_OFF_VALUE;
    };

    this.triggerSqueezed = function() {
        var triggerValue = this.rawTriggerValue;
        return triggerValue > TRIGGER_ON_VALUE;
    };

    this.bumperSqueezed = function() {
        return _this.rawBumperValue > BUMPER_ON_VALUE;
    }

    this.bumperReleased = function() {
        return _this.rawBumperValue < BUMPER_ON_VALUE;
    }


    this.off = function() {
        if (this.triggerSmoothedSqueezed()) {
            this.lastPickTime = 0;
            this.setState(STATE_SEARCHING);
            return;
        }
        if (this.bumperSqueezed()) {
            this.lastPickTime = 0;
            this.setState(STATE_EQUIP_SEARCHING);
            return;
        }
    }

    this.search = function() {
        this.grabbedEntity = null;

        // if this hand is the one that's disabled, we don't want to search for anything at all
        if (this.hand === disabledHand) {
            return;
        }

        if (this.state == STATE_SEARCHING ? this.triggerSmoothedReleased() : this.bumperReleased()) {
            this.setState(STATE_RELEASE);
            return;
        }

        // the trigger is being pressed, do a ray test
        var handPosition = this.getHandPosition();
        var distantPickRay = {
            origin: handPosition,
            direction: Quat.getUp(this.getHandRotation()),
            length: PICK_MAX_DISTANCE
        };

        // don't pick 60x per second.
        var pickRays = [];
        var now = Date.now();
        if (now - this.lastPickTime > MSECS_PER_SEC / PICKS_PER_SECOND_PER_HAND) {
            pickRays = [distantPickRay];
            this.lastPickTime = now;
        }

        for (var index=0; index < pickRays.length; ++index) {
            var pickRay = pickRays[index];
            var directionNormalized = Vec3.normalize(pickRay.direction);
            var directionBacked = Vec3.multiply(directionNormalized, PICK_BACKOFF_DISTANCE);
            var pickRayBacked = {
                origin: Vec3.subtract(pickRay.origin, directionBacked),
                direction: pickRay.direction
            };

            if (WANT_DEBUG) {
                this.debugLine(pickRayBacked.origin, Vec3.multiply(pickRayBacked.direction, NEAR_PICK_MAX_DISTANCE), {
                    red: 0,
                    green: 255,
                    blue: 0
                })
            }

            var intersection = Entities.findRayIntersection(pickRayBacked, true);

            if (intersection.intersects) {
                // the ray is intersecting something we can move.
                var intersectionDistance = Vec3.distance(pickRay.origin, intersection.intersection);

                //this code will disabled the beam for the opposite hand of the one that grabbed it if the entity says so
                var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, intersection.entityID, DEFAULT_GRABBABLE_DATA);
                if (grabbableData["turnOffOppositeBeam"]) {
                    if (this.hand === RIGHT_HAND) {
                        disabledHand = LEFT_HAND;
                    } else {
                        disabledHand = RIGHT_HAND;
                    }
                } else {
                    disabledHand = 'none';
                }

                if (intersection.properties.name == "Grab Debug Entity") {
                    continue;
                }

                if (typeof grabbableData.grabbable !== 'undefined' && !grabbableData.grabbable) {
                    continue;
                }
                if (intersectionDistance > pickRay.length) {
                    // too far away for this ray.
                    continue;
                }
                if (intersectionDistance <= NEAR_PICK_MAX_DISTANCE) {
                    // the hand is very close to the intersected object.  go into close-grabbing mode.
                    if (grabbableData.wantsTrigger) {
                        this.grabbedEntity = intersection.entityID;
                        this.setState(STATE_NEAR_TRIGGER);
                        return;
                    } else if (!intersection.properties.locked) {
                        this.grabbedEntity = intersection.entityID;
                        if (this.state == STATE_SEARCHING) {
                            this.setState(STATE_NEAR_GRABBING);
                        } else { // equipping
                            if (typeof grabbableData.spatialKey !== 'undefined') {
                                this.setState(STATE_EQUIP_SPRING);
                            } else {
                                this.setState(STATE_EQUIP);
                            }
                        }
                        return;
                    }
                } else if (! entityIsGrabbedByOther(intersection.entityID)) {
                    // don't allow two people to distance grab the same object
                    if (intersection.properties.collisionsWillMove
                        && !intersection.properties.locked) {
                        // the hand is far from the intersected object.  go into distance-holding mode
                        this.grabbedEntity = intersection.entityID;
                        if (typeof grabbableData.spatialKey !== 'undefined' && this.state == STATE_EQUIP_SEARCHING) {
                            // if a distance pick in equip mode hits something with a spatialKey, equip it
                            this.setState(STATE_EQUIP_SPRING);
                            return;
                        } else if (this.state == STATE_SEARCHING) {
                            this.setState(STATE_DISTANCE_HOLDING);
                            return;
                        }
                    } else if (grabbableData.wantsTrigger) {
                        this.grabbedEntity = intersection.entityID;
                        this.setState(STATE_FAR_TRIGGER);
                        return;
                    }
                }
            }
        }

        // forward ray test failed, try sphere test.
        if (WANT_DEBUG) {
            Entities.addEntity({
                type: "Sphere",
                name: "Grab Debug Entity",
                dimensions: {x: GRAB_RADIUS, y: GRAB_RADIUS, z: GRAB_RADIUS},
                visible: true,
                position: handPosition,
                color: { red: 0, green: 255, blue: 0},
                lifetime: 0.1
            });
        }

        var nearbyEntities = Entities.findEntities(handPosition, GRAB_RADIUS);
        var minDistance = PICK_MAX_DISTANCE;
        var i, props, distance, grabbableData;
        this.grabbedEntity = null;
        for (i = 0; i < nearbyEntities.length; i++) {
            var grabbableDataForCandidate =
                getEntityCustomData(GRABBABLE_DATA_KEY, nearbyEntities[i], DEFAULT_GRABBABLE_DATA);
            if (typeof grabbableDataForCandidate.grabbable !== 'undefined' && !grabbableDataForCandidate.grabbable) {
                continue;
            }
            var propsForCandidate =
                Entities.getEntityProperties(nearbyEntities[i], GRABBABLE_PROPERTIES);

            if (propsForCandidate.type == 'Unknown') {
                continue;
            }

            if (propsForCandidate.type == 'Light') {
                continue;
            }

            if (propsForCandidate.type == 'ParticleEffect') {
                continue;
            }

            if (propsForCandidate.type == 'PolyLine') {
                continue;
            }

            if (propsForCandidate.type == 'Zone') {
                continue;
            }

            if (propsForCandidate.locked && !grabbableDataForCandidate.wantsTrigger) {
                continue;
            }

            if (propsForCandidate.name == "Grab Debug Entity") {
                continue;
            }

            if (propsForCandidate.name == "grab pointer") {
                continue;
            }

            distance = Vec3.distance(propsForCandidate.position, handPosition);
            if (distance < minDistance) {
                this.grabbedEntity = nearbyEntities[i];
                minDistance = distance;
                props = propsForCandidate;
                grabbableData = grabbableDataForCandidate;
            }
        }
        if (this.grabbedEntity !== null) {
            if (grabbableData.wantsTrigger) {
                this.setState(STATE_NEAR_TRIGGER);
                return;
            } else if (!props.locked && props.collisionsWillMove) {
                this.setState(this.state == STATE_SEARCHING ? STATE_NEAR_GRABBING : STATE_EQUIP)
                return;
            }
        }

        this.lineOn(distantPickRay.origin, Vec3.multiply(distantPickRay.direction, LINE_LENGTH), NO_INTERSECT_COLOR);
    };

    this.distanceHolding = function() {
        var handControllerPosition = (this.hand === RIGHT_HAND) ? MyAvatar.rightHandPosition : MyAvatar.leftHandPosition;
        var controllerHandInput = (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        var handRotation = Quat.multiply(MyAvatar.orientation, Controller.getPoseValue(controllerHandInput).rotation);
        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);
        var now = Date.now();

        // add the action and initialize some variables
        this.currentObjectPosition = grabbedProperties.position;
        this.currentObjectRotation = grabbedProperties.rotation;
        this.currentObjectTime = now;
        this.handRelativePreviousPosition = Vec3.subtract(handControllerPosition, MyAvatar.position);
        this.handPreviousRotation = handRotation;

        this.actionID = NULL_ACTION_ID;
        this.actionID = Entities.addAction("spring", this.grabbedEntity, {
            targetPosition: this.currentObjectPosition,
            linearTimeScale: DISTANCE_HOLDING_ACTION_TIMEFRAME,
            targetRotation: this.currentObjectRotation,
            angularTimeScale: DISTANCE_HOLDING_ACTION_TIMEFRAME,
            tag: getTag(),
            ttl: ACTION_TTL
        });
        if (this.actionID === NULL_ACTION_ID) {
            this.actionID = null;
        }
        this.actionTimeout = now + (ACTION_TTL * MSEC_PER_SEC);

        if (this.actionID !== null) {
            this.setState(STATE_CONTINUE_DISTANCE_HOLDING);
            this.activateEntity(this.grabbedEntity, grabbedProperties);
            if (this.hand === RIGHT_HAND) {
                Entities.callEntityMethod(this.grabbedEntity, "setRightHand");
            } else {
                Entities.callEntityMethod(this.grabbedEntity, "setLeftHand");
            }
            Entities.callEntityMethod(this.grabbedEntity, "startDistantGrab");
        }

        this.currentAvatarPosition = MyAvatar.position;
        this.currentAvatarOrientation = MyAvatar.orientation;

    };

    this.continueDistanceHolding = function() {
        if (this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            Entities.callEntityMethod(this.grabbedEntity, "releaseGrab");
            return;
        }

        var handPosition = this.getHandPosition();
        var handControllerPosition = (this.hand === RIGHT_HAND) ? MyAvatar.rightHandPosition : MyAvatar.leftHandPosition;
        var controllerHandInput = (this.hand === RIGHT_HAND) ? Controller.Standard.RightHand : Controller.Standard.LeftHand;
        var handRotation = Quat.multiply(MyAvatar.orientation, Controller.getPoseValue(controllerHandInput).rotation);
        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);
        var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, this.grabbedEntity, DEFAULT_GRABBABLE_DATA);

        if (this.state == STATE_CONTINUE_DISTANCE_HOLDING && this.bumperSqueezed() &&
            typeof grabbableData.spatialKey !== 'undefined') {
            var saveGrabbedID = this.grabbedEntity;
            this.release();
            this.setState(STATE_EQUIP);
            this.grabbedEntity = saveGrabbedID;
            return;
        }

        this.lineOn(handPosition, Vec3.subtract(grabbedProperties.position, handPosition), INTERSECT_COLOR);

        // the action was set up on a previous call.  update the targets.
        var radius = Math.max(Vec3.distance(this.currentObjectPosition, handControllerPosition) *
                              DISTANCE_HOLDING_RADIUS_FACTOR, DISTANCE_HOLDING_RADIUS_FACTOR);
        // how far did avatar move this timestep?
        var currentPosition = MyAvatar.position;
        var avatarDeltaPosition = Vec3.subtract(currentPosition, this.currentAvatarPosition);
        this.currentAvatarPosition = currentPosition;

        // How far did the avatar turn this timestep?
        // Note:  The following code is too long because we need a Quat.quatBetween() function
        // that returns the minimum quaternion between two quaternions.
        var currentOrientation = MyAvatar.orientation;
        if (Quat.dot(currentOrientation, this.currentAvatarOrientation) < 0.0) {
            var negativeCurrentOrientation = {
                x: -currentOrientation.x,
                y: -currentOrientation.y,
                z: -currentOrientation.z,
                w: -currentOrientation.w
            };
            var avatarDeltaOrientation = Quat.multiply(negativeCurrentOrientation, Quat.inverse(this.currentAvatarOrientation));
        } else {
            var avatarDeltaOrientation = Quat.multiply(currentOrientation, Quat.inverse(this.currentAvatarOrientation));
        }
        var handToAvatar = Vec3.subtract(handControllerPosition, this.currentAvatarPosition);
        var objectToAvatar = Vec3.subtract(this.currentObjectPosition, this.currentAvatarPosition);
        var handMovementFromTurning = Vec3.subtract(Quat.multiply(avatarDeltaOrientation, handToAvatar), handToAvatar);
        var objectMovementFromTurning = Vec3.subtract(Quat.multiply(avatarDeltaOrientation, objectToAvatar), objectToAvatar);
        this.currentAvatarOrientation = currentOrientation;

        // how far did hand move this timestep?
        var handMoved = Vec3.subtract(handToAvatar, this.handRelativePreviousPosition);
        this.handRelativePreviousPosition = handToAvatar;

        //  magnify the hand movement but not the change from avatar movement & rotation
        handMoved = Vec3.subtract(handMoved, handMovementFromTurning);
        var superHandMoved = Vec3.multiply(handMoved, radius);

        //  Move the object by the magnified amount and then by amount from avatar movement & rotation
        var newObjectPosition = Vec3.sum(this.currentObjectPosition, superHandMoved);
        newObjectPosition = Vec3.sum(newObjectPosition, avatarDeltaPosition);
        newObjectPosition = Vec3.sum(newObjectPosition, objectMovementFromTurning);

        var deltaPosition = Vec3.subtract(newObjectPosition, this.currentObjectPosition); // meters
        var now = Date.now();
        var deltaTime = (now - this.currentObjectTime) / MSEC_PER_SEC; // convert to seconds

        this.currentObjectPosition = newObjectPosition;
        this.currentObjectTime = now;

        // this doubles hand rotation
        var handChange = Quat.multiply(Quat.slerp(this.handPreviousRotation,
                                                  handRotation,
                                                  DISTANCE_HOLDING_ROTATION_EXAGGERATION_FACTOR),
                                       Quat.inverse(this.handPreviousRotation));
        this.handPreviousRotation = handRotation;
        this.currentObjectRotation = Quat.multiply(handChange, this.currentObjectRotation);

        Entities.callEntityMethod(this.grabbedEntity, "continueDistantGrab");

        Entities.updateAction(this.grabbedEntity, this.actionID, {
            targetPosition: this.currentObjectPosition,
            linearTimeScale: DISTANCE_HOLDING_ACTION_TIMEFRAME,
            targetRotation: this.currentObjectRotation,
            angularTimeScale: DISTANCE_HOLDING_ACTION_TIMEFRAME,
            ttl: ACTION_TTL
        });
        this.actionTimeout = now + (ACTION_TTL * MSEC_PER_SEC);
    };

    this.nearGrabbing = function() {
        var now = Date.now();

        var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, this.grabbedEntity, DEFAULT_GRABBABLE_DATA);

        var turnOffOtherHand = grabbableData["turnOffOtherHand"];
        if (turnOffOtherHand) {
            //don't activate the second hand grab because the script is handling the second hand logic
            return;
        }

        if (this.state == STATE_NEAR_GRABBING && this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            Entities.callEntityMethod(this.grabbedEntity, "releaseGrab");
            return;
        }

        this.lineOff();

        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);
        this.activateEntity(this.grabbedEntity, grabbedProperties);
        if (grabbedProperties.collisionsWillMove && NEAR_GRABBING_KINEMATIC) {
            Entities.editEntity(this.grabbedEntity, {
                collisionsWillMove: false
            });
        }

        var handRotation = this.getHandRotation();
        var handPosition = this.getHandPosition();

        var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, this.grabbedEntity, DEFAULT_GRABBABLE_DATA);

        if (this.state != STATE_NEAR_GRABBING && grabbableData.spatialKey) {
            // if an object is "equipped" and has a spatialKey, use it.
            if (grabbableData.spatialKey.relativePosition) {
                this.offsetPosition = grabbableData.spatialKey.relativePosition;
            }
            if (grabbableData.spatialKey.relativeRotation) {
                this.offsetRotation = grabbableData.spatialKey.relativeRotation;
            }
        } else {
            var objectRotation = grabbedProperties.rotation;
            this.offsetRotation = Quat.multiply(Quat.inverse(handRotation), objectRotation);

            var currentObjectPosition = grabbedProperties.position;
            var offset = Vec3.subtract(currentObjectPosition, handPosition);
            this.offsetPosition = Vec3.multiplyQbyV(Quat.inverse(Quat.multiply(handRotation, this.offsetRotation)), offset);
        }

        this.actionID = NULL_ACTION_ID;
        this.actionID = Entities.addAction("hold", this.grabbedEntity, {
            hand: this.hand === RIGHT_HAND ? "right" : "left",
            timeScale: NEAR_GRABBING_ACTION_TIMEFRAME,
            relativePosition: this.offsetPosition,
            relativeRotation: this.offsetRotation,
            ttl: ACTION_TTL,
            kinematic: NEAR_GRABBING_KINEMATIC,
            kinematicSetVelocity: true
        });
        if (this.actionID === NULL_ACTION_ID) {
            this.actionID = null;
        } else {
            this.actionTimeout = now + (ACTION_TTL * MSEC_PER_SEC);
            if (this.state == STATE_NEAR_GRABBING) {
                this.setState(STATE_CONTINUE_NEAR_GRABBING);
            } else {
                // equipping
                this.setState(STATE_CONTINUE_EQUIP_BD);
            }

            if (this.hand === RIGHT_HAND) {
                Entities.callEntityMethod(this.grabbedEntity, "setRightHand");
            } else {
                Entities.callEntityMethod(this.grabbedEntity, "setLeftHand");
            }
            Entities.callEntityMethod(this.grabbedEntity, "startNearGrab");

        }

        this.currentHandControllerTipPosition =
            (this.hand === RIGHT_HAND) ? MyAvatar.rightHandTipPosition : MyAvatar.leftHandTipPosition;

        this.currentObjectTime = Date.now();

        print("handler on");
        // copy some stuff into globals where the animation state handler can get at it
        dragEntityID[this.hand] = this.grabbedEntity;
        dragOffsetPosition[this.hand] = this.offsetPosition;
        dragOffsetRotation[this.hand] = this.offsetRotation;
        if (this.hand == RIGHT_HAND) {
            this.handlerId = MyAvatar.addAnimationStateHandler(dragHandRight, ['rightHandPosition']);
        } else {
            this.handlerId = MyAvatar.addAnimationStateHandler(dragHandLeft, ['leftHandPosition']);
        }
    };

    this.continueNearGrabbing = function() {
        if (this.state == STATE_CONTINUE_NEAR_GRABBING && this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            Entities.callEntityMethod(this.grabbedEntity, "releaseGrab");
            return;
        }
        if (this.state == STATE_CONTINUE_EQUIP_BD && this.bumperReleased()) {
            this.setState(STATE_CONTINUE_EQUIP);
            return;
        }
        if (this.state == STATE_CONTINUE_EQUIP && this.bumperSqueezed()) {
            this.setState(STATE_WAITING_FOR_BUMPER_RELEASE);
            return;
        }
        if (this.state == STATE_CONTINUE_NEAR_GRABBING && this.bumperSqueezed()) {
            this.setState(STATE_CONTINUE_EQUIP_BD);
            return;
        }

        // Keep track of the fingertip velocity to impart when we release the object.
        // Note that the idea of using a constant 'tip' velocity regardless of the
        // object's actual held offset is an idea intended to make it easier to throw things:
        // Because we might catch something or transfer it between hands without a good idea
        // of it's actual offset, let's try imparting a velocity which is at a fixed radius
        // from the palm.

        var handControllerPosition = (this.hand === RIGHT_HAND) ? MyAvatar.rightHandPosition : MyAvatar.leftHandPosition;
        var now = Date.now();

        var deltaPosition = Vec3.subtract(handControllerPosition, this.currentHandControllerTipPosition); // meters
        var deltaTime = (now - this.currentObjectTime) / MSEC_PER_SEC; // convert to seconds

        this.currentHandControllerTipPosition = handControllerPosition;
        this.currentObjectTime = now;
        Entities.callEntityMethod(this.grabbedEntity, "continueNearGrab");

        if (this.actionTimeout - now < ACTION_TTL_REFRESH * MSEC_PER_SEC) {
            // if less than a 5 seconds left, refresh the actions ttl
            Entities.updateAction(this.grabbedEntity, this.actionID, {
                hand: this.hand === RIGHT_HAND ? "right" : "left",
                timeScale: NEAR_GRABBING_ACTION_TIMEFRAME,
                relativePosition: this.offsetPosition,
                relativeRotation: this.offsetRotation,
                ttl: ACTION_TTL,
                kinematic: NEAR_GRABBING_KINEMATIC,
                kinematicSetVelocity: true
            });
            this.actionTimeout = now + (ACTION_TTL * MSEC_PER_SEC);
        }
    };

    this.waitingForBumperRelease = function() {
        if (this.bumperReleased()) {
            this.setState(STATE_RELEASE);
            Entities.callEntityMethod(this.grabbedEntity, "releaseGrab");
        }
    };

    this.pullTowardEquipPosition = function() {
        this.lineOff();

        var grabbedProperties = Entities.getEntityProperties(this.grabbedEntity, GRABBABLE_PROPERTIES);
        var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, this.grabbedEntity, DEFAULT_GRABBABLE_DATA);

        // use a spring to pull the object to where it will be when equipped
        var relativeRotation = { x: 0.0, y: 0.0, z: 0.0, w: 1.0 };
        var relativePosition = { x: 0.0, y: 0.0, z: 0.0 };
        if (grabbableData.spatialKey.relativePosition) {
            relativePosition = grabbableData.spatialKey.relativePosition;
        }
        if (grabbableData.spatialKey.relativeRotation) {
            relativeRotation = grabbableData.spatialKey.relativeRotation;
        }
        var handRotation = this.getHandRotation();
        var handPosition = this.getHandPosition();
        var targetRotation = Quat.multiply(handRotation, relativeRotation);
        var offset = Vec3.multiplyQbyV(targetRotation, relativePosition);
        var targetPosition = Vec3.sum(handPosition, offset);

        if (typeof this.equipSpringID === 'undefined' ||
            this.equipSpringID === null ||
            this.equipSpringID === NULL_ACTION_ID) {
            this.equipSpringID = Entities.addAction("spring", this.grabbedEntity, {
                targetPosition: targetPosition,
                linearTimeScale: EQUIP_SPRING_TIMEFRAME,
                targetRotation: targetRotation,
                angularTimeScale: EQUIP_SPRING_TIMEFRAME,
                ttl: ACTION_TTL
            });
            if (this.equipSpringID === NULL_ACTION_ID) {
                this.equipSpringID = null;
                this.setState(STATE_OFF);
                return;
            }
        } else {
            Entities.updateAction(this.grabbedEntity, this.equipSpringID, {
                targetPosition: targetPosition,
                linearTimeScale: EQUIP_SPRING_TIMEFRAME,
                targetRotation: targetRotation,
                angularTimeScale: EQUIP_SPRING_TIMEFRAME,
                ttl: ACTION_TTL
            });
        }

        if (Vec3.distance(grabbedProperties.position, targetPosition) < EQUIP_SPRING_SHUTOFF_DISTANCE) {
            Entities.deleteAction(this.grabbedEntity, this.equipSpringID);
            this.equipSpringID = null;
            this.setState(STATE_EQUIP);
        }
    };

    this.nearTrigger = function() {
        if (this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            Entities.callEntityMethod(this.grabbedEntity, "stopNearTrigger");
            return;
        }
        if (this.hand === RIGHT_HAND) {
            Entities.callEntityMethod(this.grabbedEntity, "setRightHand");
        } else {
            Entities.callEntityMethod(this.grabbedEntity, "setLeftHand");
        }
        Entities.callEntityMethod(this.grabbedEntity, "startNearTrigger");
        this.setState(STATE_CONTINUE_NEAR_TRIGGER);
    };

    this.farTrigger = function() {
        if (this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            Entities.callEntityMethod(this.grabbedEntity, "stopFarTrigger");
            return;
        }

        if (this.hand === RIGHT_HAND) {
            Entities.callEntityMethod(this.grabbedEntity, "setRightHand");
        } else {
            Entities.callEntityMethod(this.grabbedEntity, "setLeftHand");
        }
        Entities.callEntityMethod(this.grabbedEntity, "startFarTrigger");
        this.setState(STATE_CONTINUE_FAR_TRIGGER);
    };

    this.continueNearTrigger = function() {
        if (this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            Entities.callEntityMethod(this.grabbedEntity, "stopNearTrigger");
            return;
        }

        Entities.callEntityMethod(this.grabbedEntity, "continueNearTrigger");
    };

    this.continueFarTrigger = function() {
        if (this.triggerSmoothedReleased()) {
            this.setState(STATE_RELEASE);
            Entities.callEntityMethod(this.grabbedEntity, "stopNearTrigger");
            return;
        }

        var handPosition = this.getHandPosition();
        var pickRay = {
            origin: handPosition,
            direction: Quat.getUp(this.getHandRotation())
        };

        var now = Date.now();
        if (now - this.lastPickTime > MSECS_PER_SEC / PICKS_PER_SECOND_PER_HAND) {
            var intersection = Entities.findRayIntersection(pickRay, true);
            this.lastPickTime = now;
            if (intersection.entityID != this.grabbedEntity) {
                this.setState(STATE_RELEASE);
                Entities.callEntityMethod(this.grabbedEntity, "stopFarTrigger");
                return;
            }
        }

        this.lineOn(pickRay.origin, Vec3.multiply(pickRay.direction, LINE_LENGTH), NO_INTERSECT_COLOR);
        Entities.callEntityMethod(this.grabbedEntity, "continueFarTrigger");
    };

    _this.allTouchedIDs = {};
    this.touchTest = function() {
        var maxDistance = 0.05;
        var leftHandPosition = MyAvatar.getLeftPalmPosition();
        var rightHandPosition = MyAvatar.getRightPalmPosition();
        var leftEntities = Entities.findEntities(leftHandPosition, maxDistance);
        var rightEntities = Entities.findEntities(rightHandPosition, maxDistance);
        var ids = [];

        if (leftEntities.length !== 0) {
            leftEntities.forEach(function(entity) {
                ids.push(entity);
            });

        }

        if (rightEntities.length !== 0) {
            rightEntities.forEach(function(entity) {
                ids.push(entity);
            });
        }

        ids.forEach(function(id) {

            var props = Entities.getEntityProperties(id, ["boundingBox", "name"]);
            if (props.name === 'pointer') {
                return;
            } else {
                var entityMinPoint = props.boundingBox.brn;
                var entityMaxPoint = props.boundingBox.tfl;
                var leftIsTouching = pointInExtents(leftHandPosition, entityMinPoint, entityMaxPoint);
                var rightIsTouching = pointInExtents(rightHandPosition, entityMinPoint, entityMaxPoint);

                if ((leftIsTouching || rightIsTouching) && _this.allTouchedIDs[id] === undefined) {
                    // we haven't been touched before, but either right or left is touching us now
                    _this.allTouchedIDs[id] = true;
                    _this.startTouch(id);
                } else if ((leftIsTouching || rightIsTouching) && _this.allTouchedIDs[id]) {
                    // we have been touched before and are still being touched
                    // continue touch
                    _this.continueTouch(id);
                } else if (_this.allTouchedIDs[id]) {
                    delete _this.allTouchedIDs[id];
                    _this.stopTouch(id);

                } else {
                    //we are in another state
                    return;
                }
            }

        });

    };

    this.startTouch = function(entityID) {
        Entities.callEntityMethod(entityID, "startTouch");
    };

    this.continueTouch = function(entityID) {
        Entities.callEntityMethod(entityID, "continueTouch");
    };

    this.stopTouch = function(entityID) {
        Entities.callEntityMethod(entityID, "stopTouch");
    };

    this.release = function() {

        if(this.hand !== disabledHand){
            //release the disabled hand when we let go with the main one
            disabledHand = 'none';
        }
        this.lineOff();

        if (this.grabbedEntity !== null) {
            if (this.actionID !== null) {
                Entities.deleteAction(this.grabbedEntity, this.actionID);
            }
        }

        this.deactivateEntity(this.grabbedEntity);

        this.grabbedEntity = null;
        this.actionID = null;
        this.setState(STATE_OFF);

        print("handler off");
        MyAvatar.removeAnimationStateHandler(this.handlerId);
        this.handlerId = null;
    };

    this.cleanup = function() {
        this.release();
    };

    this.activateEntity = function(entityID, grabbedProperties) {
        var grabbableData = getEntityCustomData(GRABBABLE_DATA_KEY, entityID, DEFAULT_GRABBABLE_DATA);
        var invertSolidWhileHeld = grabbableData["invertSolidWhileHeld"];
        var data = getEntityCustomData(GRAB_USER_DATA_KEY, entityID, {});
        data["activated"] = true;
        data["avatarId"] = MyAvatar.sessionUUID;
        data["refCount"] = data["refCount"] ? data["refCount"] + 1 : 1;
        // zero gravity and set ignoreForCollisions in a way that lets us put them back, after all grabs are done
        if (data["refCount"] == 1) {
            data["gravity"] = grabbedProperties.gravity;
            data["ignoreForCollisions"] = grabbedProperties.ignoreForCollisions;
            data["collisionsWillMove"] = grabbedProperties.collisionsWillMove;
            var whileHeldProperties = {gravity: {x:0, y:0, z:0}};
            if (invertSolidWhileHeld) {
                whileHeldProperties["ignoreForCollisions"] = ! grabbedProperties.ignoreForCollisions;
            }
            Entities.editEntity(entityID, whileHeldProperties);
        }

        setEntityCustomData(GRAB_USER_DATA_KEY, entityID, data);
        return data;
    };

    this.deactivateEntity = function(entityID) {
        var data = getEntityCustomData(GRAB_USER_DATA_KEY, entityID, {});
        if (data && data["refCount"]) {
            data["refCount"] = data["refCount"] - 1;
            if (data["refCount"] < 1) {
                Entities.editEntity(entityID, {
                    gravity: data["gravity"],
                    ignoreForCollisions: data["ignoreForCollisions"],
                    collisionsWillMove: data["collisionsWillMove"]
                });
                data = null;
            }
        } else {
            data = null;
        }
        setEntityCustomData(GRAB_USER_DATA_KEY, entityID, data);
    };
}

var rightController = new MyController(RIGHT_HAND);
var leftController = new MyController(LEFT_HAND);

var MAPPING_NAME = "com.highfidelity.handControllerGrab";

var mapping = Controller.newMapping(MAPPING_NAME);
mapping.from([Controller.Standard.RT]).peek().to(rightController.triggerPress);
mapping.from([Controller.Standard.LT]).peek().to(leftController.triggerPress);

mapping.from([Controller.Standard.RB]).peek().to(rightController.bumperPress);
mapping.from([Controller.Standard.LB]).peek().to(leftController.bumperPress);

Controller.enableMapping(MAPPING_NAME);


function update() {
    rightController.update();
    leftController.update();
}

function cleanup() {
    rightController.cleanup();
    leftController.cleanup();
    Controller.disableMapping(MAPPING_NAME);
}

Script.scriptEnding.connect(cleanup);
Script.update.connect(update);
