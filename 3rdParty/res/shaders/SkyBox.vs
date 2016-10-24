attribute vec3 vPosition;
uniform mat4 worldViewProjMat;

varying vec3 TexCoord0;

void main()
{
    vec4 pos = worldViewProjMat * vec4(vPosition, 1.0);
    gl_Position = pos.xyww;
    TexCoord0 = vPosition;
}