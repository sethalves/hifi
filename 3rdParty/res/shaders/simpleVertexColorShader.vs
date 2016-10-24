uniform mat4 worldViewProjMat;

attribute vec4 vPosition;
attribute vec4 vColor0;

varying vec4 fColor;

void main()
{
    fColor = vColor0;
    gl_Position = worldViewProjMat * vPosition;
}
