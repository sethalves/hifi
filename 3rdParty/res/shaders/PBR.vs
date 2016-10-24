attribute vec4 vPosition;       // position in modelSpace
attribute vec3 vNormal;         // normal in modelSpace
attribute vec2 vTexCoord0;

varying vec3 modelPos;          // position in model space
varying vec3 viewPos;           // position in viewSpace
varying vec3 viewNormal;        // normal in viewSpace
varying vec2 uv;

uniform mat4 worldViewMat;          // transforms from modelSpace to viewSpace
uniform mat3 worldViewNormalMat;    // transforms normal vectors from modelSpace to viewSpace
uniform mat4 worldViewProjMat;      // transforms from modelSpace to NDC space


void main()
{
    modelPos = vPosition.xyz;
    viewPos = (worldViewMat * vPosition).xyz;
    viewNormal = worldViewNormalMat * vNormal;
    uv = vTexCoord0;
    gl_Position = worldViewProjMat * vPosition;
}
