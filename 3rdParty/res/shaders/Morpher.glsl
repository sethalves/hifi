#define USE_MORHPER

// ------------------------------------------------------------------------
//  blend shapes morphing
//  Morphs every input 3 most influental morph target vertex.
// ------------------------------------------------------------------------

// maximum number of blend shapes
const int MAX_MORPH_TARGETS = 32;

void blendShapesMorph(
    // weight of each blend shape
    float blendWeights[MAX_MORPH_TARGETS],

    // input vertex
    vec3 posIn, vec3 normalIn, 
    
    // 3 most influental morph targets (difference of target vertex and original vertex)
    vec3 targetPosDiff0, vec3 targetNormalDiff0, 
    vec3 targetPosDiff1, vec3 targetNormalDiff1, 
    vec3 targetPosDiff2, vec3 targetNormalDiff2, 
    
    // which weights to use for the 3 morph targets
    vec3 blendInds, 
    
    // output vertex (note that returned normal is not normalized)
    out vec3 posOut, out vec3 normalOut)
{
    posOut = posIn;
    normalOut = normalIn;
    
    for (int i = 0; i < MAX_MORPH_TARGETS; ++i)
    {
        if (int(blendInds.x) == i)
        {
            posOut += targetPosDiff0 * blendWeights[i];
            normalOut += targetNormalDiff0 * blendWeights[i];
        }
        else if (int(blendInds.y) == i)
        {
            posOut += targetPosDiff1 * blendWeights[i];
            normalOut += targetNormalDiff1 * blendWeights[i];
        }
        else if (int(blendInds.z) == i)
        {
            posOut += targetPosDiff2 * blendWeights[i];
            normalOut += targetNormalDiff2 * blendWeights[i];
        }
    }
}
