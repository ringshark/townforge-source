#pragma once
static void DrawPetPack2D(GameState& s,Vector2 camera,int zone){
    static std::map<int,Vector2> previous;
    static std::map<int,std::pair<float,float>> health;
    auto pets=ActivePets(s);Vector2 player=zone?s.dungeonPlayerPos:s.wildernessPlayerPos;
    const Vector2* foe=zone?(s.dungeonEngaged?&s.dungeonEngaged->pos:nullptr):(s.wildEngaged?&s.wildEngaged->pos:nullptr);
    for(size_t index=0;index<pets.size();index++){
        Pet& pet=*pets[index];const auto& sheet=PetSheet(pet);if(!sheet.ok)continue;
        Vector2 at=index==0?s.companionPos:pet.fpos;
        if(index>0 && !pet.fposInit)continue;
        Vector2 velocity={at.x-previous[pet.id].x,at.y-previous[pet.id].y};previous[pet.id]=at;
        bool moving=std::hypot(velocity.x,velocity.y)>.12f,attacking=foe && s.companionAtkT>=0;
        Vector2 direction=attacking?Vector2{foe->x-at.x,foe->y-at.y}:moving?velocity:Vector2{player.x-at.x,player.y-at.y};
        float length=std::max(.001f,std::hypot(direction.x,direction.y));direction={direction.x/length,direction.y/length};
        Vector2 pos=WorldToScreen(at,camera);int species=PetSpecies(pet);float rotation=0;
        if(attacking){float pulse=sinf(std::clamp(s.companionAtkT/.7f,0.f,1.f)*PI);float reach=pet.role==PetRole::Tank?6:9;pos.x+=direction.x*reach*pulse;pos.y+=direction.y*reach*pulse;rotation=direction.x*pulse*7;
            if(pet.role==PetRole::Caster && s.companionAtkT>.25f && s.companionAtkT<.5f){auto target=WorldToScreen(*foe,camera);DrawLineEx(pos,target,2,Fade(species==10?ORANGE:SKYBLUE,.55f));}
        }
        auto& state=health[pet.id];if(state.first>pet.hp)state.second=s.worldTime;state.first=pet.hp;
        Color tint=s.worldTime-state.second<.22f?Color{255,151,136,255}:WHITE;
        tf2dart::Shadow({pos.x,pos.y+10},12,4,.16f);
        DrawActorSprite(sheet,direction,attacking?ActorAnim::Attack:moving?ActorAnim::Walk:ActorAnim::Idle,attacking?s.companionAtkT:s.worldTime,pos,34,8,rotation,tint);
        if(Dist(at,player)<120)DrawWorldLabel(pet.name,{pos.x,pos.y+28},10,Color{183,225,177,255});
    }
}
