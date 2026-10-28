// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <atomic>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
namespace apricot::library {
inline bool plain_name(const std::string& s) {
    return !s.empty() && s!="." && s!=".." && s.size()<256 &&
        s.find_first_of("/\\\r\n") == std::string::npos && s.find('\0')==std::string::npos;
}
// lstat every component: neither a chosen root nor a descendant may follow links.
inline bool directory(const std::string& path) {
    if(path.empty()||path[0]!='/'||path.size()>1024||path.find_first_of("\\\r\n")!=std::string::npos||path.find('\0')!=std::string::npos)return false;
    if(path=="/")return true;
    size_t at=1;std::string current;
    while(at<path.size()) {
        auto end=path.find('/',at);if(end==std::string::npos)end=path.size();
        auto part=path.substr(at,end-at);if(!plain_name(part))return false;
        current+="/"+part;struct stat st{};
        if(lstat(current.c_str(),&st)||!S_ISDIR(st.st_mode))return false;
        at=end+1;
    }
    return path.back()!='/';
}
inline std::string child(const std::string& path,const std::string& name){return path=="/"?path+name:path+"/"+name;}
inline bool nds_name(std::string name){std::transform(name.begin(),name.end(),name.begin(),[](unsigned char c){return char(c>='A'&&c<='Z'?c+32:c);});return name.size()>4&&name.substr(name.size()-4)==".nds";}
struct Entry{std::string name,path;bool folder=false;uint64_t bytes=0;};
inline bool list(const std::string& root,const std::string& path,std::vector<Entry>& out,std::string& error,const std::atomic<bool>*cancelled=nullptr){
    out.clear();if(!directory(root)||!directory(path)||(path!=root&&path.compare(0,root=="/"?1:root.size()+1,root=="/"?root:root+"/")!=0)){error="Speicherpfad nicht verfuegbar";return false;}
    DIR* d=opendir(path.c_str());if(!d){error="Ordner nicht lesbar";return false;}
    errno=0;bool ok=true;size_t seen=0;
    for(;;){
        if(cancelled&&cancelled->load()){error="Ordnerlesen abgebrochen";ok=false;break;}
        errno=0;auto*e=readdir(d);if(!e){if(errno){error="Ordner konnte nicht vollstaendig gelesen werden";ok=false;}break;}
        std::string name=e->d_name;if(name=="."||name=="..")continue;
        if(++seen>4096){error="Ordner enthaelt zu viele Eintraege (max. 4096)";ok=false;break;}
        if(!plain_name(name))continue;auto full=child(path,name);struct stat st{};
        if(lstat(full.c_str(),&st)){errno=0;continue;}
        bool folder=S_ISDIR(st.st_mode);if(!folder&&(!S_ISREG(st.st_mode)||!nds_name(name)))continue;
        if(access(full.c_str(),R_OK))continue;
        out.push_back({name,full,folder,st.st_size>0?uint64_t(st.st_size):0});errno=0;
    }
    closedir(d);
    std::sort(out.begin(),out.end(),[](const Entry&a,const Entry&b){return a.folder!=b.folder?a.folder>b.folder:a.name<b.name;});
    if(!ok)out.clear();return ok;
}
inline const std::vector<std::string>& edition_names(){static const std::vector<std::string> names={"game-data.bin","game-203.oqgame","title-label.rgba","title-menu.pcm","identity-refs.bin","edition.bin"};return names;}
inline bool owned_folder(const std::string& app,const std::string& path){
    for(const char* n:{"active","heartgold","soulsilver","heartgold-r7","soulsilver-r5"})if(path==app+"/games/"+n)return directory(app)&&directory(app+"/games")&&directory(path);
    return false;
}
inline bool removal_size(const std::string& app,const std::string& path,uint64_t& bytes,std::string& error){
    bytes=0;if(!owned_folder(app,path)){error="Kein eigener Importordner";return false;}
    DIR*d=opendir(path.c_str());if(!d){error="Importordner nicht lesbar";return false;}bool ok=true;errno=0;
    while(auto*e=readdir(d)){
        std::string name=e->d_name;if(name=="."||name=="..")continue;struct stat st{};
        if(std::find(edition_names().begin(),edition_names().end(),name)==edition_names().end()||lstat(child(path,name).c_str(),&st)||!S_ISREG(st.st_mode)||st.st_size<0){ok=false;break;}
        bytes+=uint64_t(st.st_size);
    }
    if(errno)ok=false;closedir(d);if(!ok)error="Unerwartete Daten im Importordner - Entfernen gesperrt";return ok;
}
// Deliberately bounded: saves, input ROMs, backups and external originals are never targets.
// Manifest is last so interrupted deletion remains visible for a retry.
inline bool remove_edition(const std::string& app,const std::string& path,const std::string& active,std::string& error){
    if(path==active){error="Aktives Spiel kann nicht entfernt werden";return false;}
    uint64_t ignored=0;if(!removal_size(app,path,ignored,error))return false;
    for(const auto& name:edition_names()){
        auto file=child(path,name);struct stat st{};
        if(lstat(file.c_str(),&st)){if(errno==ENOENT)continue;error="Importdatei nicht lesbar";return false;}
        if(!S_ISREG(st.st_mode)||unlink(file.c_str())){error="Entfernen unvollstaendig - erneut versuchen";return false;}
    }
    if(rmdir(path.c_str())){error="Importordner konnte nicht entfernt werden";return false;}return true;
}
}
