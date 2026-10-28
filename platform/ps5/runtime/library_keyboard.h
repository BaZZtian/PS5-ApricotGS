// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "../src/platform.h"
#include <algorithm>
#include <string>
namespace apricot::library {
class TextKeyboard {
public:
    unsigned page=0,index=0;
    explicit TextKeyboard(bool uppercase=false):page(uppercase?1:0){}
    const std::string&characters()const{
        static const std::string pages[]={"abcdefghijklmnopqrstuvwxyz0123456789.-_/:@","ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789.-_/:@"," !\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~0123456789"};return pages[page];
    }
    std::string case_label()const{return page==0?"abc - Kleinbuchstaben":page==1?"ABC - Grossbuchstaben":"SYM - Sonderzeichen";}
    std::string key_label(unsigned i)const{return i==characters().size()?(page==1?"abc":"ABC"):characters()[i]==' '?"SP":std::string(1,characters()[i]);}
    void apply(uint32_t buttons,std::string&value,size_t maximum){
        if((buttons&PAD_SQUARE)&&!value.empty())value.pop_back();
        if(buttons&PAD_R1){page=(page+1)%3;index=0;}else if(buttons&PAD_L1){page=(page+2)%3;index=0;}
        unsigned size=unsigned(characters().size()+1);
        if(buttons&PAD_LEFT)index=(index+size-1)%size;if(buttons&PAD_RIGHT)index=(index+1)%size;
        if(buttons&PAD_UP)index=(index+size-12)%size;if(buttons&PAD_DOWN)index=(index+12)%size;
        if(buttons&PAD_CROSS){if(index==characters().size()){page=page==1?0:1;index=unsigned(characters().size());}else if(value.size()<maximum)value+=characters()[index];}
    }
};
inline std::string keyboard_field(const std::string&value,bool secret){return secret?std::string(std::min(size_t(80),value.size()),'*'):value.substr(value.size()>80?value.size()-80:0);}
}
