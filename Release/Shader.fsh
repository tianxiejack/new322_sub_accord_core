varying vec2 vVaryingTexCoords;
uniform sampler2D tex_in;
void main(void)
{
	gl_FragColor = texture(tex_in, vVaryingTexCoords);
}
