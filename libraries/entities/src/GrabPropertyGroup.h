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

#ifndef hifi_GrabPropertyGroup_h
#define hifi_GrabPropertyGroup_h

#include <stdint.h>

#include <glm/glm.hpp>

#include <QtScript/QScriptEngine>

#include "PropertyGroup.h"
#include "EntityItemPropertiesMacros.h"

class EntityItemProperties;
class EncodeBitstreamParams;
class OctreePacketData;
class ReadBitstreamToTreeParams;

static const bool INITIAL_GRABBABLE { true };
static const bool INITIAL_KINEMATIC { true };
static const bool INITIAL_FOLLOWS_CONTROLLER { true };
static const bool INITIAL_TRIGGERABLE { false };
static const bool INITIAL_EQUIPPABLE { false };
static const glm::vec3 INITIAL_LEFT_EQUIPPABLE_POSITION { glm::vec3(0.0f) };
static const glm::quat INITIAL_LEFT_EQUIPPABLE_ROTATION { glm::quat() };
static const glm::vec3 INITIAL_RIGHT_EQUIPPABLE_POSITION { glm::vec3(0.0f) };
static const glm::quat INITIAL_RIGHT_EQUIPPABLE_ROTATION { glm::quat() };


class GrabPropertyGroup : public PropertyGroup {
public:
    // EntityItemProperty related helpers
    virtual void copyToScriptValue(const EntityPropertyFlags& desiredProperties, QScriptValue& properties,
                                   QScriptEngine* engine, bool skipDefaults,
                                   EntityItemProperties& defaultEntityProperties) const override;
    virtual void copyFromScriptValue(const QScriptValue& object, bool& _defaultSettings) override;

    void merge(const GrabPropertyGroup& other);

    virtual void debugDump() const override;
    virtual void listChangedProperties(QList<QString>& out) override;

    virtual bool appendToEditPacket(OctreePacketData* packetData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const override;

    virtual bool decodeFromEditPacket(EntityPropertyFlags& propertyFlags,
                                      const unsigned char*& dataAt, int& processedBytes) override;
    virtual void markAllChanged() override;
    virtual EntityPropertyFlags getChangedProperties() const override;

    // EntityItem related helpers
    // methods for getting/setting all properties of an entity
    virtual void getProperties(EntityItemProperties& propertiesOut) const override;

    // returns true if something changed
    virtual bool setProperties(const EntityItemProperties& properties) override;

    virtual EntityPropertyFlags getEntityProperties(EncodeBitstreamParams& params) const override;

    virtual void appendSubclassData(OctreePacketData* packetData, EncodeBitstreamParams& params,
                                    EntityTreeElementExtraEncodeDataPointer entityTreeElementExtraEncodeData,
                                    EntityPropertyFlags& requestedProperties,
                                    EntityPropertyFlags& propertyFlags,
                                    EntityPropertyFlags& propertiesDidntFit,
                                    int& propertyCount,
                                    OctreeElement::AppendState& appendState) const override;

    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                ReadBitstreamToTreeParams& args,
                                                EntityPropertyFlags& propertyFlags, bool overwriteLocalData,
                                                bool& somethingChanged) override;

    // grab properties
    DEFINE_PROPERTY(PROP_GRAB_GRABBABLE, Grabbable, grabbable, bool, INITIAL_GRABBABLE);
    DEFINE_PROPERTY(PROP_GRAB_KINEMATIC, GrabKinematic, grabKinematic, bool, INITIAL_KINEMATIC);
    DEFINE_PROPERTY(PROP_GRAB_FOLLOWS_CONTROLLER, GrabFollowsController, grabFollowsController, bool,
                    INITIAL_FOLLOWS_CONTROLLER);
    DEFINE_PROPERTY(PROP_GRAB_TRIGGERABLE, Triggerable, triggerable, bool, INITIAL_TRIGGERABLE);
    DEFINE_PROPERTY(PROP_GRAB_EQUIPPABLE, Equippable, equippable, bool, INITIAL_EQUIPPABLE);
    DEFINE_PROPERTY_REF(PROP_GRAB_LEFT_EQUIPPABLE_POSITION_OFFSET, EquippableLeftPosition, equippableLeftPosition,
                        glm::vec3, INITIAL_LEFT_EQUIPPABLE_POSITION);
    DEFINE_PROPERTY_REF(PROP_GRAB_LEFT_EQUIPPABLE_ROTATION_OFFSET, EquippableLeftRotation, equippableLeftRotation,
                        glm::quat, INITIAL_LEFT_EQUIPPABLE_ROTATION);
    DEFINE_PROPERTY_REF(PROP_GRAB_RIGHT_EQUIPPABLE_POSITION_OFFSET, EquippableRightPosition, equippableRightPosition,
                        glm::vec3, INITIAL_RIGHT_EQUIPPABLE_POSITION);
    DEFINE_PROPERTY_REF(PROP_GRAB_RIGHT_EQUIPPABLE_ROTATION_OFFSET, EquippableRightRotation, equippableRightRotation,
                        glm::quat, INITIAL_RIGHT_EQUIPPABLE_ROTATION);
};

#endif // hifi_GrabPropertyGroup_h
