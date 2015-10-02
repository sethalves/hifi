//
//  OverlaysPayload.cpp
//  interface/src/ui/overlays
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include <QScriptValueIterator>

#include <limits>
#include <typeinfo>

#include <Application.h>
#include <avatar/AvatarManager.h>
#include <LODManager.h>
#include <render/Scene.h>

#include "Image3DOverlay.h"
#include "Circle3DOverlay.h"
#include "Cube3DOverlay.h"
#include "ImageOverlay.h"
#include "Line3DOverlay.h"
#include "LocalModelsOverlay.h"
#include "ModelOverlay.h"
#include "Overlays.h"
#include "Rectangle3DOverlay.h"
#include "Sphere3DOverlay.h"
#include "Grid3DOverlay.h"
#include "TextOverlay.h"
#include "Text3DOverlay.h"


namespace render {
    template <> const ItemKey payloadGetKey(const Overlay::Pointer& overlay) {
        if (overlay->is3D() && !std::dynamic_pointer_cast<Base3DOverlay>(overlay)->getDrawOnHUD()) {
            if (std::dynamic_pointer_cast<Base3DOverlay>(overlay)->getDrawInFront()) {
                return ItemKey::Builder().withTypeShape().withLayered().build();
            } else {
                return ItemKey::Builder::opaqueShape();
            }
        } else {
            return ItemKey::Builder().withTypeShape().withViewSpace().build();
        }
    }
    template <> const Item::Bound payloadGetBound(const Overlay::Pointer& overlay) {
        return overlay->getBounds();
    }
    template <> int payloadGetLayer(const Overlay::Pointer& overlay) {
        // MAgic number while we are defining the layering mechanism:
        const int LAYER_2D =  2;
        const int LAYER_3D_FRONT = 1;
        const int LAYER_3D = 0;
        if (overlay->is3D()) {
            return (std::dynamic_pointer_cast<Base3DOverlay>(overlay)->getDrawInFront() ? LAYER_3D_FRONT : LAYER_3D);
        } else {
            return LAYER_2D;
        }
    }
    template <> void payloadRender(const Overlay::Pointer& overlay, RenderArgs* args) {
        if (args) {
            if (overlay->getAnchor() == Overlay::MY_AVATAR) {
                auto batch = args->_batch;
                MyAvatar* avatar = DependencyManager::get<AvatarManager>()->getMyAvatar();
                glm::quat myAvatarRotation = avatar->getAbsoluteOrientation();
                glm::vec3 myAvatarPosition = avatar->getAbsolutePosition();
                float angle = glm::degrees(glm::angle(myAvatarRotation));
                glm::vec3 axis = glm::axis(myAvatarRotation);
                float myAvatarScale = avatar->getScale();
                Transform transform = Transform();
                transform.setTranslation(myAvatarPosition);
                transform.setRotation(glm::angleAxis(angle, axis));
                transform.setScale(myAvatarScale);
                batch->setModelTransform(transform);
                overlay->render(args);
            } else {
                overlay->render(args);
            }
        }
    }
}
