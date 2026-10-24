#include "../homestead_life.h"
#include <cassert>
#include <iostream>
int main(){
 tflife::State s;s.farm=true;s.seeds={2,2,2};
 assert(tflife::Plant(s,0,0) && s.seeds[0]==1);
 assert(!tflife::Plant(s,0,1) && s.seeds[1]==2);
 tflife::Grow(s,1000);assert(s.beds[0].growth==0 && !tflife::Ready(s.beds[0]));
 assert(tflife::Water(s,0));assert(!tflife::Water(s,0));
 tflife::Grow(s,179);assert(!tflife::Ready(s.beds[0]));
 tflife::Grow(s,1);assert(tflife::Ready(s.beds[0]));
 assert(tflife::Harvest(s,0)==3 && s.produce[0]==3);
 assert(tflife::Harvest(s,0)==0 && s.beds[0].crop==-1);
 assert(!tflife::Plant(s,-1,0) && !tflife::Plant(s,6,0) && !tflife::Plant(s,0,3));
 tflife::Plant(s,1,2);tflife::Water(s,1);s.additions=9;s.farming=51;s.condition=28;
 tflife::State loaded;assert(tflife::Load(loaded,tflife::Save(s,100),400));
 assert(tflife::Ready(loaded.beds[1]) && loaded.additions==9 && loaded.farming==51 && loaded.condition==28);
 assert(tflife::Harvest(loaded,1)==3); // skill bonus, no well-kept bonus
 auto snapshot=tflife::Save(loaded,400);assert(!tflife::Load(loaded,"broken",500));assert(tflife::Save(loaded,400)==snapshot);
 assert(tflife::Load(loaded,tflife::Save(s,1000),200));assert(loaded.beds[1].growth==0); // clock rollback
 tflife::State danger;danger.cooldown=0;danger.check=1;
 assert(tflife::Watch(danger,2,false,true,0)==tflife::Encounter::None);
 assert(tflife::Watch(danger,2,true,false,0)==tflife::Encounter::None);
 assert(tflife::Watch(danger,2,true,true,.9f)==tflife::Encounter::None && danger.check==90);
 danger.check=1;assert(tflife::Watch(danger,2,true,true,.1f)==tflife::Encounter::Warning);
 assert(danger.warning==8 && danger.cooldown>=240);
 assert(tflife::Watch(danger,7,true,true,0)==tflife::Encounter::None);
 assert(tflife::Watch(danger,1,true,true,0)==tflife::Encounter::Arrive);
 assert(tflife::Watch(danger,1,true,true,0)==tflife::Encounter::None); // no chained raid
 danger.warning=4;assert(tflife::Watch(danger,1,true,false,0)==tflife::Encounter::Cancel);
 std::cout<<"PASS crops: seed costs, watering, growth, harvest, saved/offline state, malformed saves, clock rollback; packs: work-only chance, warning, cooldown and safe-area cancellation\n";
}
