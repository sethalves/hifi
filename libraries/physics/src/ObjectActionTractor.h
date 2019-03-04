//
//  ObjectActionTractor.h
//  libraries/physics/src
//
//  Created by Seth Alves 2017-5-8
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ObjectActionTractor_h
#define hifi_ObjectActionTractor_h

#include "ObjectAction.h"

class ObjectActionTractor : public ObjectAction {
public:
    ObjectActionTractor(const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~ObjectActionTractor();

    virtual bool updateArguments(QVariantMap arguments) override;
    virtual QVariantMap getArguments() override;

    virtual void updateActionWorker(float deltaTimeStep) override;

    virtual QByteArray serialize() const override;
    virtual void deserialize(QByteArray serializedArguments) override;

    virtual bool getTarget(float deltaTimeStep, glm::quat& rotation, glm::vec3& position,
                           glm::vec3& linearVelocity, glm::vec3& angularVelocity,
                           float& linearTimeScale, float& angularTimeScale);

protected:
    static const uint16_t tractorVersion;

    glm::vec3 _positionalTarget { glm::vec3(0.0f) };
    glm::vec3 _desiredPositionalTarget { glm::vec3(0.0f) };
    glm::vec3 _lastPositionTarget { glm::vec3(0.0f) };
    float _linearTimeScale { FLT_MAX };
    bool _positionalTargetSet { false };
    bool _havePositionTargetHistory { false };

    glm::quat _rotationalTarget { glm::quat() };
    glm::quat _desiredRotationalTarget { glm::quat() };
    float _angularTimeScale { FLT_MAX };
    bool _rotationalTargetSet { true };

    glm::vec3 _linearVelocityTarget { glm::vec3(0.0f) };
    glm::vec3 _angularVelocityTarget { glm::vec3(0.0f) };

    float _minimumActiveDistance { 0.0f };
    float _maximumActiveDistance { FLT_MAX };

    virtual bool prepareForTractorUpdate(btScalar deltaTimeStep);

    void serializeParameters(QDataStream& dataStream) const;
    void deserializeParameters(QByteArray serializedArguments, QDataStream& dataStream);
};

#endif // hifi_ObjectActionTractor_h
