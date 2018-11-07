varying vec2 vVaryingTexCoords;
uniform sampler2D tex_in;
uniform mat4 mTrans;
void main(void)
{
	vec4  vColor;	
	vec4 vTex44;
	vTex44.x = vVaryingTexCoords.x - 0.5f; 
	vTex44.y = vVaryingTexCoords.y - 0.5f; 
	vTex44.z = 0.0f;
	vTex44.w = 0.5f;
	vTex44 = mTrans*vTex44;
	//vTex44.xy *= vTex44.w;
	vTex44.x += 0.5f; 
	vTex44.y += 0.5f;

	//if(vTex44.x > 0.0001f && vTex44.x < 1.0001f && vTex44.y > 0.0001f && vTex44.y < 1.0001f)
	{
		vColor = texture(tex_in, vTex44);
	}	
	gl_FragColor = vColor;
}
