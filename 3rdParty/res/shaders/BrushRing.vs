attribute vec4 vPosition;

varying vec2 pos;

void main()
{
    pos = vPosition.xy;
	gl_Position = vPosition;
}
