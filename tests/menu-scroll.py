"""Exercise production scroll ownership with two independently drawn columns."""
from pathlib import Path
import subprocess, tempfile

source = Path('main.cpp').read_text()
start = source.index('static bool g_scrollDragging = false;')
end = source.index('\n// ---------------------------------------------------------------------', start)
code = '''#include <cassert>
struct Vector2 {float x,y;};struct Rectangle {float x,y,width,height;};
enum {MOUSE_BUTTON_LEFT};
Vector2 mouse{};bool pressed=false,down=false;float wheel=0;
Vector2 GetMousePosition(){return mouse;}
bool IsMouseButtonPressed(int){return pressed;}
bool IsMouseButtonDown(int){return down;}
float GetMouseWheelMove(){return wheel;}
bool CheckCollisionPointRec(Vector2 p,Rectangle r){return p.x>=r.x && p.y>=r.y && p.x<r.x+r.width && p.y<r.y+r.height;}
''' + source[start:end] + '''
int main(){
 Rectangle left{20,240,448,282},right{492,240,448,282};
 mouse={600,400};pressed=down=true;
 assert(ScrollDelta(left)==0);assert(ScrollDelta(right)==0);
 pressed=false;mouse.y=350;
 assert(ScrollDelta(left)==0);assert(ScrollDelta(right)==-50);
 mouse={400,300};
 assert(ScrollDelta(left)==0);assert(ScrollDelta(right)==-50);
 down=false;assert(ScrollDelta(left)==0);assert(ScrollDelta(right)==0);
 mouse={100,400};pressed=down=true;
 assert(ScrollDelta(right)==0);assert(ScrollDelta(left)==0);
 pressed=false;mouse.y=360;
 assert(ScrollDelta(right)==0);assert(ScrollDelta(left)==-40);
 down=false;ScrollDelta(left);mouse={600,300};wheel=2;
 assert(ScrollDelta(left)==0);assert(ScrollDelta(right)==48);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp)/'scroll.cpp'; exe = Path(tmp)/'scroll'
    cpp.write_text(code)
    subprocess.run(['g++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS independent menu columns: touch ownership, crossing boundaries, release and wheel')
