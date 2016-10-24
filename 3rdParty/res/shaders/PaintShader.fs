varying vec3 fModelPos;     // position in object space

uniform bool fullTrianglePaint;
uniform int cursorCnt;

uniform bool usePatternPaint;
uniform bool usePatternColors;
uniform sampler2D patternTex;                 // pattern projection texture
uniform mat4 patternMat[MAX_CURSORS];         // transforms from modelSpace to projective texture space
uniform vec3 cursorPos[MAX_CURSORS];
uniform float cursorRadius;
uniform vec4 paintColor;

uniform bool paintClipPlaneEnabled;
uniform vec3 paintClipPlaneCenter;
uniform vec3 paintClipPlaneNormal;


void main()
{
    float t = 0.0;
    vec3 color = paintColor.rgb;
    
    if (paintClipPlaneEnabled)
    {
        float t = dot(fModelPos - paintClipPlaneCenter, paintClipPlaneNormal);
        if (t < 0.0)
        {
            discard;
            return;
        }
    }
    if (fullTrianglePaint)
    {
        gl_FragColor = paintColor;
    }
    else
    {
        for (int i = 0; i < MAX_CURSORS; ++i)
        {
            if (i == cursorCnt)
            {
                break;
            }
            
            if (usePatternPaint)
            {
                vec4 patternTexCoord = patternMat[i] * vec4(fModelPos, 1.0);
                vec4 patternColor = texture2DProj(patternTex, patternTexCoord);
                if (usePatternColors)
                {
                    color = patternColor.rgb;
                }
                float ti = patternColor.a;// > 0.0 ? 1.0 : 0.0;
                t += ti;
            }
            else
            {
                float ti = distance(cursorPos[i], fModelPos) / cursorRadius;
                t += (1.0 - smoothstep(0.8, 1.0, ti));
            }
        }
        gl_FragColor = vec4(color, paintColor.a * t);
    }
}
