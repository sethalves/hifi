varying vec3 modelPos;                  // position in modelSpace

uniform int numShadows;
uniform sampler2D shadowMap;
uniform sampler2D shadowMap2;
varying vec4 shadowCoord;
varying vec4 shadowCoord2;

uniform vec4 groundColor;
uniform bool useGroundTex;
uniform sampler2D uGroundTex;
uniform vec4 fogColor;
uniform float fogDensity;

uniform float vmfScale;

void main()
{
	float z = gl_FragCoord.z / gl_FragCoord.w;
	
#if 1
	// exp2 fog
	const float SQRT2 = 1.442695;
	float fogFactor = exp2(- fogDensity * fogDensity * z * z * SQRT2);
#else
	// linear fog
	const float start = 0.001;
	const float end = 10.5;
	float fogFactor = (end-z)/(end-start);
	fogFactor = clamp(fogFactor, 0.0, 1.0);
#endif
	
    vec4 color;
    if (useGroundTex)
    {
        vec2 texCoord = (modelPos.xz*0.5+0.5); //* 1000.0 * vmfScale;
        color = texture2D(uGroundTex, texCoord);
    }
    else
    {
        color = groundColor;
    }
	
	if(numShadows > 0)
	{
		color *= shadow(shadowCoord, shadowMap, vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0), 0.2);
		if (numShadows > 1)
        {
			color *= shadow(shadowCoord2, shadowMap2, vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0), 0.2);
        }
	}
	vec4 fragColor = vec4(color.rgb, 1.0);
	gl_FragColor = mix(fogColor, fragColor, fogFactor);
}
