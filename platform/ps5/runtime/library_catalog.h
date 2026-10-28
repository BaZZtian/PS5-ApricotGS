// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "library_files.h"
#include "apricot/edition_bundle.hpp"
namespace apricot::library {
inline std::vector<std::string> edition_folders(const std::string&app){
    std::vector<std::string> result;for(const char*n:{"heartgold-r7","soulsilver-r5","heartgold","soulsilver","active"})result.push_back(app+"/games/"+n);return result;
}
struct GameFile {std::string name,path;uint64_t bytes=0;bool regular=false;};
struct GameRecord {
    std::string title="Unbekannte importierte Edition",code,path,status,remove_error;
    uint64_t bytes=0,rom_bytes=0;bool manifest=false,complete=false,removable=false,ready=false;
    std::vector<GameFile> files;
};
inline std::string size_text(uint64_t bytes){return std::to_string(bytes/(1024*1024))+" MiB ("+std::to_string(bytes)+" Bytes)";}
inline std::vector<GameRecord> catalog(const std::string&app,const std::vector<std::string>&verified={}){
    std::vector<GameRecord> result;
    for(const auto&path:edition_folders(app)){
        if(!owned_folder(app,path))continue;GameRecord game;game.path=path;
        std::vector<uint8_t> data(256);FILE*f=fopen((path+"/edition.bin").c_str(),"rb");
        bool meta=f&&fread(data.data(),1,data.size(),f)==data.size()&&fgetc(f)==EOF;if(f)fclose(f);
        apricot::EditionBundle bundle;game.manifest=meta&&bundle.load(data);
        if(game.manifest){game.code.assign(reinterpret_cast<const char*>(bundle.identity),4);game.rom_bytes=bundle.rom_bytes;
            game.title=bundle.gold()?"Pokemon Goldene Edition HeartGold":"Pokemon Silberne Edition SoulSilver";}
        bool all=true;for(const auto&name:edition_names()){
            GameFile item;item.name=name;item.path=path+"/"+name;struct stat st{};
            item.regular=!lstat(item.path.c_str(),&st)&&S_ISREG(st.st_mode)&&st.st_size>=0;
            if(item.regular){item.bytes=uint64_t(st.st_size);game.bytes+=item.bytes;}else all=false;
            game.files.push_back(std::move(item));
        }
        game.complete=game.manifest&&all;
        uint64_t removal_bytes=0;game.removable=removal_size(app,path,removal_bytes,game.remove_error);
        game.ready=game.complete&&std::find(verified.begin(),verified.end(),path)!=verified.end();
        game.status=!game.complete?"Unvollstaendig / Startpruefung fehlgeschlagen":game.ready?"Startpruefung bestanden":"Startpruefung ausstehend";
        if(!game.removable)game.status+="; Entfernen gesperrt";
        result.push_back(std::move(game));
    }return result;
}
}
