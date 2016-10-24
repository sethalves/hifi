attribute vec4 vPosition;
attribute vec2 vTexCoord0;

uniform mat4 worldViewProjMat;

varying vec2 fTexCoord0;


void main()
{
    gl_Position = worldViewProjMat * vPosition;
    fTexCoord0 = vTexCoord0;
}
