#pragma once
#include <array>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

namespace tflife {
struct Crop {const char* name;float seconds;int seedPrice,salePrice;};
static constexpr Crop crops[]={{"Turnips",180,3,8},{"Beans",300,5,14},{"Herbs",240,4,10}};
struct Bed {int crop=-1;float growth=0;bool watered=false;};
struct State {
    bool farm=false;float x=0,y=0;
    std::array<int,3> seeds{},produce{};
    std::array<Bed,6> beds{};
    float farming=0,condition=100,careTime=0;
    int additions=0;
    float workTime=0,check=90,cooldown=180,warning=-1;
};
inline bool Ready(const Bed& b){return b.crop>=0 && b.crop<3 && b.growth>=crops[b.crop].seconds;}
inline bool Plant(State& s,int bed,int crop) {
    if(!s.farm || bed<0 || bed>=6 || crop<0 || crop>=3 || s.beds[bed].crop>=0 || s.seeds[crop]<=0)return false;
    --s.seeds[crop];s.beds[bed]={crop,0,false};s.workTime=30;return true;
}
inline bool Water(State& s,int bed) {
    if(!s.farm || bed<0 || bed>=6 || s.beds[bed].crop<0 || s.beds[bed].watered || Ready(s.beds[bed]))return false;
    s.beds[bed].watered=true;s.workTime=30;return true;
}
inline int Harvest(State& s,int bed) {
    if(bed<0 || bed>=6 || !Ready(s.beds[bed]))return 0;
    int n=2+(s.condition>=75?1:0)+(s.farming>=50?1:0);
    s.produce[s.beds[bed].crop]+=n;s.beds[bed]={};s.workTime=30;return n;
}
inline void Grow(State& s,float dt) {
    if(!std::isfinite(dt) || dt<=0)return;
    for(auto& b:s.beds)if(b.crop>=0 && b.crop<3 && b.watered)b.growth=std::min(crops[b.crop].seconds,b.growth+dt);
}
// A work-time roll, not a guaranteed repeating attack. Quiet play always has
// at least four minutes after a warning before another warning is possible.
enum class Encounter {None,Warning,Arrive,Cancel};
inline Encounter Watch(State& s,float dt,bool working,bool eligible,float roll) {
    if(!std::isfinite(dt) || dt<=0)return Encounter::None;
    s.workTime=std::max(0.f,s.workTime-dt);s.cooldown=std::max(0.f,s.cooldown-dt);
    if(s.warning>=0) {
        if(!eligible){s.warning=-1;return Encounter::Cancel;}
        s.warning-=dt;if(s.warning<=0){s.warning=-1;return Encounter::Arrive;}
        return Encounter::None;
    }
    if(!eligible || !working || s.cooldown>0)return Encounter::None;
    s.check-=dt;
    if(s.check>0)return Encounter::None;
    s.check=90;
    if(roll>=.30f)return Encounter::None;
    s.warning=8;s.cooldown=240+120*std::clamp(roll,0.f,1.f);return Encounter::Warning;
}
inline std::string Save(const State& s,long long now) {
    std::ostringstream o;o<<1<<' '<<now<<' '<<s.farm<<' '<<s.x<<' '<<s.y<<' '<<s.farming<<' '<<s.condition<<' '<<s.careTime<<' '<<s.additions<<' '<<s.check<<' '<<s.cooldown;
    for(int n:s.seeds)o<<' '<<n;for(int n:s.produce)o<<' '<<n;
    for(auto b:s.beds)o<<' '<<b.crop<<' '<<b.growth<<' '<<b.watered;
    return o.str();
}
inline bool Load(State& s,const std::string& text,long long now) {
    State n;std::istringstream in(text);int version=0,farm=0;long long epoch=0;
    if(!(in>>version>>epoch>>farm>>n.x>>n.y>>n.farming>>n.condition>>n.careTime>>n.additions>>n.check>>n.cooldown) || version!=1)return false;
    n.farm=farm==1;
    for(auto& v:n.seeds)if(!(in>>v))return false;
    for(auto& v:n.produce)if(!(in>>v))return false;
    for(auto& b:n.beds){int wet=0;if(!(in>>b.crop>>b.growth>>wet))return false;b.watered=wet==1;}
    for(float v:{n.x,n.y,n.farming,n.condition,n.careTime,n.check,n.cooldown})if(!std::isfinite(v))return false;
    n.farming=std::clamp(n.farming,0.f,100.f);n.condition=std::clamp(n.condition,0.f,100.f);n.careTime=std::clamp(n.careTime,0.f,1200.f);
    n.additions=std::clamp(n.additions,0,15);n.check=std::clamp(n.check,1.f,90.f);n.cooldown=std::clamp(n.cooldown,0.f,360.f);
    for(auto& v:n.seeds)v=std::clamp(v,0,100000);for(auto& v:n.produce)v=std::clamp(v,0,100000);
    for(auto& b:n.beds) {
        if(b.crop<0 || b.crop>=3 || !std::isfinite(b.growth)){b={};continue;}
        b.growth=std::clamp(b.growth,0.f,crops[b.crop].seconds);
    }
    if(epoch>0 && epoch<=now)Grow(n,(float)std::min(86400LL,now-epoch));
    s=n;return true;
}
}
