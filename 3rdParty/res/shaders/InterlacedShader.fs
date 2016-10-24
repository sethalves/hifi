uniform sampler2D texLeft;
uniform sampler2D texRight;
uniform float screenHeight;

varying vec2 fTexCoord0;

void main()
{
	vec4 leftColor = texture2D(texLeft, fTexCoord0);
	vec4 rightColor = texture2D(texRight, fTexCoord0);

	float lineY = screenHeight - gl_FragCoord.y - 0.5;

	float maskLeft = 2.0 * fract((lineY + 1.0) * 0.5);
	float maskRight = 2.0 * fract(lineY * 0.5);
	gl_FragColor = maskLeft * leftColor + maskRight * rightColor;
}