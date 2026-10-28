#pragma once
// Shared presentation only: stable world detail, cool contact shade and warm lights.
// Neither the seed nor the drawing routines consume gameplay random numbers.
namespace tfscene {
static Texture2D dungeonFloors[6]{},dungeonWalls[6]{};
inline const Texture2D* DungeonSurface(int theme,bool wall) {
    theme=std::clamp(theme,0,5);auto& tex=wall?dungeonWalls[theme]:dungeonFloors[theme];
    if(!tex.id) {
        constexpr int tile=128;
        const Color floors[6]={{78,91,94,255},{83,78,83,255},{100,87,70,255},{82,65,59,255},{112,137,150,255},{82,86,72,255}};
        const Color walls[6]={{43,55,60,255},{48,43,51,255},{58,50,42,255},{43,33,34,255},{58,77,91,255},{43,49,39,255}};
        Color base=wall?walls[theme]:floors[theme];Image image=GenImageColor(tile,tile,base);
        if(!image.data)return nullptr; // Allocation failure falls back to the existing themed surface.
        auto* pixels=(Color*)image.data;
        int rowHeight=wall?32:16;
        for(int y=0;y<tile;y++)for(int x=0;x<tile;x++) {
            int row=y/rowHeight,xx=(x+(row%2)*16)%32,yy=y%rowHeight;
            float grain=(tf2dart::Noise(x+theme*197,y)-.5f)*9;
            float block=(tf2dart::Noise((x+(row%2)*16)/32,row)-.5f)*11;
            bool seam=xx<1 || yy<1;bool bevel=xx==1 || yy==1;
            float value=grain+block+(seam?-18:bevel?5:0);
            pixels[y*tile+x]={(unsigned char)std::clamp(base.r+value,0.f,255.f),
                             (unsigned char)std::clamp(base.g+value,0.f,255.f),
                             (unsigned char)std::clamp(base.b+value,0.f,255.f),255};
        }
        tex=LoadTextureFromImage(image);UnloadImage(image);if(tex.id)SetTextureFilter(tex,TEXTURE_FILTER_POINT);
    }
    return tex.id?&tex:nullptr;
}
inline void Unload() {
    for(auto& t:dungeonFloors){if(t.id)UnloadTexture(t);t={};}
    for(auto& t:dungeonWalls){if(t.id)UnloadTexture(t);t={};}
}
inline void Grain(Rectangle world,Vector2 camera,Rectangle view,bool grass=false) {
    float l=std::max(world.x,camera.x),t=std::max(world.y,camera.y);
    float r=std::min(world.x+world.width,camera.x+view.width),b=std::min(world.y+world.height,camera.y+view.height);
    constexpr float step=32;
    for(int y=(int)floorf(t/step);y*step<b;y++)for(int x=(int)floorf(l/step);x*step<r;x++) {
        float wx=x*step+tf2dart::Noise(x,y)*23,wy=y*step+tf2dart::Noise(y+37,x)*23;
        if(wx<l+4 || wy<t+4 || wx>r-5 || wy>b-5)continue;
        Vector2 p={view.x+wx-camera.x,view.y+wy-camera.y};
        Color shade=grass?Color{37,56,36,35}:Color{23,30,35,37};
        DrawLineEx(p,{p.x+3,p.y+1},1,shade);
        DrawLineEx({p.x+1,p.y+2},{p.x+4,p.y+2},1,Color{224,209,167,23});
        if(grass && tf2dart::Noise(x+7,y)>.7f)DrawLineEx(p,{p.x-1,p.y-3},1,shade);
    }
}
inline void Light(Vector2 p,float radius,float strength,Color color=Color{255,191,108,255}) {
    // Broad and quiet; avoids bright disks and remains compatible with raylib 6.
    DrawCircleGradient(p,radius,Fade(color,strength),Fade(color,0));
}
inline void Contact(Vector2 foot,float width,float height) {
    tf2dart::Shadow(foot,width,height,.22f);
}
inline void RoomEdges(const std::vector<Rectangle>& rooms,Vector2 camera,Rectangle view) {
    for(const auto& room:rooms)for(int side=0;side<4;side++) {
        bool horizontal=side<2;float length=horizontal?room.width:room.height;
        for(float d=0;d<length;d+=16) {
            float span=std::min(16.f,length-d);
            Vector2 edge={horizontal?room.x+d:(side==2?room.x:room.x+room.width),horizontal?(side==0?room.y:room.y+room.height):room.y+d};
            Vector2 p={view.x+edge.x-camera.x,view.y+edge.y-camera.y};
            if(p.x<view.x-18 || p.y<view.y-18 || p.x>view.x+view.width+18 || p.y>view.y+view.height+18)continue;
            Vector2 outside={edge.x+(horizontal?span*.5f:(side==2?-2.f:2.f)),edge.y+(horizontal?(side==0?-2.f:2.f):span*.5f)};
            bool exposed=true;for(const auto& other:rooms)if(CheckCollisionPointRec(outside,other)){exposed=false;break;}
            if(!exposed)continue; // Corridors stay visually connected to rooms.
            for(int inset=0;inset<7;inset++) {
                Color c={15,22,29,(unsigned char)(64-inset*8)};
                if(horizontal)DrawRectangle((int)p.x,(int)(p.y+(side==0?inset:-inset-1)),(int)ceilf(span),1,c);
                else DrawRectangle((int)(p.x+(side==2?inset:-inset-1)),(int)p.y,1,(int)ceilf(span),c);
            }
        }
    }
}
}
