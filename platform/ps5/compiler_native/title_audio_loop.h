// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <vector>
#include <cstdint>
#include <cmath>
#include <algorithm>
#include <cstring>
#include <string>
namespace apricot::aot {
struct TitleAudioLoop {uint32_t begin=0,end=0;double confidence=0;};
// Discover a complete repeating cycle from locally generated native audio.
// Require two complete cycles after the initial minute, a strong full-tail
// match and no independent competing period. A failed match never becomes an
// arbitrary-duration loop. No game samples, scores or per-game timings here.
inline bool find_title_audio_loop(const std::vector<int16_t>&samples,TitleAudioLoop&out,double rate=33513982.0/1024){
 out={};const size_t frames=samples.size()/2;if(rate<100||rate>48000||frames<size_t(rate*425)||frames>size_t(rate*450)||samples.size()%2)return false;
 auto envelope=[&](unsigned step){std::vector<double>v;v.reserve(frames/step);for(size_t n=0;n+step<=frames;n+=step){double sum=0;for(unsigned k=0;k<step;++k)for(unsigned c=0;c<2;++c){double x=samples[(n+k)*2+c];sum+=x*x;}v.push_back(std::sqrt(sum/(2*step)));}return v;};
 auto correlation=[](const std::vector<double>&v,size_t first,size_t lag,size_t count=0){
   if(first+lag>=v.size())return -1.0;count=count?std::min(count,v.size()-first-lag):v.size()-first-lag;
   if(count<16)return -1.0;double sx=0,sy=0,sxx=0,syy=0,sxy=0;
   for(size_t n=0;n<count;++n){double x=v[first+n],y=v[first+lag+n];sx+=x;sy+=y;sxx+=x*x;syy+=y*y;sxy+=x*y;}
   double vx=sxx-sx*sx/count,vy=syy-sy*sy/count;if(sx/count<32||sy/count<32||vx/count<256||vy/count<256)return -1.0;
   return (sxy-sx*sy/count)/std::sqrt(vx*vy);
 };
 unsigned coarse=unsigned(rate/10),fine=std::max(1u,unsigned(rate/100));auto e=envelope(coarse);size_t first=size_t(rate*60)/coarse;
 std::vector<double>scores(e.size(),-1);size_t best=0;double peak=-1;
 for(size_t lag=size_t(rate*30)/coarse;lag<=size_t(rate*180)/coarse&&first+2*lag<e.size();++lag){double s=correlation(e,first,lag);scores[lag]=s;if(s>peak+1e-9){peak=s;best=lag;}}
 if(peak<.94)return false;
 for(size_t lag=0;lag<scores.size();++lag){if(scores[lag]<peak-.10||std::abs(double(lag)-best)<rate*.5/coarse)continue;
   double multiple=double(lag)/best,reverse=double(best)/std::max(size_t(1),lag);
   if(std::abs(multiple-double(uint32_t(multiple+.5)))>.01&&std::abs(reverse-double(uint32_t(reverse+.5)))>.01)return false;
 }
 auto detail=envelope(fine);size_t begin=size_t(rate*60)/fine,center=best*coarse/fine,radius=std::max(size_t(2),size_t(rate*.2)/fine),chosen=0;double confidence=-1;
 for(size_t lag=center>radius?center-radius:1;lag<=center+radius&&begin+2*lag<detail.size();++lag){double s=correlation(detail,begin,lag);if(s>confidence){confidence=s;chosen=lag;}}
 if(confidence<.94||!chosen)return false;
 // Each quarter must agree, so one loud repeated phrase cannot dominate.
 for(unsigned part=0;part<4;++part)if(correlation(detail,begin+part*chosen/4,chosen,chosen/4)<.85)return false;
 size_t loop_begin=size_t(rate*60),period=chosen*fine;double seam=1e300;size_t aligned=period;
 for(int delta=-int(fine);delta<=int(fine);++delta){size_t p=size_t(int64_t(period)+delta);double error=0;
   for(unsigned k=0;k<256;++k)for(unsigned c=0;c<2;++c){double d=double(samples[(loop_begin+k)*2+c])-samples[(loop_begin+p+k)*2+c];error+=d*d;}
   if(error<seam){seam=error;aligned=p;}
 }
 if(loop_begin+aligned>frames||loop_begin+aligned>rate*300)return false;
 out={uint32_t(loop_begin),uint32_t(loop_begin+aligned),confidence};return true;
}
inline bool make_title_audio(const std::vector<int16_t>&samples,const uint8_t*identity,std::vector<uint8_t>&audio,std::string&error,TitleAudioLoop*verified=nullptr){
 TitleAudioLoop loop;if(!identity||!find_title_audio_loop(samples,loop)){error="Complete title music loop could not be verified; existing media preserved";return false;}
 size_t source=loop.end;uint64_t count=uint64_t(source)*48000*1024/33513982,begin=uint64_t(loop.begin)*48000*1024/33513982;
 if(!count||count>48000*300||begin>=count){error="Title music outside bounds";return false;}
 audio.assign(64+size_t(count)*4,0);auto put=[&](size_t at,uint64_t value,unsigned bytes=4){for(unsigned n=0;n<bytes;++n)audio[at+n]=uint8_t(value>>(8*n));};
 memcpy(audio.data(),"APAU0001",8);put(8,48000);audio[12]=2;audio[14]=16;put(16,count);put(20,begin);put(24,count);memcpy(audio.data()+32,identity,32);
 for(uint64_t n=0;n<count;++n){uint64_t position=n*33513982,den=48000*1024,at=std::min(uint64_t(source-1),position/den),frac=position%den,next=std::min(uint64_t(source-1),at+1);
   for(unsigned c=0;c<2;++c){int64_t value=(int64_t(samples[at*2+c])*int64_t(den-frac)+int64_t(samples[next*2+c])*int64_t(frac))/int64_t(den);put(64+n*4+c*2,uint16_t(int16_t(value)),2);}
 }
 if(verified)*verified=loop;return true;
}

}
