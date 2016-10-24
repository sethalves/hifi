attribute vec2 vPosition;
varying vec2 fTexCoord0;

void main()
{
    fTexCoord0 = vPosition;
    gl_Position.xy = vPosition * 2.0 - vec2(1.0, 1.0);
    gl_Position.zw = vec2(0.99, 1.0);
}
