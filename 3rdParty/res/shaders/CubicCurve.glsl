// ------------------------------------------------------------------------
//  Evaluators for cubic polynomial curves.
//  Multiple variants for float, vec2, vec3 and vec4 valued curves.
// ------------------------------------------------------------------------

// returns the cubic basis functions for a cubic polynomial curve evaluated at t with basis matrix B
vec4 getCubicBases(mat4 B, float t)
{
    vec4 T;
    T.x = 1.0;      // 1
    T.y = t;        // t
    T.z = t * t;    // t^2    
    T.w = T.z * t;  // t^3
    return B * T;
}

// evaluates a cubic polynomial curve at t with control points cp and basis matrix B
// multiple variants for float, vec2, vec3 and vec4 valued curves
float evaluateCubicCurve(float t, vec4 cp, mat4 B)
{
    vec4 bv = getCubicBases(B, t);
    return dot(cp, bv);
}

vec2 evaluateCubicCurve(float t, vec2 cp[4], mat4 B)
{
    vec4 bv = getCubicBases(B, t);
    vec2 res = bv[0] * cp[0] + 
        bv[1] * cp[1] + 
        bv[2] * cp[2] + 
        bv[3] * cp[3];
    return res;
}

vec3 evaluateCubicCurve(float t, vec3 cp[4], mat4 B)
{
    vec4 bv = getCubicBases(B, t);
    vec3 res = bv[0] * cp[0] + 
        bv[1] * cp[1] + 
        bv[2] * cp[2] + 
        bv[3] * cp[3];
    return res;
}

vec4 evaluateCubicCurve(float t, vec4 cp[4], mat4 B)
{
    vec4 bv = getCubicBases(B, t);
    vec4 res = bv[0] * cp[0] + 
        bv[1] * cp[1] + 
        bv[2] * cp[2] + 
        bv[3] * cp[3];
    return res;
}


const mat4 CubicBezierBaseMat = mat4(
    1.0, 0.0, 0.0, 0.0,
    -3.0, 3.0,  0.0, 0.0,
    3.0, -6.0, 3.0, 0.0,
    -1.0, 3.0, -3.0, 1.0);

// evaluates a cubic Bezier curve at t with control points cp
// multiple variants for float, vec2, vec3 and vec4 valued curves    
float evaluateCubicBezierCurve(float t, vec4 cp)
{
    return evaluateCubicCurve(t, cp, CubicBezierBaseMat);
}

vec2 evaluateCubicBezierCurve(float t, vec2 cp[4])
{
    return evaluateCubicCurve(t, cp, CubicBezierBaseMat);
}

vec3 evaluateCubicBezierCurve(float t, vec3 cp[4])
{
    return evaluateCubicCurve(t, cp, CubicBezierBaseMat);
}

vec4 evaluateCubicBezierCurve(float t, vec4 cp[4])
{
    return evaluateCubicCurve(t, cp, CubicBezierBaseMat);
}
// ------------------------------------------------------------------------
