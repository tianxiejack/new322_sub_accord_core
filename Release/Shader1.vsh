attribute vec4 vVertex; 
attribute vec2 vTexCoords;
varying vec2 vVaryingTexCoords;
uniform mat4 mTrans;
void main(void)
{
    gl_Position = mTrans*vVertex;
    vVaryingTexCoords = vTexCoords;
}
