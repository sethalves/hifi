varying vec4 centerColor;
varying vec4 ledgeColor;
varying vec2 pos;

void main()
{
    gl_FragColor = mix(ledgeColor, centerColor, 1.0 - length(pos)) + 0.2;
}
