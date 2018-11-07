varying vec2 vVaryingTexCoords;
uniform sampler2D tex_in;
uniform float fAlpha;
uniform float thr0Min;
uniform float thr0Max;
uniform float thr1Min;
uniform float thr1Max;
void main(void)
{
	vec4  vColor;
	vec4  vAlpha;
	float ra, ga, ba;
	vColor = texture(tex_in, vVaryingTexCoords);
	//vColor.a = fAlpha;
	//vColor.a = step(thrMin, vColor.r)*step(vColor.r, thrMax)*fAlpha;
	//vColor.a = smoothstep(thrMin, thrMax, vColor.r)*step(vColor.r, thrMax)*fAlpha;
	ra = (1-step(thr0Min, vColor.r)*step(vColor.r, thr0Max))*(1-step(thr1Min, vColor.r)*step(vColor.r, thr1Max));
	ga = (1-step(thr0Min, vColor.g)*step(vColor.g, thr0Max))*(1-step(thr1Min, vColor.g)*step(vColor.g, thr1Max));
	ba = (1-step(thr0Min, vColor.b)*step(vColor.b, thr0Max))*(1-step(thr1Min, vColor.b)*step(vColor.b, thr1Max));	
	vColor.a = (1-ra*ga*ba)*fAlpha;
	gl_FragColor = vColor;
}
