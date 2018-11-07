varying vec2 vVaryingTexCoords;
uniform sampler2D tex_in;
uniform sampler2D tex_alpha;
void main(void)
{
	vec4  vColor;
	vec4  vAlpha;
	vColor = texture(tex_in, vVaryingTexCoords);
	vAlpha = texture(tex_alpha, vVaryingTexCoords);
	vColor.a = vAlpha.r;
	gl_FragColor = vColor;
}
