"""Exercise the production C++ guild parser, including an empty last field."""
from pathlib import Path
import subprocess, tempfile
source = Path('main.cpp').read_text()
def section(start, end):
    a = source.index(start)
    return source[a:source.index(end, a)]
code = '''#include <string>
#include <vector>
#include <sstream>
#include <cstdlib>
#include <cstring>
#include <cassert>
constexpr int kGtCount = 10;
std::string wire;
int JS_GuildNetState(char* out,int n) { std::strncpy(out,wire.c_str(),n); return wire.size(); }
double GetTime() { return 1.0; }
int GuildTechIndex(const std::string&) { return -1; }
'''
code += section('static std::vector<std::string> SplitStr(', '// Item <->')
code += section('struct GuildNetRow {', 'static int GuildTechIndex(')
code += section('static void GuildNetPoll() {', '// ---- Guild Hall (2026-09-29):')
code += '''int main() {
wire = "cfg=1\\nready=1\\nbusy=0\\nmy=g|Wolves|IW\\nhall=1|0|30|-1|0|0|0|\\n";
GuildNetPoll(); assert(g_gnet.hub); assert(g_gnet.rec.empty()); assert(g_gnet.hall == 1);
wire = "my=g|Wolves|IW\\nhall=2|400|32|1200|3600|1|1|timber\\n";
GuildNetPoll(); assert(g_gnet.hub); assert(g_gnet.rec == "timber"); assert(g_gnet.lent);
wire = "my=g|Wolves|IW\\nhall=3|400|32|-1|0|1|0|\\ncitystock=50|60|70|2\\ncitybuilding=workshop|2|120\\ncityrun=5|boss|200|30|1800|0|0|1|0\\ncitybattle=8|enemy|100|200|2|0|30|40\\ncitypending=1\\n";
GuildNetPoll(); assert(g_gnet.cityReady && g_gnet.cityPending); assert(g_gnet.cityWood==50 && g_gnet.cityContracts==2);
assert(g_gnet.cityBuildings[0].level==2 && g_gnet.cityRuns[0].joined && g_gnet.cityBattles[0].mine==30);
wire = "cfg=1\\nready=1\\n"; GuildNetPoll(); assert(!g_gnet.hub); assert(!g_gnet.cityReady && g_gnet.cityRuns.empty());
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp = Path(tmp)/'guild-parser.cpp'; exe = Path(tmp)/'guild-parser'
    cpp.write_text(code)
    subprocess.run(['g++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS: production guild parser accepts empty recommendation, reads build state, clears departed guild')
