// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "game_module.h"
#include <vector>
#include <algorithm>
#include <cstring>
struct PreparedMenuAudio {
 std::vector<int16_t> samples;uint32_t cursor=0,frames=0,loop_begin=0;
 bool load(const std::vector<uint8_t>& data,const GameIdentity& identity){
   if(data.size()<64||data.size()>64*1024*1024||memcmp(data.data(),"APAU0001",8)||memcmp(data.data()+32,identity.sha256,32))return false;
   auto u32=[&](unsigned at){uint32_t v=0;for(unsigned n=0;n<4;++n)v|=uint32_t(data[at+n])<<(8*n);return v;};
   uint32_t count=u32(16);
   if(u32(8)!=48000||data[12]!=2||data[13]||data[14]!=16||data[15]||!count||count>48000*300||u32(20)>=count||u32(24)!=count||u32(28)||data.size()!=64+size_t(count)*4)return false;
   std::vector<int16_t> decoded(size_t(count)*2);
   for(size_t n=0;n<decoded.size();++n){uint16_t v=uint16_t(data[64+n*2])|uint16_t(data[65+n*2])<<8;decoded[n]=int16_t(v);}
   samples=std::move(decoded);frames=count;loop_begin=u32(20);cursor=0;return true;
 }
 static void fill(int16_t* output,int count,void* context){auto& q=*static_cast<PreparedMenuAudio*>(context);
   for(int n=0;n<count;++n){if(!q.frames){output[n*2]=output[n*2+1]=0;continue;}
     unsigned fade=q.frames<256?q.frames/2:128;if(!fade)fade=1;
     unsigned edge=q.cursor>=q.loop_begin?q.cursor-q.loop_begin+1:q.cursor+1;
     unsigned gain=std::min({fade,edge,q.frames-q.cursor});
     output[n*2]=int16_t(int64_t(q.samples[q.cursor*2])*gain/fade);
     output[n*2+1]=int16_t(int64_t(q.samples[q.cursor*2+1])*gain/fade);
     if(++q.cursor==q.frames)q.cursor=q.loop_begin;
   }
 }
};
// Keep both prepared timelines advancing, but pin the first valid title track
// before audio_start. Cartridge selection never changes the audible track.
struct PreparedMenuMixer {
 PreparedMenuAudio tracks[2];
 unsigned pinned=0;
 void pin(unsigned index){if(index<2)pinned=index;}
 static void fill(int16_t* output,int count,void* context){
   auto& m=*static_cast<PreparedMenuMixer*>(context);
   for(int n=0;n<count;++n){
     int16_t sample[2][2]{};
     for(unsigned track=0;track<2;++track)PreparedMenuAudio::fill(sample[track],1,&m.tracks[track]);
     output[n*2]=sample[m.pinned][0];
     output[n*2+1]=sample[m.pinned][1];
   }
 }
};
