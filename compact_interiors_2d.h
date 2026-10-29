#pragma once
// Compact the authored shop plan, retaining furniture and character sizes.
namespace tfinterior {
inline Vector2 Size(const std::string& key) {
    if(key=="stable" || key=="townhall")return {420,420};
    if(key=="smith" || key=="carpenter")return {400,400};
    if(key=="healer")return {380,380};
    if(key=="house")return {340,340};
    return {360,360};
}
inline Vector2 Point(Vector2 p,Vector2 size) {
    return {6+(p.x-6)*(size.x-12)/508,6+(p.y-6)*(size.y-12)/608};
}
inline Vector2 Spawn(const std::string& key) {auto s=Size(key);return {s.x*.5f,s.y-56};}
}
