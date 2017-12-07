//
//  debugHighlight.js
//  developer/utilities/render
//
//  Olivier Prat, created on 08/08/2017.
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Set up the qml ui
var qml = Script.resolvePath('highlight.qml');
var window = new OverlayWindow({
    title: 'Highlight',
    source: qml,
    width: 400, 
    height: 400,
});
window.closed.connect(function() { Script.stop(); });

"use strict";

// Created by Sam Gondelman on 9/7/2017
// Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

(function() { // BEGIN LOCAL_SCOPE

var END_DIMENSIONS = {
    x: 0.15,
    y: 0.15,
    z: 0.15
};
var COLOR = {red: 97, green: 247, blue: 255};
var end = {
    type: "sphere",
    dimensions: END_DIMENSIONS,
    color: COLOR,
    ignoreRayIntersection: true,
    alpha: 1.0,
    visible: true
}

var COLOR2 = {red: 247, green: 97, blue: 255};
var end2 = {
    type: "sphere",
    dimensions: END_DIMENSIONS,
    color: COLOR2,
    ignoreRayIntersection: true,
    alpha: 1.0,
    visible: true
}

var highlightGroupIndex = 0
var isSelectionAddEnabled = false
var isSelectionEnabled = false
var renderStates = [{name: "test", end: end}];
var defaultRenderStates = [{name: "test", distance: 20.0, end: end2}];
var time = 0

var ray = LaserPointers.createLaserPointer({
    joint: "Mouse",
    filter: RayPick.PICK_ENTITIES | RayPick.PICK_OVERLAYS | RayPick.PICK_AVATARS | RayPick.PICK_INVISIBLE | RayPick.PICK_NONCOLLIDABLE,
    renderStates: renderStates,
    defaultRenderStates: defaultRenderStates,
    enabled: false
});

function getSelectionName() {
    var selectionName = "contextOverlayHighlightList"

    if (highlightGroupIndex>0) {
        selectionName += highlightGroupIndex
    }
    return selectionName
}

function fromQml(message) {
    tokens = message.split(' ')
    print("Received '"+message+"' from hightlight.qml")
    if (tokens[0]=="highlight") {
        highlightGroupIndex = parseInt(tokens[1])
        print("Switching to highlight group "+highlightGroupIndex)
    } else if (tokens[0]=="pick") {
        isSelectionEnabled = tokens[1]=='true'
        print("Ray picking set to "+isSelectionEnabled.toString())
        if (isSelectionEnabled) {
            LaserPointers.enableLaserPointer(ray)
        } else {
            LaserPointers.disableLaserPointer(ray)
        }
        time = 0
    } else if (tokens[0]=="add") {
        isSelectionAddEnabled = tokens[1]=='true'
        print("Add to selection set to "+isSelectionAddEnabled.toString())
        if (!isSelectionAddEnabled) {
            Selection.clearSelectedItemsList(getSelectionName())
        }
    }
}

window.fromQml.connect(fromQml);

function cleanup() {
    LaserPointers.removeLaserPointer(ray);
}
Script.scriptEnding.connect(cleanup);

var prevID = 0
var prevType = ""
var selectedID = 0
var selectedType = ""
function update(deltaTime) {

    // you have to do this repeatedly because there's a bug but I'll fix it
    LaserPointers.setRenderState(ray, "test");

    var result = LaserPointers.getPrevRayPickResult(ray);
    var selectionName = getSelectionName()

    if (isSelectionEnabled && result.type != RayPick.INTERSECTED_NONE) {
        time += deltaTime
        if (result.objectID != prevID) { 
            var typeName = ""
            if (result.type == RayPick.INTERSECTED_ENTITY) {
                typeName = "entity"
            } else if (result.type == RayPick.INTERSECTED_OVERLAY) {
                typeName = "overlay"
            } else if (result.type == RayPick.INTERSECTED_AVATAR) {
                typeName = "avatar"
            }
            
            prevID = result.objectID;
            prevType = typeName;
            time = 0
        } else if (time>1.0 && prevID!=selectedID) {
            if (prevID != 0 && !isSelectionAddEnabled) {
                Selection.removeFromSelectedItemsList(selectionName, selectedType, selectedID)
            }
            selectedID = prevID
            selectedType = prevType
            Selection.addToSelectedItemsList(selectionName, selectedType, selectedID)
            print("HIGHLIGHT " + highlightGroupIndex + " picked type: " + result.type + ", id: " + result.objectID);
        }
    } else {
        if (prevID != 0 && !isSelectionAddEnabled) {
            Selection.removeFromSelectedItemsList(selectionName, prevType, prevID)
        }
        prevID = 0
        selectedID = 0
        time = 0
    }
}

Script.update.connect(update);

}()); // END LOCAL_SCOPE