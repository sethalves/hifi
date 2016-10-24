uniform sampler2D colorTex;
uniform vec2 uvOffset;

varying vec2 fTexCoord0;


#if 0
// Unfortunately GLES SL 1.0 doesn't support initializing of arrays at creation time

// 5x5 Gaussian-like blur kernel
const float kernelWeights[25] = 
{
    2.0, 7.0, 12.0, 7.0, 2.0,
    7.0, 31.0, 52.0, 31.0, 7.0,
    12.0, 52.0, 127.0, 52.0, 12.0,
    7.0, 31.0, 52.0, 31.0, 7.0,
    2.0, 7.0, 12.0, 7.0, 2.0
};
const float kernelWeightSum = 571.0;

const vec2 kernelOffsets[25] = 
{
    vec2(-2.0, -2.0), vec2(-1.0, -2.0), vec2(0.0, -2.0), vec2(1.0, -2.0), vec2(2.0, -2.0),
    vec2(-2.0, -1.0), vec2(-1.0, -1.0), vec2(0.0, -1.0), vec2(1.0, -1.0), vec2(2.0, -1.0),
    vec2(-2.0, 0.0), vec2(-1.0, 0.0), vec2(0.0, 0.0), vec2(1.0, 0.0), vec2(2.0, 0.0),
    vec2(-2.0, 1.0), vec2(-1.0, 1.0), vec2(0.0, 1.0), vec2(1.0, 1.0), vec2(2.0, 1.0),
    vec2(-2.0, 2.0), vec2(-1.0, 2.0), vec2(0.0, 2.0), vec2(1.0, 2.0), vec2(2.0, 2.0)
};

#else

uniform float kernelWeights[25];
uniform float kernelWeightSum;
uniform vec2 kernelOffsets[25];

#endif


void main()
{
    vec4 color = vec4(0.0, 0.0, 0.0, 0.0);
    vec4 offsetColor;
    
    for (int i = 0; i < 25; ++i)
    {
        offsetColor = texture2D(colorTex, fTexCoord0 + uvOffset * kernelOffsets[i]);
        offsetColor.rgb *= offsetColor.a;
        offsetColor *= kernelWeights[i];
        color += offsetColor;
    }
    
    gl_FragColor = color / kernelWeightSum;
}
