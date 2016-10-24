struct DirectionalLight
{
    vec4 dir;
    vec4 ambientColor;
    vec4 directColor;
};

struct PointLight
{
    vec4 pos;
    vec4 ambientColor;
    vec4 directColor;
};

varying vec3 modelPos;                  // position in modelSpace
varying vec3 viewPos;                   // position in viewSpace
varying vec3 viewNormal;                // normal in viewSpace
varying vec2 uv;

// directional light
uniform DirectionalLight dirLight;

// point light
uniform PointLight pointLight;

// material
uniform vec4 diffuseColor;              // diffuse RGB + opacity
uniform vec4 specularColor;             // specular RGB + power
uniform bool useDiffuseTex;
uniform sampler2D diffuseTex;
uniform bool useSpecPowerTex;
uniform sampler2D specPowerTex;


// gamma correction data
uniform vec3 gamma;
uniform vec3 inverseGamma;


float clampedDot(vec3 a, vec3 b)
{
    return max(dot(a, b), 0.0);
}


vec3 fresnel(vec3 specColor, float angle)
{
    // Schlick's approximation
    return specColor + (1.0 - specColor) * pow(1.0 - angle, 5.0);
}


vec3 calculateReflectance(
    vec3 N, vec3 V, 
    vec3 L, vec3 lightDirect, vec3 lightAmbient, 
    vec3 diffColor, vec3 specColor, float specPower)
{
    float lambertTerm = clampedDot(N, L);

    // calculate half vector
    vec3 H = normalize(L + V);
    
    // normal distribution term (normalized blinn-phong specular term)
    float D = pow(clampedDot(N, H), specPower) * (specPower + 2.0) / 8.0;
    
    vec3 specularTerm = D * fresnel(specColor, dot(L, H));
    
    vec3 refl = (diffColor + specularTerm) * lambertTerm * lightDirect;
    refl += diffColor * lightAmbient;
    
    return refl;
}


void main()
{
    vec3 N = normalize(viewNormal);
    vec3 V = normalize(-viewPos);
    
    // acquire surface diffuse color
    vec3 diffColor;
    if (useDiffuseTex)
    {
        diffColor = texture2D(diffuseTex, uv).rgb;
#ifndef USE_SRGB_TEXTURES
        diffColor = pow(diffColor, gamma);
#endif
    }
    else
    {
        diffColor = diffuseColor.rgb;
    }
    
    // acquire surface specular power
    float specPower;
    if (useSpecPowerTex)
    {
        specPower = texture2D(specPowerTex, uv).a;
    }
    else
    {
        specPower = specularColor.a;
    }
    
    // iterate over light sources and sum intensities produced by light-surface interactions
    vec3 color = vec3(0.0, 0.0, 0.0);
    
    color += calculateReflectance(
        N, V,
        dirLight.dir.xyz, dirLight.directColor.rgb, dirLight.ambientColor.rgb,
        diffColor, specularColor.rgb, specPower);
    
    vec3 L = pointLight.pos.xyz - viewPos;
    float len = length(L);
    float atten = 1.0 / (0.5 * len + 0.2 * len * len + 0.0001);
    L /= len;
    
    color += calculateReflectance(
        N, V,
        L, pointLight.directColor.rgb * atten, pointLight.ambientColor.rgb * atten,
        diffColor, specularColor.rgb, specPower);

#ifndef USE_SRGB_FRAMEBUFFER
    // final gamma correction
    color = pow(color, inverseGamma);
#endif
        
	gl_FragColor = vec4(color, diffuseColor.a);
}
