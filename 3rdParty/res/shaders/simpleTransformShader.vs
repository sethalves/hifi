attribute vec3 vPosition;
attribute vec4 vColor0;     // deform weight in w component

uniform mat4 worldViewProjMat;

#ifdef USE_DEFORMER
uniform bool useDeformers;
uniform float rwParams_weight;
uniform vec3 rwParams_axisP1;
uniform vec4 rwParams_axisDir;
uniform vec3 rwParams_waveParams;
uniform vec2 rwParams_awcp[4];

uniform float dsParams_weight;
uniform vec3 dsParams_center;
uniform vec4 dsParams_dir;
#endif


void main()
{
    vec3 modelPos;
    
#ifdef USE_DEFORMER
    if (useDeformers)
    {
        modelPos = deform(
            vec4(vPosition, vColor0.w),
            rwParams_weight, rwParams_axisP1, rwParams_axisDir, rwParams_waveParams, rwParams_awcp,
            dsParams_weight, dsParams_center, dsParams_dir);
    }
    else
#endif
    {
        modelPos = vPosition;
    }
    
    gl_Position = worldViewProjMat * vec4(modelPos, 1.0);
}
