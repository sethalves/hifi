//
//  AssignmentActionOffset.h
//  assignment-client/src/dynamics/
//
//  Created by Seth Alves 2018-3-31
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_AssignmentActionOffset_h
#define hifi_AssignmentActionOffset_h

#include "AssignmentDynamic.h"

class AssignmentActionOffset : public AssignmentDynamic {
public:
    AssignmentActionOffset(const QUuid& id, EntityItemPointer ownerEntity);
    virtual ~AssignmentActionOffset();

    virtual bool isAction() const override { return true; }

    virtual bool updateArguments(QVariantMap arguments) override;
    virtual QVariantMap getArguments() override;

    virtual QByteArray serialize() const override;
    virtual void deserialize(QByteArray serializedArguments) override;

private:
    static const uint16_t offsetVersion;
    glm::vec3 _pointToOffsetFrom;
    float _linearDistance;
    float _linearTimeScale;
    bool _positionalTargetSet;
};

#endif // hifi_AssignmentActionOffset_h
