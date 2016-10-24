uniform mat4 worldViewProjMat;
uniform mat3 viewNormalMat;
attribute vec3 vPosition;
attribute vec3 vNormal;
varying vec3 normal;

void main()
{
    vec4 pos = worldViewProjMat * vec4(vPosition, 1.0);
	normal = normalize(viewNormalMat*vNormal)*0.5+0.5;
	gl_Position = pos;
}
