"""Check actual guild attention counters against reward/help eligibility."""
from pathlib import Path
import subprocess, tempfile
source=Path('main.cpp').read_text()
def section(start,end):
    a=source.index(start);return source[a:source.index(end,a)]
code='''#include <string>
#include <vector>
#include <ctime>
#include <cassert>
constexpr int kGtCount=10;
double GetTime(){return 100;}
'''
code+=section('struct GuildNetRow {','static int GuildTechIndex(')
code+=section('static int GuildRewardCount()','static std::string GuildAttentionLabel() {')
code+='''int main(){
assert(GuildRewardCount()==0 && GuildHelpCount()==0);
g_gnet.gifts.push_back({0,"friend"});
g_gnet.cityRuns={{"ok","expedition",3,3,0,true,true,true,false},
{"claimed","boss",3,3,0,true,true,true,true},
{"unjoined","boss",3,3,0,true,true,false,false},
{"failed","boss",3,0,0,true,false,true,false},
{"ongoing","boss",3,3,20,false,true,true,false}};
g_gnet.cityBattles={{"past","enemy",0,1,1,false,0,0},
{"claimed","enemy",0,1,1,true,0,0},
{"inactive","enemy",0,1,0,false,0,0},
{"future","enemy",0,9223372036854775807LL,1,false,0,0}};
assert(GuildRewardCount()==3);
g_gnet.helps={{1,"friend","build",0,4,false,false},
{2,"me","build",0,4,true,false},
{3,"friend","build",0,4,false,true},
{4,"friend","build",4,4,false,false}};
g_gnet.hubAt=90;g_gnet.buildLeft=20;g_gnet.lent=false;
assert(GuildHelpCount()==2);
g_gnet.lent=true;assert(GuildHelpCount()==1);
g_gnet.lent=false;g_gnet.buildLeft=5;assert(GuildHelpCount()==1);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'menu-status.cpp'; exe=Path(tmp)/'menu-status'
    cpp.write_text(code)
    subprocess.run(['g++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS menu attention: actionable gifts, activity/battle rewards, help eligibility and expired upgrades')
