struct DirectionalLight
{
    vec4 dir;
    vec4 ambientColor;
    vec4 directColor;
};

varying vec3 viewPos;                   // position in viewSpace
varying vec3 viewNormal;                // normal in viewSpace
varying vec4 fColor;                    // interpolated color, only RGB is used
varying vec2 uv;

// directional light
uniform DirectionalLight dirLight;

// material
uniform vec4 diffuseColor;              // diffuse RGB + opacity
uniform sampler2D diffuseTex;
uniform vec4 specularColor;             // specular RGB + power
uniform bool useVertexColors;
uniform bool useDiffuseTex;

// gamma correction data
uniform vec3 inverseGamma;

uniform sampler2D shadowMap;
varying vec4 shadowCoord;

uniform bool useAO;
uniform vec2 screenSize;
uniform sampler2D uOcclusionMap;

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
    
    // surface diffuse color
    vec3 diffColor;
    
    /*if (useVertexColors)
    {
        diffColor = fColor.rgb;
    }
    else*/
    {
        if (useDiffuseTex)
        {
            diffColor = texture2D(diffuseTex, uv).rgb;
        }
        else
        {
            diffColor = diffuseColor.rgb;
        }
    }
    
    if(useAO)
	{
        vec2 texCoord = gl_FragCoord.xy / screenSize;
		diffColor *= texture2D(uOcclusionMap, texCoord).rgb;
	}

    // acquire surface specular power
    float specPower = specularColor.a;
    
    // iterate over light sources and sum intensities produced by light-surface interactions
    vec3 color = vec3(0.0, 0.0, 0.0);
    
    vec3 ld = dirLight.directColor.rgb;
    ld *= shadow(shadowCoord, shadowMap, N, vec3(0.0, 1.0, 0.0), 1.0);
    
    color += calculateReflectance(
        N, V,
        dirLight.dir.xyz, ld, dirLight.ambientColor.rgb,
        diffColor, specularColor.rgb, specPower);
        
    color += calculateReflectance(
        N, V,
        vec3(0.0, 0.0, 1.0), vec3(0.2, 0.2, 0.2), vec3(0.0, 0.0, 0.0),
        diffColor, specularColor.rgb, specPower);
    
#ifndef USE_SRGB_FRAMEBUFFER
    // final gamma correction
    color = pow(color, inverseGamma);
#endif
        
	gl_FragColor = vec4(color, 1.0);
}
