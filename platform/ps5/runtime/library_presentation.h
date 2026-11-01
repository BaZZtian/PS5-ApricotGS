// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "apricot/launcher_glass.hpp"
#include "../src/menu.h"
#include "controller_legend.h"
namespace apricot::library {
constexpr int library_rows_top=236,library_row_step=48,library_footer_y=628;
constexpr int library_control_x=900,library_control_label_x=942;
inline bool library_background(std::vector<uint32_t>&canvas,const std::vector<uint8_t>&rgba){
    bool artwork=rgba.size()==3840u*2160*4;
    canvas.assign(1280*720,artwork?0:0xff142c3b);
    apricot::launcher_glass(canvas,rgba,3840,2160,56,90,1224,692,true);
    apricot::launcher_glass(canvas,rgba,3840,2160,56,22,1224,82,true);
    return artwork;
}
inline void launcher_controls(std::vector<uint32_t>&canvas,bool playable){
    using I=apricot::ControlIcon;
    const int xs[4]={602,748,900,1090};const char*labels[4]={"WAHL","START","BIBLIOTHEK","ENDE"};const I icons[4]={I::Dpad,I::Cross,I::Triangle,I::Options};
    const int text_x[4]={650,796,942,1138};
    for(int n=playable?0:2;n<4;++n){apricot::controller_icon(canvas,1280,720,xs[n],660,icons[n],true);menuProportionalText(canvas,text_x[n],menuTextCenteredY(labels[n],680),labels[n],0xff112c38,3,1280);}
}
inline void empty_edition_hint(std::vector<uint32_t>&canvas,const std::vector<uint8_t>&background,unsigned width,unsigned height,bool repair_required=false){
            apricot::launcher_glass(canvas,background,width,height,600,280,1248,448,true);
            const char*heading=repair_required?"ROM NICHT STARTBEREIT":"KEINE ROM VORHANDEN";
            menuProportionalText(canvas,924-menuTextWidth(heading,4)/2,298,heading,0xff112c38,4,1280);
            const char*hint="Eigene ROM in der Bibliothek auswaehlen";
            menuProportionalText(canvas,924-menuTextWidth(hint,3)/2,350,hint,0xff112c38,3,1280);
            apricot::controller_icon(canvas,1280,720,806,396,apricot::ControlIcon::Triangle,true);
            menuProportionalText(canvas,854,menuTextCenteredY("BIBLIOTHEK",416),"BIBLIOTHEK",0xff112c38,3,1280);
}
}
