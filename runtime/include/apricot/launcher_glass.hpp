// SPDX-License-Identifier: MIT
#pragma once
#include <algorithm>
#include <cstdint>
#include <vector>
namespace apricot {
// Static background only: blur/tint once before the frame loop. The PS5 canvas
// supports cutout transparency, so blend glass against the actual artwork here.
inline void launcher_glass(std::vector<uint32_t>& canvas,const std::vector<uint8_t>& rgba,unsigned source_width,unsigned source_height,int left=580,int top=562,int right=1248,int bottom=710,bool output_bgra=true){
 if(canvas.size()!=1280*720||left<0||top<0||right>1280||bottom>720||right-left<28||bottom-top<28)return;
 bool artwork=source_width&&source_height&&source_width<=3840&&source_height<=2160&&rgba.size()==size_t(source_width)*source_height*4;
 // A broad box blur removes fine background detail; retained low-frequency color
 // gives the material depth without putting high-frequency artwork behind text.
 constexpr int stride=1281,radius=14,blur=18;
 std::vector<uint32_t> integral;
 if(artwork){integral.assign(size_t(1281)*721*3,0);
   for(int y=0;y<720;++y){uint32_t row[3]{};for(int x=0;x<1280;++x){auto p=rgba.data()+(size_t(unsigned(y)*source_height/720)*source_width+unsigned(x)*source_width/1280)*4;
     for(unsigned c=0;c<3;++c){row[c]+=p[c];integral[((size_t(y)+1)*stride+x+1)*3+c]=integral[(size_t(y)*stride+x+1)*3+c]+row[c];}}}
 }
 for(int y=top;y<bottom;++y)for(int x=left;x<right;++x){
   int dx=std::max({left+radius-x,0,x-(right-radius-1)}),dy=std::max({top+radius-y,0,y-(bottom-radius-1)});if(dx*dx+dy*dy>radius*radius)continue;
   int x0=std::max(0,x-blur),x1=std::min(1280,x+blur+1),y0=std::max(0,y-blur),y1=std::min(720,y+blur+1);
   unsigned tint[3]={210,230,242},alpha=y<top+2?210:176;uint32_t pixel=0xff000000u;
   for(unsigned c=0;c<3;++c){unsigned average=0;if(artwork)average=(integral[(size_t(y1)*stride+x1)*3+c]+integral[(size_t(y0)*stride+x0)*3+c]-integral[(size_t(y0)*stride+x1)*3+c]-integral[(size_t(y1)*stride+x0)*3+c])/unsigned((x1-x0)*(y1-y0));
     unsigned value=(average*(255-alpha)+tint[c]*alpha+127)/255;pixel|=value<<((output_bgra?2-c:c)*8);}
   canvas[size_t(y)*1280+x]=pixel;
 }

}
inline int centered_text_x(float center,size_t letters,int scale){return int(center)-int(letters)*6*scale/2;}
inline double confirmation_spin(double base,double progress){
 double t=std::clamp(progress,0.,1.);return base+12.566370614359172*(1.-(1.-t)*(1.-t)*(1.-t));
}
}
