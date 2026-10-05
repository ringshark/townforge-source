// Native raylib 6 review harness. Run from the repository root.
// Uses the production character, equipment, animation and town renderer.
void JS_CloudOpen(){}
void CleanupAndClose(){}
#define main game_main
#include "../../main.cpp"
#undef main
int main(int argc, char** argv){
 InitWindow(1100,760,"Knight skin review");rlSetClipPlanes(10,5000);HumanEnsure();Town3DEnsureLit();
 GameState s;s.selectedTown=0;s.worldTime=12*60*60;s.townPlayerPos={675,900};s.playerFacing={0,1};
 Equipment e;e.rightHand=Item{1,"Longsword",ItemType::Weapon,"","1h",10,"Swordsmanship"};e.leftHand=Item{2,"Round Shield",ItemType::Armor,"shield","",10,""};s.equipped=e;
 g_alwaysDay=true;Town3DLoadModels();Town3DEnsureGround(s);T3DUpdateDayNight(s.worldTime,false);
 RenderTexture2D rt[4];for(auto &t:rt)t=LoadRenderTexture(550,380);
 const char* label[]={"Complete knight - idle","Complete knight - walking","Complete knight - sword attack","In the actual town renderer"};
 for(int f=0;f<45;f++){g_gameClock=f/30.;for(int k=0;k<4;k++){
 Camera3D cam{{90,65,145},{0,32,0},{0,1,0},30,CAMERA_PERSPECTIVE};if(k==3)cam={{1035,310,1330},{675,32,900},{0,1,0},35,CAMERA_PERSPECTIVE};
 SetShaderValue(g_t3dLit.shader,g_t3dLit.viewPosLoc,&cam.position,SHADER_UNIFORM_VEC3);
 BeginTextureMode(rt[k]);ClearBackground({29,37,44,255});BeginMode3D(cam);
 if(k==3)Town3DDrawSceneContents(s,false);else{DrawPlane({0,0,0},{900,900},{66,73,58,255});HumanPose p;if(k==1)p.move=.85f;if(k==2){p.attackT=.45f;p.attackDuration=1;p.engaged=true;}DrawEquippedHero(300+k,0,0,k==1?3.14f:1.57f,WHITE,e,p,false);}
 EndMode3D();EndTextureMode();}
 BeginDrawing();ClearBackground(BLACK);for(int k=0;k<4;k++){float x=(k%2)*550,y=(k/2)*380;DrawTextureRec(rt[k].texture,{0,0,550,-380},{x,y},WHITE);DrawRectangle(x,y,550,40,{29,37,44,230});DrawText(label[k],x+16,y+12,18,{230,231,223,255});}rlDrawRenderBatchActive();if(f==44){Image im=LoadImageFromScreen();ExportImage(im,argc > 1 ? argv[1] : "knight-skin-review.png");UnloadImage(im);}EndDrawing();}
 CloseWindow();return 0;
}
