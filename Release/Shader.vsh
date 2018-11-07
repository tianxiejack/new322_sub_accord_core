attribute vec4 vVertex; 
attribute vec2 vTexCoords;
varying vec2 vVaryingTexCoords;
void main(void)
{
    gl_Position = vVertex;
    vVaryingTexCoords = vTexCoords;
}
