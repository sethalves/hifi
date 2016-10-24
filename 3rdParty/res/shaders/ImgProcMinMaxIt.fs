uniform sampler2D inputTex;
uniform vec2 outputSize;        // size of the output image

void main()
{
    vec2 uv1 = (gl_FragCoord.xy + vec2(-0.25, -0.25)) / outputSize;
    vec2 uv2 = (gl_FragCoord.xy + vec2(0.25, -0.25)) / outputSize;
    vec2 uv3 = (gl_FragCoord.xy + vec2(-0.25, 0.25)) / outputSize;
    vec2 uv4 = (gl_FragCoord.xy + vec2(0.25, 0.25)) / outputSize;
    
    vec4 s1 = texture2D(inputTex, uv1);
    vec4 s2 = texture2D(inputTex, uv2);
    vec4 s3 = texture2D(inputTex, uv3);
    vec4 s4 = texture2D(inputTex, uv4);
    
    gl_FragColor.xy = min(s1.xy, s2.xy);
    gl_FragColor.xy = min(gl_FragColor.xy, s3.xy);
    gl_FragColor.xy = min(gl_FragColor.xy, s4.xy);
    
    gl_FragColor.zw = max(s1.zw, s2.zw);
    gl_FragColor.zw = max(gl_FragColor.zw, s3.zw);
    gl_FragColor.zw = max(gl_FragColor.zw, s4.zw);
}