attribute vec3 vPosition;       // position in modelSpace

varying vec3 modelPos;

uniform mat4 worldViewMat;          // transforms from modelSpace to viewSpace
uniform mat3 worldViewNormalMat;    // transforms normal vectors from modelSpace to viewSpace
uniform mat4 worldViewProjMat;      // transforms from modelSpace to NDC space

uniform mat4 uBiasMat;
uniform mat4 lightWorldViewProjMat;
uniform mat4 lightWorldViewProjMat2;
varying vec4 shadowCoord;
varying vec4 shadowCoord2;


void main()
{
	shadowCoord = uBiasMat * lightWorldViewProjMat * vec4(vPosition, 1.0);
	shadowCoord2 = uBiasMat * lightWorldViewProjMat2 * vec4(vPosition, 1.0);
	
	modelPos = vPosition;
    gl_Position = worldViewProjMat * vec4(vPosition, 1.0);
}
