varying vec3 TexCoord0;
uniform samplerCube uCubemapTexture;

void main()
{
	vec3 texcoord = TexCoord0;
	texcoord.y = -texcoord.y;
    gl_FragColor = textureCube(uCubemapTexture, texcoord);
}