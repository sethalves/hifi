//
//  PhysicalSimulationTracker.h
//  interface/src/
//
//  Created by Seth Alves on 2016-5-14
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_PhysicalSimulationTracker_h
#define hifi_PhysicalSimulationTracker_h

#include <QUuid>
#include "SimulationTracker.h"

class PhysicalSimulationTracker : public SimulationTracker {
public:
    virtual EntitySimulationPointer newSimulation(QUuid key, EntityTreePointer tree) override;
    void setEntityEditPacketSender(EntityEditPacketSender* entityEditSender) { _entityEditSender = entityEditSender; }
private:
    EntityEditPacketSender* _entityEditSender;
};

#endif // hifi_PhysicalSimulationTracker_h
