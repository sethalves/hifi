"use strict";

// Created by james b. pollack @imgntn on 8/18/2016
// Copyright 2016 High Fidelity, Inc.
//
// advanced movements settings are in individual controller json files
// what we do is check the status of the 'advance movement' checkbox when you enter HMD mode
// if 'advanced movement' is checked...we give you the defaults that are in the json.
// if 'advanced movement' is not checked... we override the advanced controls with basic ones.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() { // BEGIN LOCAL_SCOPE

var mappingName, basicMapping, isChecked;

var TURN_RATE = 1000;
var MENU_ITEM_NAME = "Advanced Movement For Hand Controllers";
var isDisabled = false;
var previousSetting = MyAvatar.useAdvancedMovementControls;
if (previousSetting === false) {
    previousSetting = false;
    isChecked = false;
}

if (previousSetting === true) {
    previousSetting = true;
    isChecked = true;
}

function addAdvancedMovementItemToSettingsMenu() {
    Script.beginProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.addAdvancedMovementItemToSettingsMenu");
    Menu.addMenuItem({
        menuName: "Settings",
        menuItemName: MENU_ITEM_NAME,
        isCheckable: true,
        isChecked: previousSetting
    });
    Script.endProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.addAdvancedMovementItemToSettingsMenu");
}

function rotate180() {
    Script.beginProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.rotate180");
    var newOrientation = Quat.multiply(MyAvatar.orientation, Quat.angleAxis(180, {
        x: 0,
        y: 1,
        z: 0
    }))
    MyAvatar.orientation = newOrientation
    Script.endProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.rotate180");
}

var inFlipTurn = false;

function registerBasicMapping() {
    Script.beginProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.registerBasicMapping");
    mappingName = 'Hifi-AdvancedMovement-Dev-' + Math.random();
    basicMapping = Controller.newMapping(mappingName);
    basicMapping.from(Controller.Standard.LY).to(function(value) {
        if (isDisabled) {
            return;
        }
        var stick = Controller.getValue(Controller.Standard.LS);
        if (value === 1 && Controller.Hardware.OculusTouch !== undefined) {
            rotate180();
        } else if (Controller.Hardware.Vive !== undefined) {
            if (value > 0.75 && inFlipTurn === false) {
                inFlipTurn = true;
                rotate180();
                Script.setTimeout(function() {
                    inFlipTurn = false;
                }, TURN_RATE)
            }
        }
        return;
    });
    basicMapping.from(Controller.Standard.RY).to(function(value) {
        if (isDisabled) {
            return;
        }
        var stick = Controller.getValue(Controller.Standard.RS);
        if (value === 1 && Controller.Hardware.OculusTouch !== undefined) {
            rotate180();
        } else if (Controller.Hardware.Vive !== undefined) {
            if (value > 0.75 && inFlipTurn === false) {
                inFlipTurn = true;
                rotate180();
                Script.setTimeout(function() {
                    inFlipTurn = false;
                }, TURN_RATE)
            }
        }
        return;
    })
    Script.endProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.registerBasicMapping");
}


function enableMappings() {
    Script.beginProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.enableMappings");
    Controller.enableMapping(mappingName);
    Script.endProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.enableMappings");
}

function disableMappings() {
    Script.beginProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.disableMappings");
    Controller.disableMapping(mappingName);
    Script.endProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.disableMappings");
}

function scriptEnding() {
    Script.beginProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.scriptEnding");
    Menu.removeMenuItem("Settings", MENU_ITEM_NAME);
    disableMappings();
    Script.endProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.scriptEnding");
}


function menuItemEvent(menuItem) {
    Script.beginProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.menuItemEvent");
    if (menuItem == MENU_ITEM_NAME) {
        isChecked = Menu.isOptionChecked(MENU_ITEM_NAME);
        if (isChecked === true) {
            MyAvatar.useAdvancedMovementControls = true;
            disableMappings();
        } else if (isChecked === false) {
            MyAvatar.useAdvancedMovementControls = false;
            enableMappings();
        }
    }
    Script.endProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.menuItemEvent");
}

addAdvancedMovementItemToSettingsMenu();

Script.scriptEnding.connect(scriptEnding);

Menu.menuItemEvent.connect(menuItemEvent);

registerBasicMapping();

Script.setTimeout(function() {
    Script.beginProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.timeout");
    if (previousSetting === true) {
        disableMappings();
    } else {
        enableMappings();
    }
    Script.endProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.timeout");
}, 100)


HMD.displayModeChanged.connect(function(isHMDMode) {
    Script.beginProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.displayModeChanged");
    if (isHMDMode) {
        if (Controller.Hardware.Vive !== undefined || Controller.Hardware.OculusTouch !== undefined) {
            if (isChecked === true) {
                disableMappings();
            } else if (isChecked === false) {
                enableMappings();
            }

        }
    }
    Script.endProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.displayModeChanged");
});


var HIFI_ADVANCED_MOVEMENT_DISABLER_CHANNEL = 'Hifi-Advanced-Movement-Disabler';
function handleMessage(channel, message, sender) {
    Script.beginProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.handleMessage");
    if (channel == HIFI_ADVANCED_MOVEMENT_DISABLER_CHANNEL) {
        if (message == 'disable') {
            isDisabled = true;
        } else if (message == 'enable') {
            isDisabled = false;
        }
    }
    Script.endProfileRange("controllerScripts.toggleAdvancedMovementForHandControllers.handleMessage");
}

Messages.subscribe(HIFI_ADVANCED_MOVEMENT_DISABLER_CHANNEL);
Messages.messageReceived.connect(handleMessage);

}()); // END LOCAL_SCOPE
