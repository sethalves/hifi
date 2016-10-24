uniform bool useTexture;

attribute vec4 vPosition;
attribute vec2 vTexCoord0;
attribute vec4 vColor0;

varying vec4 fColor0;
varying vec2 fTexCoord0;


void main()
{
	if(useTexture)
	{
        fTexCoord0 = vTexCoord0;
	}
	else
	{
        fColor0 = vColor0;
	}
    
    gl_Position = vPosition;
    gl_Position.z = 0.99;
}