varying vec2 pos;
uniform float alphaScale;

void main()
{
    float t = distance(vec2(0.0, 0.0), pos) / 0.9;
    t = abs(t - 1.0);
    t = 1.0 - smoothstep(0.0, 0.08, t);
    gl_FragColor = vec4(1.0, 1.0, 1.0, t * alphaScale);
}
