attribute vec2 vPosition;       // the destination uv
attribute vec2 vTexCoord0;      // the source uv

varying vec2 fTexCoord0;        // the source uv

void main()
{
    fTexCoord0 = vTexCoord0;
    gl_Position.xy = vPosition * 2.0 - vec2(1.0, 1.0);
    gl_Position.zw = vec2(0.0, 1.0);
}
