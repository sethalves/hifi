varying vec2 fTexCoord0;
uniform int texWidth;
uniform int texHeight;
uniform sampler2D uInputTex;
uniform sampler2D uDepthTex;
uniform float uRange;
//uniform float uKernelWeights[16];

//uniform int uBlurSize = 4; // use size of noise texture
const int uBlurSize = 4;

void main() {
	vec2 texelSize =  1.0 / vec2(texWidth, texHeight);
	float depth = unpackNUI16toNUF(texture2D(uDepthTex, fTexCoord0).xy)*2.0-1.0;
	vec4 centerColor = texture2D(uInputTex, fTexCoord0);
	
//	ideally use a fixed size noise and blur so that this loop can be unrolled
	vec4 fResult = vec4(0.0);
	vec2 hlim = vec2(float(-uBlurSize) * 0.5);
	float sum = 0.0;
	for (int x = 0; x < uBlurSize; ++x) {
		for (int y = 0; y < uBlurSize; ++y) {
			vec2 offset = vec2(float(x), float(y));
			offset += hlim;
			offset *= texelSize;
			vec4 offsetColor = texture2D(uInputTex, fTexCoord0 + offset);
			
			float SampleDepth = unpackNUI16toNUF(texture2D(uDepthTex, fTexCoord0 + offset).xy)*2.0-1.0;
			float weight = (1.0 - smoothstep(0.0, uRange, abs(depth-SampleDepth)) * smoothstep(0.0, 0.5, distance(centerColor.xyz, offsetColor.xyz)));
			//float weight = 1.0;
			sum += weight;
			fResult += offsetColor * weight;
		}
	}
	
	fResult = fResult / sum;
	gl_FragColor = vec4(fResult.rgb, 1.0);
}