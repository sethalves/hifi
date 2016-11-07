varying vec3 viewNormal;
varying vec3 viewPos;
varying vec2 uv;            // texture coordinates    
varying vec4 shadowCoord;   // for shadow rendering
varying float fDepth;       // for depth only rendering

// surface material properties
uniform vec4 diffuseColor;      // diffuse RGB + opacity
uniform bool useDiffuseTex;
uniform sampler2D diffuseTex;   // diffuse RGB when useDiffuseTex is true
uniform vec4 specularColor;     // specular RGB + specular power

// directional light properties
uniform vec3 lightDir;              // direction of light in view space
uniform vec3 lightAmbientColor;
uniform vec3 lightDirectColor;

uniform bool useSecondaryLight;
uniform vec3 light2Dir;
uniform vec3 light2AmbientColor;
uniform vec3 light2DirectColor;

uniform bool useShadowMap;
uniform sampler2D shadowMap;

// environment maps
uniform ivec2 useEnvMaps;
uniform sampler2D diffuseEnvMap;            // a diffuse environment map (irradiance sphere map)
uniform sampler2D specularEnvMap;           // a specular environment map (reflection sphere map)

// for visualize picked/selected state or other stuff
uniform vec4 postColor;     // final color rgb will be lerp(finalColor.rgb, postColor.rgb, postColor.w);

uniform bool useDiffTextureAndDiffColorOnly;

uniform bool useBloomEffect;

uniform bool useCustomMaxTexCoord;
uniform vec2 customTextureCoord;

//#define USE_GAMMA_CORRECTION

vec3 gammaCorrection(vec3 color)
{
#ifdef USE_GAMMA_CORRECTION
    return sqrt(color);
#else
    return color;
#endif
}

vec3 inverseGammaCorrection(vec3 color)
{
#ifdef USE_GAMMA_CORRECTION
    return color * color;
#else
    return color;
#endif
}


float clampedDot(vec3 a, vec3 b)
{
    return max(dot(a, b), 0.0);
}


vec3 fresnel(vec3 specColor, float angle)
{
    // Schlick's approximation
    vec3 ret = specColor + (vec3(1.0, 1.0, 1.0) - specColor) * pow(1.0 - angle, 5.0);
    return ret;
}


vec3 calculateReflectance(
    vec3 N, vec3 V, 
    vec3 L, vec3 lightAmbient, vec3 lightDirect,
    vec3 diffColor, vec3 specColor, float shininess)
{
	//L = normalize(L);
    float lambertTerm = clampedDot(N, L);

    // calculate half vector
    vec3 H = normalize(L + V);
    
    // normal distribution term multiplied with implicit Geometry term (with normalization factor)
    float D = pow(clampedDot(N, H), shininess) * (shininess + 2.0) / 8.0;
    
    vec3 specularTerm = D * fresnel(specColor, dot(L, H));
    vec3 refl = (diffColor + specularTerm) * lambertTerm * lightDirect;
    refl += diffColor * lightAmbient;
    
    return refl;
}


vec2 encodeNormal(vec3 n)
{
    float p = sqrt(n.z * 8.0 + 8.0);
    return vec2(n.xy / p + vec2(0.5, 0.5));
}


void main()
{
#ifdef DEPTH_ONLY_RENDER

    vec2 encodedNormal;
#ifdef WRITE_NORMALS_WITH_DEPTH
    encodedNormal = encodeNormal(normalize(viewNormal));
#endif
    gl_FragColor = vec4(packNUFtoNUI16(fDepth), encodedNormal);
    
#else
    vec4 diffCol;
    if (useDiffuseTex)
    {
		if(useCustomMaxTexCoord)
		{
			if(uv.x>customTextureCoord.x || uv.y> customTextureCoord.y)
			{
				discard;
				return;
			}
		}
		diffCol = texture2D(diffuseTex, uv);
		if(useDiffTextureAndDiffColorOnly)
		{
		diffCol.xyz =diffuseColor.xyz* inverseGammaCorrection(diffCol.xyz);
		// alpha cut
			if (diffCol.w < 0.5)
			{
				discard;
				return;
			}
		gl_FragColor = diffCol;
		return;
		}
		else
		{
        diffCol.xyz = inverseGammaCorrection(diffCol.xyz);
		}
    }
    else
    {
        diffCol = diffuseColor;
    }
	
	// alpha cut
	if (diffCol.w < 0.5)
	{
		discard;
		return;
	}
	
	vec3 N = normalize(viewNormal);
    vec3 V = normalize(viewPos);
    
    vec3 color = vec3(0.0, 0.0, 0.0);
	
    if (useEnvMaps.x != 0)
    {
        vec2 envMapUV = N.xy * 0.5 + vec2(0.5, 0.5);
        vec3 diffLC = texture2D(diffuseEnvMap, envMapUV).xyz;
        diffLC = inverseGammaCorrection(diffLC);
        color += diffLC * diffCol.xyz;
    }
        
    if (useEnvMaps.y != 0)
    {
        vec3 R = normalize(reflect(viewPos, N));
        R = normalize((R + vec3(0.0, 0.0, 1.0)) / 2.0);
        vec2 envMapUV = R.xy * 0.5 + vec2(0.5, 0.5);
        vec3 specLC = texture2D(specularEnvMap, envMapUV).xyz;
        specLC = inverseGammaCorrection(specLC);
        color += specLC * fresnel(specularColor.xyz, dot(V, N));
    }
    
    vec3 ld = lightDirectColor;
    if (useShadowMap)
    {
        ld *= shadow(shadowCoord, shadowMap, N, vec3(0.0, 1.0, 0.0), 1.0);
    }
    
    color += calculateReflectance(
        N, V,
        lightDir, lightAmbientColor, ld,
        diffCol.xyz, specularColor.rgb, specularColor.w);
		
		if(useSecondaryLight)
		{
			color += calculateReflectance(
				N, V,
				light2Dir, light2AmbientColor, light2DirectColor,
				diffCol.xyz, specularColor.rgb, specularColor.w);
		}
    //color = gammaCorrection(color);
    
    color = mix(color, postColor.rgb, postColor.w);
	if(useBloomEffect)
	{
		 float brightness = dot(color.rgb, vec3(0.3126, 0.7152, 0.0722));
		if(brightness > 1.0)
		{
			gl_FragColor = vec4(color.rgb, 1.0);
			return;
		}
		else
		{
			gl_FragColor = vec4(1.0,1.0,1.0, 0.0);
			return;
		}
		
	}
    gl_FragColor = vec4(color, diffCol.w);
    
#endif
}