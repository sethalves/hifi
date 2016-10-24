varying vec2 fTexCoord0;
varying vec4 fColor0;

uniform sampler2D colorTex;
uniform bool useTex;
uniform float discardLimit;

void main()
{
    vec4 color = fColor0;
    if (useTex)
    {
        vec4 texColor = texture2D(colorTex, fTexCoord0);
        color *= texColor;
    }
    if(color.a < discardLimit || ( (discardLimit == 0.0) && (color.a == 0.0)))
    {
        discard;
    }
    else
    {
        gl_FragColor = color;
    }
}
