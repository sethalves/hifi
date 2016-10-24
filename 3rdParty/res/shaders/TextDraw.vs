attribute vec4 vPosition;
attribute vec2 vTexCoord0;
attribute vec4 vColor0;

varying vec2 fTexCoord0;
varying vec4 fColor0;

uniform mat4 worldViewProjMat;


void main()
{
    gl_Position = worldViewProjMat * vPosition;
    fTexCoord0 = vTexCoord0;
    fColor0 = vColor0;
}