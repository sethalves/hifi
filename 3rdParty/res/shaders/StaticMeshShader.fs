varying vec3 fNormal;

uniform vec4 color;


float clampedDot(vec3 a, vec3 b)
{
    return max(dot(a, b), 0.0);
}


void main()
{
    vec3 N = normalize(fNormal);
    vec3 L = vec3(0.0, 0.0, 1.0);
    
    float lambertTerm = clampedDot(N, L);
    
    gl_FragColor = vec4(lambertTerm * color.rgb, color.w);
}