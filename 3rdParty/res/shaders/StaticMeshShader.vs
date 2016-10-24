attribute vec4 vPosition;
attribute vec3 vNormal;

varying vec3 fNormal;

uniform mat4 worldViewProjMat;
uniform mat3 worldViewNormalMat;

void main()
{
    fNormal = worldViewNormalMat * vNormal;
    gl_Position = worldViewProjMat * vPosition;
}