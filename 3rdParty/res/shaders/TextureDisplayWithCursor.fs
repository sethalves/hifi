uniform vec3 cursorPos;         // u, v and radius in texture space
uniform vec3 cursorColor;
uniform sampler2D colorTex;

varying vec2 fTexCoord0;

void main()
{
    float cursor = calculateCursorStrength(vec3(cursorPos.xy, 0.0), cursorPos.z, vec3(fTexCoord0, 0.0));
    
    vec3 color = texture2D(colorTex, fTexCoord0).rgb;
    if (cursor > 0.0)
    {
        color = mix(color + cursorColor * cursor, cursorColor, cursor);
    }

    gl_FragColor = vec4(color, 1.0);
}
