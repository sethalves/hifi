//
//  GrabPropertyGroup.h
//  libraries/entities/src
//
//  Created by Seth Alves on 2018-8-8.
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "GrabPropertyGroup.h"

#include <OctreePacketData.h>

#include "EntityItemProperties.h"
#include "EntityItemPropertiesMacros.h"

void GrabPropertyGroup::copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties,
                                          QScriptEngine* engine, bool skipDefaults,
                                          EntityItemProperties& defaultEntityProperties) const {
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_GRAB_GRABBABLE, Grab, grab, Grabbable, grabbable);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_GRAB_KINEMATIC, Grab, grab, GrabKinematic, grabKinematic);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_GRAB_FOLLOWS_CONTROLLER, Grab, grab, GrabFollowsController, grabFollowsController);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_GRAB_TRIGGERABLE, Grab, grab, Triggerable, triggerable);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_GRAB_EQUIPPABLE, Grab, grab, Equippable, equippable);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_GRAB_LEFT_EQUIPPABLE_POSITION_OFFSET, Grab, grab,
                                        EquippableLeftPosition, equippableLeftPosition);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_GRAB_LEFT_EQUIPPABLE_ROTATION_OFFSET, Grab, grab,
                                        EquippableLeftRotation, equippableLeftRotation);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_GRAB_RIGHT_EQUIPPABLE_POSITION_OFFSET, Grab, grab,
                                        EquippableRightPosition, equippableRightPosition);
    COPY_GROUP_PROPERTY_TO_QSCRIPTVALUE(PROP_GRAB_RIGHT_EQUIPPABLE_ROTATION_OFFSET, Grab, grab,
                                        EquippableRightRotation, equippableRightRotation);
}

void GrabPropertyGroup::copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) {
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(grab, grabbable, bool, setGrabbable);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(grab, grabKinematic, bool, setGrabKinematic);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(grab, grabFollowsController, bool, setGrabFollowsController);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(grab, triggerable, bool, setTriggerable);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(grab, equippableLeftPosition, vec3, setEquippableLeftPosition);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(grab, equippableLeftRotation, quat, setEquippableLeftRotation);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(grab, equippableRightPosition, vec3, setEquippableRightPosition);
    COPY_GROUP_PROPERTY_FROM_QSCRIPTVALUE(grab, equippableRightRotation, quat, setEquippableRightRotation);
}

void GrabPropertyGroup::merge(const GrabPropertyGroup& other) {
    COPY_PROPERTY_IF_CHANGED(grabbable);
    COPY_PROPERTY_IF_CHANGED(grabKinematic);
    COPY_PROPERTY_IF_CHANGED(triggerable);
    COPY_PROPERTY_IF_CHANGED(equippable);
    COPY_PROPERTY_IF_CHANGED(equippableLeftPosition);
    COPY_PROPERTY_IF_CHANGED(equippableLeftRotation);
    COPY_PROPERTY_IF_CHANGED(equippableRightPosition);
    COPY_PROPERTY_IF_CHANGED(equippableRightRotation);
}

void GrabPropertyGroup::debugDump() const {
    qCDebug(entities) << "   GrabPropertyGroup: ---------------------------------------------";

    qCDebug(entities) << "            _grabbable:" << _grabbable;
    qCDebug(entities) << "            _grabKinematic:" << _grabKinematic;
    qCDebug(entities) << "            _grabFollowsController:" << _grabFollowsController;
    qCDebug(entities) << "            _triggerable:" << _triggerable;
    qCDebug(entities) << "            _equippable:" << _equippable;
    qCDebug(entities) << "            _equippableLeftPosition:" << _equippableLeftPosition;
    qCDebug(entities) << "            _equippableLeftRotation:" << _equippableLeftRotation;
    qCDebug(entities) << "            _equippableRightPosition:" << _equippableRightPosition;
    qCDebug(entities) << "            _equippableRightRotation:" << _equippableRightRotation;
}

void GrabPropertyGroup::listChangedProperties(QList<QString>& out) {
    if (grabbableChanged()) {
        out << "grab-grabbable";
    }
    if (grabKinematicChanged()) {
        out << "grab-grabKinematic";
    }
    if (triggerableChanged()) {
        out << "grab-triggerable";
    }
    if (equippableChanged()) {
        out << "grab-equippable";
    }
    if (equippableLeftPositionChanged()) {
        out << "grab-equippableLeftPosition";
    }
    if (equippableLeftRotationChanged()) {
        out << "grab-equippableLeftRotation";
    }
    if (equippableRightPositionChanged()) {
        out << "grab-equippableRightPosition";
    }
    if (equippableRightRotationChanged()) {
        out << "grab-equippableRightRotation";
    }
}

bool GrabPropertyGroup::appendToEditPacket(OctreePacketData* packetData,
                                           EntityPropertyFlags& requestedProperties,
                                           EntityPropertyFlags& propertyFlags,
                                           EntityPropertyFlags& propertiesDidntFit,
                                           int& propertyCount,
                                           OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_GRAB_GRABBABLE, getGrabbable());
    APPEND_ENTITY_PROPERTY(PROP_GRAB_KINEMATIC, getGrabKinematic());
    APPEND_ENTITY_PROPERTY(PROP_GRAB_FOLLOWS_CONTROLLER, getGrabFollowsController());
    APPEND_ENTITY_PROPERTY(PROP_GRAB_TRIGGERABLE, getTriggerable());
    APPEND_ENTITY_PROPERTY(PROP_GRAB_EQUIPPABLE, getEquippable());
    APPEND_ENTITY_PROPERTY(PROP_GRAB_LEFT_EQUIPPABLE_POSITION_OFFSET, getEquippableLeftPosition());
    APPEND_ENTITY_PROPERTY(PROP_GRAB_LEFT_EQUIPPABLE_ROTATION_OFFSET, getEquippableLeftRotation());
    APPEND_ENTITY_PROPERTY(PROP_GRAB_RIGHT_EQUIPPABLE_POSITION_OFFSET, getEquippableRightPosition());
    APPEND_ENTITY_PROPERTY(PROP_GRAB_RIGHT_EQUIPPABLE_ROTATION_OFFSET, getEquippableRightRotation());

    return true;
}

bool GrabPropertyGroup::decodeFromEditPacket(EntityPropertyFlags& propertyFlags,
                                             const unsigned char*& dataAt , int& processedBytes) {

    int bytesRead = 0;
    bool overwriteLocalData = true;
    bool somethingChanged = false;

    READ_ENTITY_PROPERTY(PROP_GRAB_GRABBABLE, bool, setGrabbable);
    READ_ENTITY_PROPERTY(PROP_GRAB_KINEMATIC, bool, setGrabKinematic);
    READ_ENTITY_PROPERTY(PROP_GRAB_FOLLOWS_CONTROLLER, bool, setGrabFollowsController);
    READ_ENTITY_PROPERTY(PROP_GRAB_TRIGGERABLE, bool, setTriggerable);
    READ_ENTITY_PROPERTY(PROP_GRAB_EQUIPPABLE, bool, setEquippable);
    READ_ENTITY_PROPERTY(PROP_GRAB_LEFT_EQUIPPABLE_POSITION_OFFSET, glm::vec3, setEquippableLeftPosition);
    READ_ENTITY_PROPERTY(PROP_GRAB_LEFT_EQUIPPABLE_ROTATION_OFFSET, glm::quat, setEquippableLeftRotation);
    READ_ENTITY_PROPERTY(PROP_GRAB_RIGHT_EQUIPPABLE_POSITION_OFFSET, glm::vec3, setEquippableRightPosition);
    READ_ENTITY_PROPERTY(PROP_GRAB_RIGHT_EQUIPPABLE_ROTATION_OFFSET, glm::quat, setEquippableRightRotation);

    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_GRAB_GRABBABLE, Grabbable);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_GRAB_KINEMATIC, GrabKinematic);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_GRAB_FOLLOWS_CONTROLLER, GrabFollowsController);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_GRAB_TRIGGERABLE, Triggerable);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_GRAB_EQUIPPABLE, Equippable);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_GRAB_LEFT_EQUIPPABLE_POSITION_OFFSET, EquippableLeftPosition);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_GRAB_LEFT_EQUIPPABLE_ROTATION_OFFSET, EquippableLeftRotation);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_GRAB_RIGHT_EQUIPPABLE_POSITION_OFFSET, EquippableRightPosition);
    DECODE_GROUP_PROPERTY_HAS_CHANGED(PROP_GRAB_RIGHT_EQUIPPABLE_ROTATION_OFFSET, EquippableRightRotation);

    processedBytes += bytesRead;

    Q_UNUSED(somethingChanged);

    return true;
}

void GrabPropertyGroup::markAllChanged() {
    _grabbableChanged = true;
    _grabKinematicChanged = true;
    _grabFollowsControllerChanged = true;
    _triggerableChanged = true;
    _equippableChanged = true;
    _equippableLeftPositionChanged = true;
    _equippableLeftRotationChanged = true;
    _equippableRightPositionChanged = true;
    _equippableRightRotationChanged = true;
}

EntityPropertyFlags GrabPropertyGroup::getChangedProperties() const {
    EntityPropertyFlags changedProperties;

    CHECK_PROPERTY_CHANGE(PROP_GRAB_GRABBABLE, grabbable);
    CHECK_PROPERTY_CHANGE(PROP_GRAB_KINEMATIC, grabKinematic);
    CHECK_PROPERTY_CHANGE(PROP_GRAB_FOLLOWS_CONTROLLER, grabFollowsController);
    CHECK_PROPERTY_CHANGE(PROP_GRAB_TRIGGERABLE, triggerable);
    CHECK_PROPERTY_CHANGE(PROP_GRAB_EQUIPPABLE, equippable);
    CHECK_PROPERTY_CHANGE(PROP_GRAB_LEFT_EQUIPPABLE_POSITION_OFFSET, equippableLeftPosition);
    CHECK_PROPERTY_CHANGE(PROP_GRAB_LEFT_EQUIPPABLE_ROTATION_OFFSET, equippableLeftRotation);
    CHECK_PROPERTY_CHANGE(PROP_GRAB_RIGHT_EQUIPPABLE_POSITION_OFFSET, equippableRightPosition);
    CHECK_PROPERTY_CHANGE(PROP_GRAB_RIGHT_EQUIPPABLE_ROTATION_OFFSET, equippableRightRotation);

    return changedProperties;
}

void GrabPropertyGroup::getProperties(EntityItemProperties& properties) const {
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Grab, Grabbable, getGrabbable);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Grab, GrabKinematic, getGrabKinematic);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Grab, GrabFollowsController, getGrabFollowsController);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Grab, Triggerable, getTriggerable);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Grab, Equippable, getEquippable);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Grab, EquippableLeftPosition, getEquippableLeftPosition);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Grab, EquippableLeftRotation, getEquippableLeftRotation);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Grab, EquippableRightPosition, getEquippableRightPosition);
    COPY_ENTITY_GROUP_PROPERTY_TO_PROPERTIES(Grab, EquippableRightRotation, getEquippableRightRotation);
}

bool GrabPropertyGroup::setProperties(const EntityItemProperties& properties) {
    bool somethingChanged = false;

    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Grab, Grabbable, grabbable, setGrabbable);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Grab, GrabKinematic, grabKinematic, setGrabKinematic);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Grab, GrabFollowsController, grabFollowsController, setGrabFollowsController);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Grab, Triggerable, triggerable, setTriggerable);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Grab, Equippable, equippable, setEquippable);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Grab, EquippableLeftPosition, equippableLeftPosition, setEquippableLeftPosition);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Grab, EquippableLeftRotation, equippableLeftRotation, setEquippableLeftRotation);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Grab, EquippableRightPosition, equippableRightPosition,
                                              setEquippableRightPosition);
    SET_ENTITY_GROUP_PROPERTY_FROM_PROPERTIES(Grab, EquippableRightRotation, equippableRightRotation,
                                              setEquippableRightRotation);

    return somethingChanged;
}

EntityPropertyFlags GrabPropertyGroup::getEntityProperties(EncodeBitstreamParams& params) const {
    EntityPropertyFlags requestedProperties;

    requestedProperties += PROP_GRAB_GRABBABLE;
    requestedProperties += PROP_GRAB_KINEMATIC;
    requestedProperties += PROP_GRAB_FOLLOWS_CONTROLLER;
    requestedProperties += PROP_GRAB_TRIGGERABLE;
    requestedProperties += PROP_GRAB_EQUIPPABLE;
    requestedProperties += PROP_GRAB_LEFT_EQUIPPABLE_POSITION_OFFSET;
    requestedProperties += PROP_GRAB_LEFT_EQUIPPABLE_ROTATION_OFFSET;
    requestedProperties += PROP_GRAB_RIGHT_EQUIPPABLE_POSITION_OFFSET;
    requestedProperties += PROP_GRAB_RIGHT_EQUIPPABLE_ROTATION_OFFSET;

    return requestedProperties;
}

void GrabPropertyGroup::appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                           EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                           EntityPropertyFlags& requestedProperties,
                                           EntityPropertyFlags& propertyFlags,
                                           EntityPropertyFlags& propertiesDidntFit,
                                           int& propertyCount,
                                           OctreeElement::AppendState& appendState) const {

    bool successPropertyFits = true;

    APPEND_ENTITY_PROPERTY(PROP_GRAB_GRABBABLE, getGrabbable());
    APPEND_ENTITY_PROPERTY(PROP_GRAB_KINEMATIC, getGrabKinematic());
    APPEND_ENTITY_PROPERTY(PROP_GRAB_FOLLOWS_CONTROLLER, getGrabFollowsController());
    APPEND_ENTITY_PROPERTY(PROP_GRAB_TRIGGERABLE, getTriggerable());
    APPEND_ENTITY_PROPERTY(PROP_GRAB_EQUIPPABLE, getEquippable());
    APPEND_ENTITY_PROPERTY(PROP_GRAB_LEFT_EQUIPPABLE_POSITION_OFFSET, getEquippableLeftPosition());
    APPEND_ENTITY_PROPERTY(PROP_GRAB_LEFT_EQUIPPABLE_ROTATION_OFFSET, getEquippableLeftRotation());
    APPEND_ENTITY_PROPERTY(PROP_GRAB_RIGHT_EQUIPPABLE_POSITION_OFFSET, getEquippableRightPosition());
    APPEND_ENTITY_PROPERTY(PROP_GRAB_RIGHT_EQUIPPABLE_ROTATION_OFFSET, getEquippableRightRotation());
}

int GrabPropertyGroup::readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                        ReadBitstreamToTreeParams& args,
                                                        EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                        bool& somethingChanged) {

    int bytesRead = 0;
    const unsigned char* dataAt = data;

    READ_ENTITY_PROPERTY(PROP_GRAB_GRABBABLE, bool, setGrabbable);
    READ_ENTITY_PROPERTY(PROP_GRAB_KINEMATIC, bool, setGrabKinematic);
    READ_ENTITY_PROPERTY(PROP_GRAB_FOLLOWS_CONTROLLER, bool, setGrabFollowsController);
    READ_ENTITY_PROPERTY(PROP_GRAB_TRIGGERABLE, bool, setTriggerable);
    READ_ENTITY_PROPERTY(PROP_GRAB_EQUIPPABLE, bool, setEquippable);
    READ_ENTITY_PROPERTY(PROP_GRAB_LEFT_EQUIPPABLE_POSITION_OFFSET, glm::vec3, setEquippableLeftPosition);
    READ_ENTITY_PROPERTY(PROP_GRAB_LEFT_EQUIPPABLE_ROTATION_OFFSET, glm::quat, setEquippableLeftRotation);
    READ_ENTITY_PROPERTY(PROP_GRAB_RIGHT_EQUIPPABLE_POSITION_OFFSET, glm::vec3, setEquippableRightPosition);
    READ_ENTITY_PROPERTY(PROP_GRAB_RIGHT_EQUIPPABLE_ROTATION_OFFSET, glm::quat, setEquippableRightRotation);

    return bytesRead;
}
