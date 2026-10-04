"""Verify seam smoothing keeps hard plate creases and never changes geometry."""
from pathlib import Path
import subprocess,tempfile
s=Path('main.cpp').read_text();start=s.index('static void SkinSmoothSeams(');helper=s[start:s.index('\n}\nstatic SkinChar*',start)+2]
code='''#include <array>
#include <map>
#include <vector>
#include <cmath>
#include <cassert>
struct Vector3{float x,y,z;};
Vector3 Vector3Add(Vector3 a,Vector3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
float Vector3DotProduct(Vector3 a,Vector3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Vector3 Vector3Normalize(Vector3 a){float l=sqrtf(Vector3DotProduct(a,a));return {a.x/l,a.y/l,a.z/l};}
struct Mesh{int vertexCount;float* vertices;float* normals;};struct Model{int meshCount;Mesh* meshes;};
void UpdateMeshBuffer(Mesh,int,const void*,int,int){}
'''+helper+'''
int main(){float vertices[]={0,0,0,0,0,0,0,0,0};float normals[]={0,1,0,.1f,.9949874f,0,1,0,0};Mesh mesh{3,vertices,normals};Model model{1,&mesh};SkinSmoothSeams(model);
for(int i=0;i<3;i++){Vector3 n{normals[i*3],normals[i*3+1],normals[i*3+2]};assert(fabsf(Vector3DotProduct(n,n)-1)<.00001f);}
assert(fabsf(normals[0]-normals[3])<.00001f&&fabsf(normals[1]-normals[4])<.00001f);
assert(normals[6]==1&&normals[7]==0&&normals[8]==0);
for(float v:vertices)assert(v==0);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 p=Path(tmp)/'seams.cpp';exe=Path(tmp)/'seams';p.write_text(code);subprocess.run(['g++','-std=c++17',str(p),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
print('PASS seam lighting: unit normals, welded UV seam, preserved hard crease and unchanged vertex positions')
