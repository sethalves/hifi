uniform mat4 worldViewProjMat;
uniform mat3 viewNormalMat;
attribute vec3 vPosition;
attribute vec4 vColor0;         // deform weight in w component
attribute vec3 vNormal;
varying float fDepth;
varying vec3 normal;

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
	
    vec4 pos = worldViewProjMat * vec4(modelPos, 1.0);
	fDepth = pos.z/pos.w;
	fDepth = (fDepth + 1.0) * 0.5;
	normal = viewNormalMat * vNormal;
	gl_Position = pos;
}
