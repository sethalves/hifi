uniform sampler2D texLeft;
uniform sampler2D texRight;
uniform float colored;
uniform vec3 greyWeights;
uniform vec3 leftWeights;
uniform vec3 rightWeights;

varying vec2 fTexCoord0;

void main()
{
	vec4 left = texture2D(texLeft, fTexCoord0);
	vec4 right = texture2D(texRight, fTexCoord0);

	vec3 leftColor;
	vec3 rightColor;

	if (colored == 0.0)
	{
		leftColor = leftWeights * left.rgb;
		rightColor = rightWeights * right.rgb;
	}
	else
	{
		float leftGrey = dot(greyWeights, left.rgb);
		float rightGrey = dot(greyWeights, right.rgb);
		leftColor = leftGrey * leftWeights;
		rightColor = rightGrey * rightWeights;
	}
	gl_FragColor = vec4(leftColor + rightColor, 1.0);
}