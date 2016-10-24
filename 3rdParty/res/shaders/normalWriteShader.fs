varying vec3 normal;

void main()
{
    /*if (fDepth < 0.0 || fDepth > 1.0)
    {
        discard;
    }
    else*/
    {
        gl_FragColor = vec4(normal, 1.0);
    }
}
