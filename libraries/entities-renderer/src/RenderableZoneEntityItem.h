//
//  RenderableZoneEntityItem.h
//
//
//  Created by Clement on 4/22/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableZoneEntityItem_h
#define hifi_RenderableZoneEntityItem_h

#include <Model.h>
#include <ZoneEntityItem.h>

class NetworkGeometry;

class RenderableZoneEntityItem : public ZoneEntityItem  {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);

    RenderableZoneEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties) :
        ZoneEntityItem(entityItemID, properties),
        _model(nullptr),
        _needsInitialSimulation(true) {
    }

    virtual bool setProperties(const EntityItemProperties& properties);
    virtual int readEntitySubclassDataFromBuffer(const unsigned char* data, int bytesLeftToRead,
                                                 ReadBitstreamToTreeParams& args,
                                                 EntityPropertyFlags& propertyFlags, bool overwriteLocalData);

    virtual void render(RenderArgs* args);
    virtual bool contains(const glm::vec3& point) const;
    
    virtual bool addToScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges);
    virtual void removeFromScene(EntityItemPointer self, std::shared_ptr<render::Scene> scene, render::PendingChanges& pendingChanges);
    
private:
    Model* getModel();
    void initialSimulation();
    void updateGeometry();
    
    template<typename Lambda>
    void changeProperties(Lambda functor);
    
    Model* _model;
    bool _needsInitialSimulation;
    
    render::ItemID _myMetaItem;
};

#endif // hifi_RenderableZoneEntityItem_h
