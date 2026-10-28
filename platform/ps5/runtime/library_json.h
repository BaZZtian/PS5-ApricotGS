// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>
namespace apricot::library {
// Small bounded JSON reader for RomM metadata. No executable or generated game data.
struct Json {
    enum Kind{Null,Bool,Number,String,Array,Object} kind=Null;
    std::string text;std::vector<Json> items;std::map<std::string,Json> fields;
    const Json& get(const std::string& key)const{static const Json empty;auto i=fields.find(key);return i==fields.end()?empty:i->second;}
    uint64_t integer(uint64_t fallback=0)const{if(kind!=Number||text.empty())return fallback;uint64_t n=0;for(char c:text){if(c<'0'||c>'9'||n>(UINT64_MAX-uint64_t(c-'0'))/10)return fallback;n=n*10+uint64_t(c-'0');}return n;}
    static bool parse(const std::string& data,Json& out){
        struct Reader {
            const std::string&s;size_t at=0,nodes=0;
            void ws(){while(at<s.size()&&(s[at]==' '||s[at]=='\n'||s[at]=='\r'||s[at]=='\t'))++at;}
            int hex(){if(at>=s.size())return -1;char c=s[at++];return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:c>='A'&&c<='F'?c-'A'+10:-1;}
            bool unicode(unsigned&v){v=0;for(int i=0;i<4;++i){int x=hex();if(x<0)return false;v=v*16+unsigned(x);}return true;}
            void utf8(std::string&o,unsigned v){if(v<128)o+=char(v);else if(v<2048){o+=char(192|(v>>6));o+=char(128|(v&63));}else if(v<65536){o+=char(224|(v>>12));o+=char(128|((v>>6)&63));o+=char(128|(v&63));}else{o+=char(240|(v>>18));o+=char(128|((v>>12)&63));o+=char(128|((v>>6)&63));o+=char(128|(v&63));}}
            bool string(std::string&o){if(at>=s.size()||s[at++]!='"')return false;o.clear();while(at<s.size()){unsigned char c=s[at++];if(c=='"')return true;if(c<32)return false;if(c!='\\'){o+=char(c);continue;}if(at==s.size())return false;char e=s[at++];switch(e){case '"':case '\\':case '/':o+=e;break;case 'b':o+='\b';break;case 'f':o+='\f';break;case 'n':o+='\n';break;case 'r':o+='\r';break;case 't':o+='\t';break;case 'u':{unsigned v;if(!unicode(v))return false;if(v>=0xd800&&v<=0xdbff){if(at+2>s.size()||s[at++]!='\\'||s[at++]!='u')return false;unsigned low;if(!unicode(low)||low<0xdc00||low>0xdfff)return false;v=0x10000+((v-0xd800)<<10)+low-0xdc00;}else if(v>=0xdc00&&v<=0xdfff)return false;utf8(o,v);break;}default:return false;}if(o.size()>1024*1024)return false;}return false;}
            bool value(Json&o,unsigned depth){
                if(depth>48||++nodes>100000)return false;ws();if(at>=s.size())return false;char c=s[at];
                if(c=='"'){o.kind=String;return string(o.text);}
                if(c=='['||c=='{'){++at;bool obj=c=='{';o.kind=obj?Object:Array;ws();if(at<s.size()&&s[at]==(obj?'}':']')){++at;return true;}for(;;){std::string key;if(obj){ws();if(!string(key))return false;ws();if(at==s.size()||s[at++]!=':')return false;}Json child;if(!value(child,depth+1))return false;if(obj){if(!o.fields.emplace(key,std::move(child)).second)return false;}else o.items.push_back(std::move(child));ws();if(at==s.size())return false;char end=s[at++];if(end==(obj?'}':']'))return true;if(end!=',')return false;}}
                for(const char* literal:{"null","true","false"}){std::string l=literal;if(s.compare(at,l.size(),l)==0){at+=l.size();o.kind=l=="null"?Null:Bool;o.text=l;return true;}}
                size_t begin=at;if(s[at]=='-')++at;if(at==s.size()||s[at]<'0'||s[at]>'9')return false;if(s[at]=='0')++at;else while(at<s.size()&&s[at]>='0'&&s[at]<='9')++at;
                if(at<s.size()&&s[at]=='.'){++at;size_t b=at;while(at<s.size()&&s[at]>='0'&&s[at]<='9')++at;if(b==at)return false;}
                if(at<s.size()&&(s[at]=='e'||s[at]=='E')){++at;if(at<s.size()&&(s[at]=='+'||s[at]=='-'))++at;size_t b=at;while(at<s.size()&&s[at]>='0'&&s[at]<='9')++at;if(b==at)return false;}
                o.kind=Number;o.text=s.substr(begin,at-begin);return true;
            }
        }r{data};if(data.size()>8*1024*1024)return false;Json result;if(!r.value(result,0))return false;r.ws();if(r.at!=data.size())return false;out=std::move(result);return true;
    }
};
inline std::string url_escape(const std::string&s){const char*h="0123456789ABCDEF";std::string out;for(unsigned char c:s){if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='-'||c=='_'||c=='.'||c=='~')out+=char(c);else {out+='%';out+=h[c>>4];out+=h[c&15];}}return out;}
inline bool base_url(std::string&url){
    if(url.find("://")==std::string::npos)url="https://"+url;
    if(url.size()>1024||url.find_first_of("\r\n\t ?#\\")!=std::string::npos||url.find('\0')!=std::string::npos)return false;
    size_t start=url.compare(0,8,"https://")==0?8:url.compare(0,7,"http://")==0?7:0;if(!start)return false;
    auto end=url.find('/',start);auto host=url.substr(start,end==std::string::npos?end:end-start);if(host.empty()||host.find('@')!=std::string::npos)return false;
    while(url.size()>start&&url.back()=='/')url.pop_back();return true;
}
}
