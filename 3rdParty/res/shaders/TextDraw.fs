varying vec2 fTexCoord0;
varying vec4 fColor0;

uniform sampler2D fontTexture;

void main()
{
    vec4 texColor = texture2D(fontTexture, fTexCoord0);
    if(texColor.a==0.0)
    {
        discard;
    }
    else
    {
        gl_FragColor = fColor0;
        gl_FragColor.a = texColor.a*fColor0.a;
    }
}
