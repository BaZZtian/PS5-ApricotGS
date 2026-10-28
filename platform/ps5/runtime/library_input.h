// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "../src/platform.h"
namespace apricot::library {
struct MenuActions {uint32_t pressed=0;int horizontal=0,vertical=0;};
// Caller-owned state: one physical press cannot activate consecutive views.
class MenuInput {
    uint32_t previous=0;bool release_required=true;int direction=0;double next_repeat=0;
public:
    void enter(){release_required=true;direction=0;}
    MenuActions update(uint32_t held,float x,float y,double now){
        MenuActions out;uint32_t edge=held&~previous;previous=held;
        if(release_required){if(!held&&x>-.35f&&x<.35f&&y>-.35f&&y<.35f)release_required=false;return out;}
        out.pressed=edge&~uint32_t(PAD_UP|PAD_DOWN|PAD_LEFT|PAD_RIGHT);
        int wanted=0;
        bool up=held&PAD_UP,down=held&PAD_DOWN,left=held&PAD_LEFT,right=held&PAD_RIGHT;
        if(up!=down)wanted=up?-2:2;else if(left!=right)wanted=left?-1:1;
        else if(!up&&!down&&!left&&!right){
            float ax=x<0?-x:x,ay=y<0?-y:y;
            if(ay>=.65f&&ay>=ax)wanted=y<0?-2:2;else if(ax>=.65f)wanted=x<0?-1:1;
            else if(direction&&((direction==2&&y>.35f)||(direction==-2&&y<-.35f)||(direction==1&&x>.35f)||(direction==-1&&x<-.35f)))wanted=direction;
        }
        bool emit=false;
        if(wanted!=direction){direction=wanted;next_repeat=now+.42;emit=wanted!=0;}
        else if(direction&&now>=next_repeat){emit=true;next_repeat=now+.12;}
        if(emit){if(direction==1||direction==-1){out.horizontal=direction;out.pressed|=direction<0?PAD_LEFT:PAD_RIGHT;}
            else {out.vertical=direction<0?-1:1;out.pressed|=direction<0?PAD_UP:PAD_DOWN;}}
        return out;
    }
};
}
