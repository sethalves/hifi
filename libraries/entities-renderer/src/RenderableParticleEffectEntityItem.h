//
//  RenderableParticleEffectEntityItem.h
//  interface/src/entities
//
//  Created by Jason Rickwald on 3/2/15.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#ifndef hifi_RenderableParticleEffectEntityItem_h
#define hifi_RenderableParticleEffectEntityItem_h

#include <ParticleEffectEntityItem.h>
#include <TextureCache.h>
#include "RenderableEntityItem.h"

class RenderableParticleEffectEntityItem : public ParticleEffectEntityItem  {
public:
    static EntityItemPointer factory(const EntityItemID& entityID, const EntityItemProperties& properties);
    RenderableParticleEffectEntityItem(const EntityItemID& entityItemID, const EntityItemProperties& properties);
    virtual void render(RenderArgs* args);

    SIMPLE_RENDERABLE();

protected:

    void updateQuads(RenderArgs* args, bool textured);

    int _cacheID;
    const int VERTS_PER_PARTICLE = 4;

    NetworkTexturePointer _texture;
};


#endif // hifi_RenderableParticleEffectEntityItem_h
