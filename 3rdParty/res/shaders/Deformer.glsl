// ------------------------------------------------------------------------
//  RadialWaveDeformer
// ------------------------------------------------------------------------
vec3 radialWaveDeformer(vec4 pos, float weight, vec3 axisP1, vec4 axisDir, vec3 waveParams, vec2 awcp[4])
{
    float t = dot(pos.xyz - axisP1, axisDir.xyz);
    
    float w = weight * pos.w;
    
    vec3 B = axisDir.xyz * t + axisP1.xyz;
    vec3 dir = pos.xyz - B;
    
    t /= axisDir.w;
    vec2 awp = evaluateCubicBezierCurve(t, awcp);
    float Ar = awp.y * waveParams.x;
    t = t * waveParams.y + waveParams.z;

    vec3 move = Ar * sin(t) * dir;
    return pos.xyz + w * move;
}
// ------------------------------------------------------------------------



// ------------------------------------------------------------------------
//  DirectionScaleDeformer
// ------------------------------------------------------------------------
vec3 directionScaleDeformer(vec4 pos, float weight, vec3 center, vec4 dir)
{
    float t = dot(pos.xyz - center.xyz, dir.xyz);
    float w = weight * pos.w;
    vec3 move = t * (dir.w - 1.0) * dir.xyz;
    return pos.xyz + w * move;
}
// ------------------------------------------------------------------------



// ------------------------------------------------------------------------
//  Deform
// ------------------------------------------------------------------------
vec3 deform(vec4 pos, 
            float weight, vec3 axisP1, vec4 axisDir, vec3 waveParams, vec2 awcp[4],
            float rwWeight, vec3 center, vec4 dir)
{
    vec3 pos2 = radialWaveDeformer(pos, weight, axisP1, axisDir, waveParams, awcp);
    return directionScaleDeformer(vec4(pos2, pos.w), rwWeight, center, dir);
}
// ------------------------------------------------------------------------
