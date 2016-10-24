varying vec4 fColor0;

uniform bool useTexture;
uniform sampler2D uTexture;
varying vec2 fTexCoord0;

void main()
{
	vec4 color;
	if(useTexture)
    {
		color = texture2D(uTexture, fTexCoord0);
    }
	else
    {
		color = fColor0;
    }
    gl_FragColor = color;
}
