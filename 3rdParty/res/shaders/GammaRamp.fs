varying vec2 fTexCoord0;

uniform float inverseGamma;

void main()
{
    float x = 5.0 * fTexCoord0.x;
    x = floor(x);
    x /= 4.0;
    
    vec3 color;
    /*if (fTexCoord0.y < 0.25)
    {
        color = vec3(x, 0.0, 0.0);
    }
    else if (fTexCoord0.y < 0.5)
    {
        color = vec3(0.0, x, 0.0);
    }
    else if (fTexCoord0.y < 0.75)
    {
        color = vec3(0.0, 0.0, x);
    }
    else*/
    {
        color = vec3(x, x, x);
    }
    
    color = pow(color, vec3(inverseGamma, inverseGamma, inverseGamma));
    
    gl_FragColor = vec4(color, 1.0);
}