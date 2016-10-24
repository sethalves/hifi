uniform sampler2D colorTex;
uniform vec2 texCoord;

void main()
{
    gl_FragColor = texture2D(colorTex, texCoord);
}
