// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <string>
namespace apricot::library {
inline bool pairing_code(const std::string&input,std::string&normalized){
    normalized.clear();for(unsigned char c:input){if(c=='-'||c==' ')continue;if(c>='a'&&c<='z')c=unsigned(c-'a'+'A');
        if(!((c>='A'&&c<='Z')||(c>='0'&&c<='9'))||normalized.size()==8){normalized.clear();return false;}normalized+=char(c);}
    if(normalized.size()!=8){normalized.clear();return false;}return true;
}
inline bool client_token_format(const std::string&token){
    if(token.size()!=68||token.compare(0,4,"rmm_"))return false;
    for(size_t i=4;i<token.size();++i){char c=token[i];if(!((c>='0'&&c<='9')||(c>='a'&&c<='f')||(c>='A'&&c<='F')))return false;}return true;
}
}
