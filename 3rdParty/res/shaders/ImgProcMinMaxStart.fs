uniform sampler2D inputTex;
varying vec2 uv;

void main()
{
    vec4 color = texture2D(inputTex, uv);
    if (color.a > 0.0)
    {
        gl_FragColor = uv.xyxy;
    }
    else
    {
        gl_FragColor = vec4(1.0, 1.0, 0.0, 0.0);
    }
}