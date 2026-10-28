// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "apricot/launcher_glass.hpp"
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
}
