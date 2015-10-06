//
//  Transform.cpp
//  shared/src/gpu
//
//  Created by Sam Gateau on 11/4/2014.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

#include "Transform.h"


void Transform::evalRotationScale(Quat& rotation, Vec3& scale, const Mat3& rotationScaleMatrix) {
    const float ACCURACY_THREASHOLD = 0.00001f;

    // Following technique taken from:
    // http://callumhay.blogspot.com/2010/10/decomposing-affine-transforms.html
    // Extract the rotation component - this is done using polar decompostion, where
    // we successively average the matrix with its inverse transpose until there is
    // no/a very small difference between successive averages
    float norm;
    int count = 0;
    Mat3 rotationMat = rotationScaleMatrix;
    do {
        Mat3 currInvTranspose = glm::inverse(glm::transpose(rotationMat));
        
        Mat3 nextRotation = 0.5f * (rotationMat + currInvTranspose);

        norm = 0.0;
        for (int i = 0; i < 3; i++) {
            float n = static_cast<float>(
                         fabs(rotationMat[0][i] - nextRotation[0][i]) +
                         fabs(rotationMat[1][i] - nextRotation[1][i]) +
                         fabs(rotationMat[2][i] - nextRotation[2][i]));
            norm = (norm > n ? norm : n);
        }
        rotationMat = nextRotation;
    } while (count++ < 100 && norm > ACCURACY_THREASHOLD);


    // extract scale of the matrix as the length of each axis
    Mat3 scaleMat = glm::inverse(rotationMat) * rotationScaleMatrix;

    scale = glm::max(Vec3(ACCURACY_THREASHOLD), Vec3(scaleMat[0][0], scaleMat[1][1], scaleMat[2][2]));

    // Let's work on a local matrix containing rotation only
    Mat3 matRot(
        rotationScaleMatrix[0] / scale.x,
        rotationScaleMatrix[1] / scale.y,
        rotationScaleMatrix[2] / scale.z);

    // Beware!!! needs to detect for the case there is a negative scale
    // Based on the determinant sign we just can flip the scale sign of one component: we choose X axis
    float determinant = glm::determinant(matRot);
    if (determinant < 0.0f) {
        scale.x = -scale.x;
        matRot[0] *= -1.0f;
    }

    // Beware: even though the matRot is supposed to be normalized at that point,
    // glm::quat_cast doesn't always return a normalized quaternion...
   // rotation = glm::normalize(glm::quat_cast(matRot));
    rotation = (glm::quat_cast(matRot));
}
