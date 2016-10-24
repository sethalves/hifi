uniform sampler2D uCamDepth;
uniform sampler2D uLightDepth;

varying vec2 fTexCoord0;

uniform vec2 screenSize;

uniform mat4 uInvProjMat;
uniform mat4 uProjectionMat;
uniform int uSampleKernelSize;
const int MAX_KERNEL = 64;
uniform vec3 uSampleKernel[MAX_KERNEL];
uniform sampler2D uTexRandom;

uniform float uRadius;			// used by method 0, 1, 2, 3,4,5,6
uniform float uRangeRadius;		// used by method 0, 1, 2

//METHOD 0..6, method 2 is deleted
#define METHOD 0
uniform float uIntensity;		// used by method 0,1,2,3,4,5,6
uniform float uBrighten;		// used by method 0

/*
const float PI = 3.14159265358979323846264;
const int NUM_SAMPLES = 16;   		// used by method 3,4,5,6
uniform int NUM_SPIRAL_TURNS;	// used by method 3,4,5,6
uniform float uBias;			// used by method 3,4,5,6
*/

/** The height in pixels of a 1m object if viewed from 1m away.  
    You can compute it from your projection matrix.  The actual value is just
    a scale factor on radius; you can simply hardcode this to a constant (~500)
    and make your radius value unitless (...but resolution dependent.)  */
//uniform float uProjScale;		// used by method 3,4,5,6



vec3 decode(vec2 enc)
{
    vec2 fenc = enc*4.0-2.0;
    float f = dot(fenc,fenc);
    float g = sqrt(1.0-f/4.0);
    vec3 n;
    n.xy = fenc*g;
    n.z = 1.0-f/2.0;
    return n;
}

/*
vec3 normal_from_depth(float depth, vec2 texcoords) {
  
  const vec2 offset1 = vec2(0.0,0.001);
  const vec2 offset2 = vec2(0.001,0.0);

  float depth1 = unpackNUI16toNUF(texture2D(uCamDepth, texcoords+offset1).xy)*2.0-1.0;
  float depth2 = unpackNUI16toNUF(texture2D(uCamDepth, texcoords+offset2).xy)*2.0-1.0;
  
  vec3 p1 = vec3(offset1, depth1 - depth);
  vec3 p2 = vec3(offset2, depth2 - depth);
  
  vec3 normal = cross(p1, p2);
  normal.z = -normal.z;
  
  return normalize(normal);
}
*/

vec3 calculatePosition(in vec2 coord, in float depth)
{
    // Get the depth value for this pixel
    float z = depth; 
    // Get x/w and y/w from the viewport position
    float x = coord.x * 2.0 - 1.0;
    float y = (coord.y) * 2.0 - 1.0;
    vec4 vProjectedPos = vec4(x, y, z, 1.0);
    // Transform by the inverse projection matrix
    vec4 vPositionVS = uInvProjMat * vProjectedPos;  
    // Divide by w to get the view-space position
    return vPositionVS.xyz / vPositionVS.w;  
}
	
//TODO

/** Scalable ambient obscurance webGL implementation from:
// https://gist.github.com/fisch0920/6770311
// The code is based on the nvidia paper and demo source:
// http://graphics.cs.williams.edu/papers/SAOHPG12/
**/

/*
vec3 getPositionVS(vec2 uv) {
  // Get the depth value for this pixel
    float z = unpackNUI16toNUF(texture2D(uCamDepth, uv).xy)*2.0-1.0; 
    // Get x/w and y/w from the viewport position
    float x = uv.x * 2.0 - 1.0;
    float y = (uv.y) * 2.0 - 1.0;
    vec4 vProjectedPos = vec4(x, y, z, 1.0);
    // Transform by the inverse projection matrix
    vec4 vPositionVS = uInvProjMat * vProjectedPos;  
    // Divide by w to get the view-space position
    return vPositionVS.xyz / vPositionVS.w;  
}
 
// returns a unit vector and a screen-space radius for the tap on a unit disk 
// (the caller should scale by the actual disk radius)
vec2 tapLocation(int sampleNumber, float spinAngle, out float radiusSS) {
  // radius relative to radiusSS
  float alpha = (float(sampleNumber) + 0.5) * (1.0 / float(NUM_SAMPLES));
  float angle = alpha * (float(NUM_SPIRAL_TURNS) * 6.28) + spinAngle;
  
  radiusSS = alpha;
  return vec2(cos(angle), sin(angle));
}
 
vec3 getOffsetPositionVS(vec2 uv, vec2 unitOffset, float radiusSS) {
  uv = uv + radiusSS * unitOffset * (1.0 / screenSize);
  
  return getPositionVS(uv);
}
 
float sampleAO(vec2 uv, vec3 positionVS, vec3 normalVS, float sampleRadiusSS, 
               int tapIndex, float rotationAngle)
{
  const float epsilon = 0.01;
  float radius2 = uRadius * uRadius;
  
  // offset on the unit disk, spun for this pixel
  float radiusSS;
  vec2 unitOffset = tapLocation(tapIndex, rotationAngle, radiusSS);
  radiusSS *= sampleRadiusSS;
  
  vec3 Q = getOffsetPositionVS(uv, unitOffset, radiusSS);
  vec3 v = Q - positionVS;
  
  float vv = dot(v, v);
  float vn = dot(v, normalVS) - uBias;
  
#if METHOD == 3
  
  // (from the HPG12 paper)
  // Note large epsilon to avoid overdarkening within cracks
  return float(vv < radius2) * max(vn / (epsilon + vv), 0.0);
  
#elif METHOD == 4 // default / recommended
  
  // Smoother transition to zero (lowers contrast, smoothing out corners). [Recommended]
  float f = max(radius2 - vv, 0.0) / radius2;
  return f * f * f * max(vn / (epsilon + vv), 0.0);
  
#elif METHOD == 5
  
  // Medium contrast (which looks better at high radii), no division.  Note that the 
  // contribution still falls off with radius^2, but we've adjusted the rate in a way that is
  // more computationally efficient and happens to be aesthetically pleasing.
  float invRadius2 = 1.0 / radius2;
  return 4.0 * max(1.0 - vv * invRadius2, 0.0) * max(vn, 0.0);
  
#else
  
  // Low contrast, no division operation
  return 2.0 * float(vv < radius2) * max(vn, 0.0);
  
#endif
}

float SAO()
{
  //vec2 vUV = (texCoord.xy / texCoord.w);
  vec2 vUV = fTexCoord0;

  vec3 originVS = getPositionVS(vUV);
  vec3 normalVS = decode(texture2D(uCamDepth, vUV).zw);
  
  vec2 noiseScale = vec2(screenSize.x / 4.0, screenSize.y / 4.0);
  vec3 sampleNoise = texture2D(uTexRandom, vUV * noiseScale).xyz*2.0-1.0;
  
  float randomPatternRotationAngle = 2.0 * PI * sampleNoise.x;
  
  float radiusSS  = 0.0; // radius of influence in screen space
  float radiusWS  = 0.0; // radius of influence in world space
  float occlusion = 0.0;
  
  //float projScale = 40.0;//1.0 / (2.0 * tan(uFOV * 0.5));
  radiusWS = uRadius;
  radiusSS = uProjScale * radiusWS / originVS.y;
  
  for (int i = 0; i < NUM_SAMPLES; ++i) {
    occlusion += sampleAO(vUV, originVS, normalVS, radiusSS, i, 
                          randomPatternRotationAngle);
  }
  
  occlusion = 1.0 - occlusion / (4.0 * float(NUM_SAMPLES));
  occlusion = clamp(pow(occlusion, 1.0 + uIntensity), 0.0, 1.0);
  //gl_FragColor = vec4(occlusion, occlusion, occlusion, 1.0);
	return occlusion;
}
	
*/

float SSAO2()
{
	/*float bias = 0.005;
	vec3 lightViewTexCoord = (shadowCoord.xyz/ shadowCoord.w);
	float visibility = 0.0;*/

	vec2 noiseScale = vec2(screenSize.x / 4.0, screenSize.y / 4.0);
	vec2 texcoordFS = fTexCoord0;

	vec3 normal = decode(texture2D(uCamDepth, texcoordFS).zw);
			//normalize(viewNormal);
	//return normal;
	float depth = unpackNUI16toNUF(texture2D(uCamDepth, texcoordFS).xy)*2.0-1.0;
	
	//vec3 normal = normal_from_depth(depth, texcoordFS);
	
	//return 1-depth;
    vec3 origin = calculatePosition(texcoordFS, depth);
	//return ( vec3(invView*vec4(origin, 1.0)));
	
	vec3 rvec = (texture2D(uTexRandom, texcoordFS*noiseScale)).xyz*2.0-1.0;
    vec3 tangent = normalize(rvec - normal * dot(rvec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 tbn = mat3(tangent, bitangent, normal);
	
	float occlusion = 0.0;
	for (int i = 0; i < MAX_KERNEL; ++i) {
		if (i == uSampleKernelSize)
        {
            break;
        }
		// get sample position:
		vec3 kernel = tbn * uSampleKernel[i];
		vec3 sample = kernel * uRadius + origin;
	  
		// project sample position:
		vec4 offset = uProjectionMat * vec4(sample, 1.0);
		offset.xy /= offset.w;
		offset.xy = offset.xy * 0.5 + 0.5;
	  
		// get sample depth, and caculate position in view space:
		float sampleDepth = (unpackNUI16toNUF(texture2D(uCamDepth, offset.xy).xy)*2.0-1.0);// -near)/(far-near); //texture2D(uTexLinearDepth, offset.xy).r;
		vec3 samplePos = calculatePosition(offset.xy, sampleDepth);
		sampleDepth = samplePos.z;
	   
		// range check & accumulate:
		//float rangeCheck= abs(origin.z - sampleDepth) < uRadius ? 1.0 : 0.0;
		//occlusion += (sampleDepth <= sample.z ? 1.0 : 0.0);//* rangeCheck;
	   
		float rangeCheck = 1.0 - smoothstep(0.0, 1.0, uRangeRadius / abs(origin.z - sampleDepth));
	   
#if METHOD == 0
	    
			vec3 sampleDir = normalize(samplePos - origin);
			// angle between SURFACE-NORMAL and SAMPLE-DIRECTION (vector from SURFACE-POSITION to SAMPLE-POSITION)
			float NdotS = dot(normal, sampleDir);
			if(NdotS < 0.0) NdotS = -uBrighten;
			occlusion += (NdotS * rangeCheck);
		
#else
		
			occlusion +=  step(sampleDepth, sample.z)* rangeCheck;
		
#endif
	}
	occlusion = 1.0 - occlusion / float(uSampleKernelSize);
	occlusion = clamp(pow(occlusion, 1.0 + uIntensity), 0.0, 1.0);
	
	return (occlusion);
   
}

void main()
{
	gl_FragColor = vec4(vec3(SSAO2()), 1.0);
}