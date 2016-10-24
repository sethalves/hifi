attribute vec4 vPosition;
attribute vec2 vTexCoord0;

varying vec3 fModelPos;

void main()
{
    fModelPos = vPosition.xyz;
    vec2 clipTC = vTexCoord0 * 2.0 - vec2(1.0, 1.0);
    gl_Position = vec4(clipTC, 0.0, 1.0);
}
