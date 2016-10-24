varying vec3 modelPos;                  // position in modelSpace
varying vec3 viewPos;                   // position in viewSpace
varying vec3 modelNormal;               // normal in modelSpace
varying vec2 texCoord0;                 // texture coordinates
varying vec4 vertColor;

uniform int numShadows;
uniform bool useAO;
uniform sampler2D uOcclusionMap;
uniform sampler2D shadowMap;
uniform sampler2D shadowMap2;
varying vec4 shadowCoord;
varying vec4 shadowCoord2;

uniform mat4 viewMat;

// for masking
uniform bool useVertColor;

// material
uniform vec4 diffuseColor;
uniform bool useDiffuseTex;
uniform sampler2D diffuseTex;
uniform vec3 specularColor;
uniform vec2 shininessRange;

// triplanar textures
uniform bool useTriplanarTextures;
uniform sampler2D planarTexX;
uniform sampler2D planarTexY;
uniform sampler2D planarTexZ;
uniform vec3 planarTexScale;
uniform vec3 planarTexStrength;
uniform bool useTriplanarBumpTextures;
uniform sampler2D planarBumpTexX;
uniform sampler2D planarBumpTexY;
uniform sampler2D planarBumpTexZ;
uniform vec3 planarBumpTexScale;
uniform vec3 planarBumpTexStrength;
uniform mat3 worldViewNormalMat;    // transforms normal vectors from modelSpace to viewSpace

// directional lights
const int MAX_LIGHTS = 3;
uniform int numLights;
uniform vec3 lightAmbientColor[MAX_LIGHTS];
uniform vec3 lightDirectColor[MAX_LIGHTS];
uniform vec3 lightDir[MAX_LIGHTS];

// reflection maps
uniform bool useEnvMaps;
uniform sampler2D diffuseEnvMap;            // a diffuse environment map (irradiance sphere map)
uniform sampler2D specularEnvMap;           // a specular environment map (reflection sphere map)

// cursor
uniform int cursorCnt;
uniform vec3 cursorPos[MAX_CURSORS];        // cursor position in model space
uniform float cursorRadius;                 // cursor radius in model space
uniform vec3 cursorColor;
uniform bool usePatternCursor;
uniform sampler2D patternTex;               // pattern projection texture
uniform vec3 patternViewDir[MAX_CURSORS];   // pattern projection (reverse) direction in viewSpace
uniform mat4 patternMat[MAX_CURSORS];       // transforms from modelSpace to projective texture space

// symmetry axes
uniform bvec3 showSymmetryAxes;
uniform float vmfScale;

// other
uniform vec2 screenSize;


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
	L = normalize(L);
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


vec3 getTriplanarBlendWeights(in vec3 P, in vec3 N, out vec2 uv[3])
{
    vec3 weights = abs(N);
    
    // tighten up the blending zone
    weights = (weights - vec3(0.2, 0.2, 0.2)) * 7.0;
    weights = max(weights, vec3(0.0, 0.0, 0.0));
    
    // force weights to sum to 1.0 (very important)
    weights /= (weights.x + weights.y + weights.z);
    
    // compute the uv coords for each of the 3 planar projections
    uv[0] = P.yz;
    uv[1] = P.xz;
    uv[2] = P.xy;
    
    return weights;
}


void main()
{
    vec3 V = normalize(-viewPos);

    vec4 diffuse;
    if (useDiffuseTex)
    {
        diffuse = texture2D(diffuseTex, texCoord0);
    }
    else
    {
        diffuse = diffuseColor;
    }
	
	if(useAO)
	{
        vec2 texCoord = gl_FragCoord.xy / screenSize;
		diffuse *= vec4(texture2D(uOcclusionMap, texCoord).rgb, 1.0);
	}
    
    // add triplanar textures
    vec3 N = modelNormal;
    float shininess = shininessRange.x;
    if (useTriplanarTextures || useTriplanarBumpTextures)
    {
        vec2 uvs[3];
        vec3 weights = getTriplanarBlendWeights(modelPos, N, uvs);

        if (useTriplanarTextures)
        {
            vec4 pc1 = texture2D(planarTexX, uvs[0] * planarTexScale.x);
            pc1.xyz = inverseGammaCorrection(pc1.xyz);
    
            vec4 pc2 = texture2D(planarTexY, uvs[1] * planarTexScale.y);
            pc2.xyz = inverseGammaCorrection(pc2.xyz);
    
            vec4 pc3 = texture2D(planarTexZ, uvs[2] * planarTexScale.z);
            pc3.xyz = inverseGammaCorrection(pc3.xyz);
    
            vec4 tpColor = weights.x * pc1 + weights.y * pc2 + weights.z * pc3;
            diffuse.xyz = mix(diffuse.xyz, vec3(tpColor.x * diffuse.x, tpColor.y * diffuse.y, tpColor.z * diffuse.z), planarTexStrength);
            shininess = mix(shininessRange.x, shininessRange.y, tpColor.w);
        }
    
        if (useTriplanarBumpTextures)
        {
            // TODO: optimize
            vec3 normal1 = texture2D(planarBumpTexX, uvs[0] * planarBumpTexScale.x).xyz * 2.0 - vec3(1.0, 1.0, 1.0);
            vec3 normal2 = texture2D(planarBumpTexY, uvs[1] * planarBumpTexScale.y).xyz * 2.0 - vec3(1.0, 1.0, 1.0);
            vec3 normal3 = texture2D(planarBumpTexZ, uvs[2] * planarBumpTexScale.z).xyz * 2.0 - vec3(1.0, 1.0, 1.0);
        
            vec3 tangent;
            float maxWeight = max(weights.x, weights.y);
            maxWeight = max(maxWeight, weights.z);
            if (maxWeight == weights.x)
            {
                tangent = cross(vec3(0.0, 0.0, -1.0), modelNormal);
            }
            else if (maxWeight == weights.y)
            {
                tangent = cross(vec3(0.0, 0.0, 1.0), modelNormal);
            }
            else if (maxWeight == weights.z)
            {
                tangent = cross(vec3(0.0, 1.0, 0.0), modelNormal);
            }
            
            vec3 bitangent = cross(modelNormal, tangent);
        
            vec3 perturbedNormal1 = mix(N, normal1.x * tangent + normal1.y * bitangent + normal1.z * modelNormal, planarBumpTexStrength.x);
            vec3 perturbedNormal2 = mix(N, normal2.x * tangent + normal2.y * bitangent + normal2.z * modelNormal, planarBumpTexStrength.y);
            vec3 perturbedNormal3 = mix(N, normal3.x * tangent + normal3.y * bitangent + normal3.z * modelNormal, planarBumpTexStrength.z);
        
            vec3 perturbedNormal = weights.x * perturbedNormal1 + 
                                   weights.y * perturbedNormal2 + 
                                   weights.z * perturbedNormal3;
            perturbedNormal = normalize(perturbedNormal);
            N = perturbedNormal;
        }
    }
    N = normalize(worldViewNormalMat * N);
    
    vec3 color = vec3(0.0, 0.0, 0.0);
	
    if (useEnvMaps)
    {
        // diffuse light color
        vec2 envMapUV = N.xy * 0.5 + vec2(0.5, 0.5);
        vec3 diffLC = texture2D(diffuseEnvMap, envMapUV).xyz;
        diffLC = inverseGammaCorrection(diffLC);
        
        // specular light color
        vec3 R = normalize(reflect(viewPos, N));
        R = normalize((R + vec3(0.0, 0.0, 1.0)) / 2.0);
        envMapUV = R.xy * 0.5 + vec2(0.5, 0.5);
        vec3 specLC = texture2D(specularEnvMap, envMapUV).xyz;
        specLC = inverseGammaCorrection(specLC);
        
        // combined result
        color += diffLC * diffuse.xyz + specLC * fresnel(specularColor.xyz, dot(V, N));
    }
    
    for (int i = 0; i < MAX_LIGHTS; ++i)
    {
        if (i == numLights)
        {
            break;
        }
        
        vec3 ld = lightDirectColor[i];
        vec3 lightDirView = lightDir[i]; 
        
        if (numShadows > 0)
        {
            lightDirView = (viewMat * vec4(lightDir[i], 0.0)).xyz;
        
            if (i == 0)
            {
                ld *= shadow(shadowCoord, shadowMap, N, lightDirView, 1.0);
            }
            else if (i == 1 && numShadows > 1)
            {
                ld *= shadow(shadowCoord2, shadowMap2, N, lightDirView, 1.0);
            }
        }
        else
        {
            lightDirView = lightDir[i];
        }
        
        color += calculateReflectance(
            N, V,
            lightDirView, lightAmbientColor[i], ld,
            diffuse.rgb, specularColor.rgb, shininess);
    }

    vec4 cursor;
    if (usePatternCursor)
    {
        vec4 patternTexCoord[MAX_CURSORS];
        for (int i = 0; i < MAX_CURSORS; ++i)
        {
            patternTexCoord[i] = patternMat[i] * vec4(modelPos, 1.0);
            if (i == cursorCnt)
            {
                break;
            }
        }
        
        cursor = calculateCursorColor(
            cursorCnt, 
            patternTex, 
            patternTexCoord, 
            patternViewDir, 
            N);
            
        cursor.xyz *= cursorColor;
    }
    else
    {
        cursor.xyz = cursorColor;
        cursor.a = calculateCursorStrength(cursorCnt, cursorPos, cursorRadius, modelPos);
    }
    
    bool masked = useVertColor && vertColor.x > 0.9999;
    if (!masked)
    {
        /*if (usePatternCursor)
        {
            color.xyz += 0.3 * cursor.xyz * cursor.a;
        }
        else*/
        {
            color.xyz = mix(color.xyz, cursor.xyz, cursor.a * 0.4);
        }
    }
    
    float axisTerm = calculateSimmetryAxisTerm(showSymmetryAxes, modelPos, vmfScale);
    if (axisTerm > 0.0)
    {
        color = color * (1.0 - axisTerm) + axisTerm * vec3(0.5, 0.5, 0.5);
    }
    
    color = gammaCorrection(color);
    
	if (masked)
    {
        color = mix(color, vec3(0.5, 0.2, 0.2), 0.5);
    }
	
	gl_FragColor = vec4(color, diffuseColor.a);
}
