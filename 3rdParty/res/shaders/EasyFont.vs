attribute vec4 vPosition;
attribute vec4 vColor0;

uniform mat4 worldViewProjMat;

varying vec4 fColor0;

void main()
{
    fColor0 = vColor0;
    gl_Position = worldViewProjMat * vPosition;
}