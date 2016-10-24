#ifdef VSM

#extension GL_OES_standard_derivatives : enable
varying vec3 lightPos;

vec2 encode(vec3 n)
{
    float p = sqrt(n.z*8.0 + 8.0);
    return vec2(n.xy/p + 0.5);
}

void main()
{
    float depth = clamp(length(lightPos)/40.0, 0.0, 1.0);
    float dx = dFdx(depth);
    float dy = dFdy(depth);
    gl_FragColor = vec4(depth, pow(depth, 2.0) + 0.25*(dx*dx + dy*dy), 0.0, 1.0);
}

#endif //VSM