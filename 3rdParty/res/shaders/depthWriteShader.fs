varying float fDepth;
varying vec3 normal;

vec2 encode(vec3 n)
{
    float p = sqrt(n.z*8.0 + 8.0);
    return vec2(n.xy/p + 0.5);
}

void main()
{
    /*if (fDepth < 0.0 || fDepth > 1.0)
    {
        discard;
    }
    else*/
    {
        gl_FragColor = vec4(packNUFtoNUI16(fDepth), encode(normalize(normal)));
    }
}
