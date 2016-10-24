attribute vec3 vPosition;       // position in modelSpace
attribute vec3 vNormal;         // normal in modelSpace
attribute vec2 vTexCoord0;      // texture coodrinates
attribute vec4 vColor0;         // deform weight in w component

varying vec3 modelPos;          // position in model space
varying vec3 viewPos;           // position in viewSpace
varying vec3 modelNormal;       // normal in modelSpace
varying vec2 texCoord0;         // texture coordinates
varying vec4 vertColor;

uniform mat4 worldViewMat;          // transforms from modelSpace to viewSpace
uniform mat3 worldViewNormalMat;    // transforms normal vectors from modelSpace to viewSpace
uniform mat4 worldViewProjMat;      // transforms from modelSpace to NDC space

uniform mat4 uBiasMat;
uniform mat4 lightWorldViewProjMat;
uniform mat4 lightWorldViewProjMat2;
varying vec4 shadowCoord;
varying vec4 shadowCoord2;

uniform bool useDeformers;

// this is because the HP Sprout Intel(R) HD Graphics 5500 driver on Window 10 does not support structs in the GLSL shader..
uniform float rwParams_weight;
uniform vec3 rwParams_axisP1;
uniform vec4 rwParams_axisDir;
uniform vec3 rwParams_waveParams;
uniform vec2 rwParams_awcp[4];

uniform float dsParams_weight;
uniform vec3 dsParams_center;
uniform vec4 dsParams_dir;


void main()
{
    if (useDeformers)
    {
        modelPos = deform(
            vec4(vPosition, vColor0.w),
            rwParams_weight, rwParams_axisP1, rwParams_axisDir, rwParams_waveParams, rwParams_awcp,
            dsParams_weight, dsParams_center, dsParams_dir);
            
        // TODO: deform modelNormal
        modelNormal = vNormal;
    }
    else
    {
        modelPos = vPosition;
        modelNormal = vNormal;
    }
	
	shadowCoord = uBiasMat * lightWorldViewProjMat * vec4(modelPos, 1.0);
	shadowCoord2 = uBiasMat * lightWorldViewProjMat2 * vec4(modelPos, 1.0);
    
    viewPos = (worldViewMat * vec4(modelPos, 1.0)).xyz;
    texCoord0 = vTexCoord0;
    vertColor = vColor0;
    gl_Position = worldViewProjMat * vec4(modelPos, 1.0);
}
