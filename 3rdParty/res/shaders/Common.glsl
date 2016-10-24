const float EPSILON = 0.000001;

#define MAX_CURSORS 2

// ------------------------------------------------------------------------
//  Calculates cursor display strength with projective texturing.
// ------------------------------------------------------------------------
vec4 getPatternSample(sampler2D s, vec4 tc)
{   
    return texture2DProj(s, tc);
}

vec4 calculateCursorColor(sampler2D patternTex, vec4 cursorTexCoord, vec3 patternProjDir, vec3 normal)
{
    if (dot(patternProjDir, normal) <= 0.0)
    {
        return vec4(0.0, 0.0, 0.0, 0.0);
    }
    
    return getPatternSample(patternTex, cursorTexCoord);
}


vec4 calculateCursorColor(
    int numCursors,
    sampler2D patternTex,
    vec4 cursorTexCoords[MAX_CURSORS],
    vec3 patternProjDir[MAX_CURSORS],
    vec3 normal)
{
    if (numCursors <= 0)
    {
        return vec4(0.0, 0.0, 0.0, 0.0);
    }

    vec4 t = calculateCursorColor(patternTex, cursorTexCoords[0], patternProjDir[0], normal);
    
    for (int i = 1; i < MAX_CURSORS; ++i)
    {
        if (i == numCursors)
        {
            break;
        }
        vec4 ti = calculateCursorColor(patternTex, cursorTexCoords[i], patternProjDir[i], normal);
        t += (ti.xyz, 0.5 * ti.a);
    }
    
    return t;
}


// ------------------------------------------------------------------------
//  Calculates cursor display strength with position, radius and fragment 
//  position.
// ------------------------------------------------------------------------
float calculateCursorStrength(vec3 cursorPos, float cursorRadius, vec3 pos)
{
    float t = distance(cursorPos, pos) / cursorRadius;
    t = abs(t - 1.0);
    return 1.0 - smoothstep(0.0, 0.08, t);
}


float calculateCursorStrength(
    int numCursors, 
    vec3 cursorPositions[MAX_CURSORS], 
    float radius, 
    vec3 pos)
{
    if (numCursors <= 0)
    {
        return 0.0;
    }
    
    float t = calculateCursorStrength(cursorPositions[0], radius, pos);

    for (int i = 1; i < MAX_CURSORS; ++i)
    {
        if (i == numCursors)
        {
            break;
        }
        float ti = calculateCursorStrength(cursorPositions[i], radius, pos);
        t += 0.5 * ti;
    }
    
    return t;
}


// ------------------------------------------------------------------------
//  Calculates symmetry axis strength
// ------------------------------------------------------------------------
float calculateSimmetryAxisTerm(bvec3 showAxes, vec3 pos, float scale)
{
    float ret = 0.0;
    
    float sc = 0.003 * scale;
    
    if (showAxes.x)
    {
        float t = abs(pos.x);
        ret += 1.0 - smoothstep(0.0, sc, t);
    }
    if (showAxes.y)
    {
        float t = abs(pos.y);
        ret += 1.0 - smoothstep(0.0, sc, t);
    }
    if (showAxes.z)
    {
        float t = abs(pos.z);
        ret += 1.0 - smoothstep(0.0, sc, t);
    }
    
    return ret;
}
