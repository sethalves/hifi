// Includes code for depth rendering and also for solid rendering with shadows.
// Macro defines are in the fragment shader, 
// compiler will optimize out the unnecessary parts for each program

// original vertex position and normal
attribute vec3 vPosition;
attribute vec3 vNormal;

// vertex uv
attribute vec2 vTexCoord0;

#ifdef USE_MORHPER
// morph target 0 position and normal differences
attribute vec3 vTexCoord1;
attribute vec3 vTexCoord2;

// morph target 1 position and normal differences
attribute vec3 vTexCoord3;
attribute vec3 vTexCoord4;

// morph target 2 position and normal differences
attribute vec3 vTexCoord5;
attribute vec3 vTexCoord6;

// which blend weights to use for the 3 morph targets?
attribute vec3 vBlendInds;

uniform float blendWeights[MAX_MORPH_TARGETS];       // morph target influences
#endif

uniform mat4 worldViewMat;
uniform mat4 worldViewProjMat;
uniform mat3 worldViewNormalMat;

// for shadow rendering
uniform mat4 lightMat;  // biasMat * lightWorldViewProjMat

varying vec3 viewNormal;
varying vec3 viewPos;
varying vec2 uv;            // texture coordinates
varying vec4 shadowCoord;   // for shadow rendering
varying float fDepth;       // for depth only rendering


void main()
{
#ifdef USE_MORHPER
    vec3 pos;
    vec3 normal;

    blendShapesMorph(
        blendWeights, 
        vPosition, vNormal, 
        vTexCoord1, vTexCoord2,
        vTexCoord3, vTexCoord4,
        vTexCoord5, vTexCoord6,
        vBlendInds,
        pos, normal);
        
    // TODO: is it necessary?
    normal = normalize(normal);        
    vec4 hpos = vec4(pos, 1.0);
#else
    vec3 normal = vNormal;
    vec4 hpos = vec4(vPosition, 1.0);
#endif

    viewPos = -(worldViewMat * hpos).xyz;
    viewNormal = worldViewNormalMat * normal;
    uv = vTexCoord0;
    
    // for shadow rendering
    shadowCoord = lightMat * hpos;
    
    // for depth rendering
    vec4 clipPos = worldViewProjMat * hpos;
    fDepth = clipPos.z/clipPos.w;
	fDepth = fDepth * 0.5 + 0.5;
    
    gl_Position = clipPos;
}