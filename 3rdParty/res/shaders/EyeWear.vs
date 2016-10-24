// Includes code for depth rendering and also for solid rendering with shadows.
// Macro defines are in the fragment shader, 
// compiler will optimize out the unnecessary parts for each program

// original vertex position and normal
attribute vec3 vPosition;
attribute vec3 vNormal;

// morph target 0 position and normal differences
attribute vec3 vTexCoord0;
attribute vec3 vTexCoord1;

// morph target 1 position and normal differences
attribute vec3 vTexCoord2;
attribute vec3 vTexCoord3;

// morph target 2 position and normal differences
attribute vec3 vTexCoord4;
attribute vec3 vTexCoord5;

// which blend weights to use for the 3 morph targets?
attribute vec3 vBlendInds;

uniform mat4 worldViewMat;
uniform mat4 worldViewProjMat;
uniform mat3 worldViewNormalMat;
uniform float blendWeights[MAX_MORPH_TARGETS];       // morph target influences

// for shadow rendering
uniform sampler2D shadowMap;
uniform mat4 lightMat;  // biasMat * lightWorldViewProjMat

varying vec3 viewNormal;
varying vec3 viewPos;
varying vec4 shadowCoord;   // for shadow rendering
varying float fDepth;       // for depth only rendering


void main()
{
    vec3 pos;
    vec3 normal;
    
    blendShapesMorph(
        blendWeights, 
        vPosition, vNormal, 
        vTexCoord0, vTexCoord1,
        vTexCoord2, vTexCoord3,
        vTexCoord4, vTexCoord5,
        vBlendInds,
        pos, normal);

    // TODO: is it necessary?
    normal = normalize(normal);
    
    vec4 hpos = vec4(pos, 1.0);
    viewPos = -(worldViewMat * hpos).xyz;
    viewNormal = worldViewNormalMat * normal;
    
    // for shadow rendering
    shadowCoord = lightMat * hpos;
    
    // for depth rendering
    vec4 clipPos = worldViewProjMat * hpos;
    fDepth = clipPos.z/clipPos.w;
	fDepth = fDepth * 0.5 + 0.5;
    
    gl_Position = clipPos;
}