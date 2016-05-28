//
//  ObjectMotionStateInterface.h
//  libraries/entities/src
//
//  Created by Seth Alves on 2016-5-14
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


#ifndef hifi_ObjectMotionStateInterface_h
#define hifi_ObjectMotionStateInterface_h

class ObjectMotionStateInterface {
 public:
    // include virtual destructor to force objects to be polymorphic so that dynamic_cast works
    virtual ~ObjectMotionStateInterface() { }
    virtual void maybeSwitchPhysicsEngines() { }
};

#endif // hifi_ObjectMotionStateInterface_h
