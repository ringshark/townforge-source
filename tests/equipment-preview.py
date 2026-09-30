"""Run the production equipment transition, preview and backpack operations."""
from pathlib import Path
import subprocess
import tempfile

s = Path('main.cpp').read_text()
def section(start, end):
    at = s.index(start)
    return s[at:s.index(end, at)]

code = '''#include <vector>
#include <string>
#include <optional>
#include <cassert>
enum class ItemType {Weapon, Armor, Clothing, Jewelry};
'''
code += section('struct Item {', '// ---- Clothing & dyes')
start = s.index('static std::optional<Item> Equipment::* ClothSlotField(')
code += s[start:s.index('\n}', start) + 2]
code += '\nstruct GameState {Equipment equipped;std::vector<Item> backpack;std::string logLine;int gold=200,hp=50;};\n'
code += section('static bool ApplyEquipmentItem(', '// JS sellValueFor()')
code += '''
Item item(int id,ItemType type,const char* slot="",const char* handed="") {
    return {id,"test item",type,slot,handed,10,""};
}
void same(const std::optional<Item>& a,const std::optional<Item>& b){assert(bool(a)==bool(b));if(a)assert(a->id==b->id);}
void sameEquipment(const Equipment& a,const Equipment& b){
    same(a.leftHand,b.leftHand);same(a.rightHand,b.rightHand);same(a.chest,b.chest);
    same(a.shirt,b.shirt);same(a.robe,b.robe);same(a.shoes,b.shoes);same(a.ring,b.ring);
}
int main(){
    Item sword=item(1,ItemType::Weapon,"","1h"),shield=item(2,ItemType::Armor,"shield");
    Item pole=item(3,ItemType::Weapon,"","2h"),robe=item(4,ItemType::Clothing,"robe");
    Item armor=item(5,ItemType::Armor,"chest"),ring=item(6,ItemType::Jewelry,"ring");
    Equipment base;base.rightHand=sword;base.leftHand=shield;
    Equipment trying=EquipmentPreview(base,&pole);
    assert(trying.leftHand->id==3&&trying.rightHand->id==3);
    assert(base.leftHand->id==2&&base.rightHand->id==1); // preview leaves actual gear intact
    GameState state;state.equipped=base;state.backpack={pole};
    EquipFromBackpack(state,0);sameEquipment(trying,state.equipped);
    assert(state.backpack.size()==2&&state.backpack[0].id==2&&state.backpack[1].id==1);
    trying=EquipmentPreview(state.equipped,&shield);assert(!trying.rightHand&&trying.leftHand->id==2);
    // A two-hander displaced by a shield is returned once, never duplicated.
    EquipFromBackpack(state,0);sameEquipment(trying,state.equipped);
    assert(state.backpack.size()==2&&state.backpack[1].id==3);
    state.equipped.leftHand=pole;state.equipped.rightHand=pole;state.backpack={sword};
    trying=EquipmentPreview(state.equipped,&sword);assert(!trying.leftHand&&trying.rightHand->id==1);
    EquipFromBackpack(state,0);sameEquipment(trying,state.equipped);
    assert(state.backpack.size()==1&&state.backpack[0].id==3);
    // Equipping another two-hander also returns the old item once.
    state.equipped.leftHand=pole;state.equipped.rightHand=pole;Item other=item(7,ItemType::Weapon,"","2h");state.backpack={other};
    EquipFromBackpack(state,0);assert(state.backpack.size()==1&&state.backpack[0].id==3);
    // Every clothing/armor/jewelry replacement uses the exact same rules.
    for(const Item& candidate:{robe,armor,ring,item(8,ItemType::Clothing,"shoes"),item(9,ItemType::Clothing,"shirt")}){
        state.backpack={candidate};auto original=state.equipped;auto pack=state.backpack;
        trying=EquipmentPreview(state.equipped,&candidate);
        sameEquipment(original,state.equipped);assert(state.backpack[0].id==pack[0].id);
        assert(state.gold==200&&state.hp==50);
        EquipFromBackpack(state,0);sameEquipment(trying,state.equipped);
    }
    Item invalid=item(10,ItemType::Clothing,"unknown");state.backpack={invalid};auto original=state.equipped;
    EquipFromBackpack(state,0);assert(state.backpack.size()==1);sameEquipment(original,state.equipped);
    sameEquipment(state.equipped,EquipmentPreview(state.equipped,nullptr)); // cancel
    EquipFromBackpack(state,-1);EquipFromBackpack(state,99);assert(state.backpack.size()==1);
}
'''
with tempfile.TemporaryDirectory() as temp:
    cpp = Path(temp) / 'equipment.cpp'
    exe = Path(temp) / 'equipment'
    cpp.write_text(code)
    subprocess.run(['g++', '-std=c++17', str(cpp), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
print('PASS equipment preview: no mutations; identical equip rules; two-hand transitions; no item duplication')
