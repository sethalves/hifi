uniform sampler2D colorTex;

#if defined(DISPLAY_DEPTH_TEXTURE) && defined(LINEARIZE_DEPTH)
uniform vec4 clipPlaneValues;   // near, far, near + far, far - near
#endif

varying vec2 fTexCoord0;

void main()
{
#ifdef DISPLAY_DEPTH_TEXTURE

    vec2 packedDepth = texture2D(colorTex, fTexCoord0).rg;
    float depth = unpackNUI16toNUF(packedDepth);
    
#ifdef LINEARIZE_DEPTH
    // depth buffer Z-value remapped to a linear [0..1] range (near plane to far plane)
    float n = clipPlaneValues.x;
    float f = clipPlaneValues.y;
    float npf = clipPlaneValues.z;
    float fmn = clipPlaneValues.w;
    depth = 2.0 * n * depth / (npf - depth * fmn);
#endif
    
    gl_FragColor = vec4(depth, depth, depth, 1.0);
#else

    gl_FragColor = texture2D(colorTex, fTexCoord0);
    
#endif
}
