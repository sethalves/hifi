uniform float colorDiff;

attribute vec4 vPosition;
attribute vec4 vColor0;

varying vec4 centerColor;
varying vec4 ledgeColor;
varying vec2 pos;

void main()
{
    pos = vPosition.xy;
    centerColor = vColor0 + colorDiff;
    ledgeColor = vColor0;
    gl_Position = vPosition;
    gl_Position.z = 0.99;
}
