attribute vec4 vPosition;       // position in modelSpace
attribute vec3 vNormal;         // normal in modelSpace
attribute vec4 vColor0;         // color of vertex + dispacemt scale in alpha channel
attribute vec2 vTexCoord0;

varying vec3 viewPos;           // position in viewSpace
varying vec3 viewNormal;        // normal in viewSpace
varying vec4 fColor;            // interpolated color
varying vec2 uv;

uniform mat4 worldViewMat;          // transforms from modelSpace to viewSpace
uniform mat3 worldViewNormalMat;    // transforms normal vectors from modelSpace to viewSpace
uniform mat4 worldViewProjMat;      // transforms from modelSpace to NDC space
uniform vec3 displaceNormal;        // direction and scale of displacement

uniform bool useVertexColors;
uniform bool useDiffuseTex;

uniform mat4 uBiasMat;
uniform mat4 lightWorldViewProjMat;
varying vec4 shadowCoord;

void main()
{
    vec3 modelPos = vPosition.xyz;
    
    if (useVertexColors)
    {
        modelPos += displaceNormal * vColor0.a;
        fColor = vColor0;
    }
    
    uv = vTexCoord0;
    
    viewPos = (worldViewMat * vPosition).xyz;
    viewNormal = worldViewNormalMat * vNormal;
    
    shadowCoord = uBiasMat * lightWorldViewProjMat * vec4(modelPos, 1.0);
    gl_Position = worldViewProjMat * vec4(modelPos, 1.0);
}
