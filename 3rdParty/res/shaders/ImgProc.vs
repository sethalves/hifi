attribute vec4 vPosition;

varying vec2 uv;

void main()
{
    uv = 0.5 * (vPosition.xy + vec2(1.0, 1.0));
    gl_Position = vPosition;
}