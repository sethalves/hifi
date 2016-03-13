//
//  ParentMissingOperator.h
//  libraries/entities/src
//
//  Created by Seth Alves on 2016-3-1.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_ParentMissingOperator_h
#define hifi_ParentMissingOperator_h

#include "Octree.h"

class ParentMissingOperator : public RecurseOctreeOperator {
public:
    ParentMissingOperator(QVector<QUuid> goneParentID) : _goneParentIDs(goneParentID) { }
    ~ParentMissingOperator();
    virtual bool preRecursion(OctreeElementPointer element) { return true; }
    virtual bool postRecursion(OctreeElementPointer element);
private:
    QVector<QUuid> _goneParentIDs;
    QSet<EntityItemID> _toDelete;
    EntityTreePointer _tree { nullptr };
};

#endif // hifi_ParentMissingOperator_h
