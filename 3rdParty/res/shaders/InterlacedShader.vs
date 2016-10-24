attribute vec4 vPosition;
attribute vec2 vTexCoord0;
varying vec2 fTexCoord0;

void main()
{
	fTexCoord0 = vTexCoord0;
	gl_Position = vPosition;
}
