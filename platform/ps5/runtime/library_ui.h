// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "library_http_ps5.h"
#include "library_input.h"
#include "library_keyboard.h"
#include "library_catalog.h"
#include "library_presentation.h"
#include "library_defaults.h"
#include "apricot/controller_prompts.hpp"
#include "app_version.h"
#include "../src/display.h"
#include "../src/platform.h"
#include "../src/menu.h"
#include <thread>
namespace apricot::library {
class LibraryUi {
#ifdef APRICOT_LIBRARY_UI_TEST
    friend struct LibraryUiTestAccess;
#endif
    Ps5GpuDisplay& display;std::vector<uint32_t>&canvas;std::string app;
    Ps5Http http;Romm romm{http};std::string server=default_romm_url,root="/data/apricotgs-roms";
    pad raw{};MenuInput navigation;std::vector<std::string> playable;
    std::vector<uint8_t> background;std::vector<uint32_t> base;bool background_attempted=false;
    static std::string visible(const std::string&s,size_t max=90){std::string out;for(unsigned char c:s){if(out.size()==max)break;out+=c>=32&&c<127?char(c):'?';}return out;}
    static constexpr uint32_t ink=0xff193341,muted=0xff193341,paper=0xfff0f4f5,focus=0xff214e60;
    static std::string fitted(std::string text,int width,int scale){
        if(menuTextWidth(text,scale)<=width)return text;
        while(!text.empty()&&menuTextWidth(text+"...",scale)>width)text.pop_back();return text+"...";
    }
    void line(int x,int y,const std::string&s,uint32_t color=ink,int scale=3,int width=1100){
        menuProportionalText(canvas,x,y,fitted(visible(s,2048),std::min(width,1224-x),scale),color,scale,1280);
    }
    void centered(int x,int y,int width,const std::string&text,uint32_t color,int scale){
        line(x+(width-menuTextWidth(text,scale))/2,y,text,color,scale,width);
    }
    void box(int x,int y,int width,int height,uint32_t color,int radius=10){
        for(int b=0;b<height;++b)for(int a=0;a<width;++a){
            int dx=std::max({radius-a,0,a-(width-radius-1)}),dy=std::max({radius-b,0,b-(height-radius-1)});
            if(dx*dx+dy*dy<=radius*radius&&x+a>=0&&x+a<1280&&y+b>=0&&y+b<720)canvas[(y+b)*1280+x+a]=color;
        }
    }
    void glaze(int x,int y,int width,int height,uint32_t tint,unsigned alpha,int radius=10){
        for(int b=0;b<height;++b)for(int a=0;a<width;++a){int dx=std::max({radius-a,0,a-(width-radius-1)}),dy=std::max({radius-b,0,b-(height-radius-1)});if(dx*dx+dy*dy>radius*radius)continue;
            auto&pixel=canvas[(y+b)*1280+x+a];uint32_t out=0xff000000u;for(unsigned c=0;c<3;++c){unsigned front=(tint>>(8*c))&255,back=(pixel>>(8*c))&255;unsigned value=front*alpha+back*(255-alpha)+127;out|=(value/255)<<(8*c);}pixel=out;}
    }
    void button(int x,int y,uint32_t key,const std::string&label){
        using I=apricot::ControlIcon;I icon=key==PAD_CROSS?I::Cross:key==PAD_CIRCLE?I::Circle:key==PAD_SQUARE?I::Square:key==PAD_TRIANGLE?I::Triangle:key==PAD_L1?I::L1:key==PAD_R1?I::R1:I::Dpad;
        apricot::controller_icon(canvas,1280,720,x,y,icon,true);line(x+48,y+8,label,ink,3,240);
    }
    void paragraph(int x,int y,const std::string&text,int width,int maximum=2){
        std::string rest=visible(text,2048);
        for(int row=0;row<maximum&&!rest.empty();++row){
            if(row+1==maximum){line(x,y+row*28,rest,muted,3,width);break;}
            size_t count=0;while(count<rest.size()&&menuTextWidth(rest.substr(0,count+1),3)<=width)++count;
            size_t split=count;if(count<rest.size()){auto space=rest.rfind(' ',count);if(space!=std::string::npos&&space>count/2)split=space;}
            line(x,y+row*28,rest.substr(0,split),muted,3,width);rest.erase(0,split);while(!rest.empty()&&rest.front()==' ')rest.erase(0,1);
        }
    }
    pad poll(){pad_poll(&raw);pad event=raw;auto action=navigation.update(raw.held,raw.left_x,raw.left_y,now_seconds());event.pressed=action.pressed;return event;}
    void prepare_background(){
        if(!background_attempted){background_attempted=true;FILE*f=fopen("/app0/userdata/launcher-uhd.rgba","rb");
            if(f){background.resize(3840u*2160*4);bool ok=fread(background.data(),1,background.size(),f)==background.size()&&fgetc(f)==EOF;fclose(f);if(!ok)background.clear();}
            library_background(base,background);
        }
        if(!background.empty()&&display.native_background_rgba.empty())display.set_background(background);
    }
    void frame(const std::string&title,const std::string&detail){
        prepare_background();canvas=base;
        line(80,38,"ApricotGS",ink,4,500);line(998,40,"v" APRICOT_VERSION,ink,3,204);
        line(88,116,title,ink,4,1104);paragraph(88,164,detail,1104);
    }
    bool present(){bool ok=!display.present(canvas.data(),1280,720,1280*4,stderr,nullptr,true);usleep(16000);return ok;}
    int menu(const std::string&title,const std::string&detail,const std::vector<std::string>&rows,const std::vector<std::vector<std::string>>&details={}){
        navigation.enter();size_t selected=0;
        for(;;){pad p=poll();if(p.pressed&(PAD_CIRCLE|PAD_OPTIONS))return -1;
            if(!rows.empty()){
                if((p.pressed&PAD_UP)&&selected) --selected;
                if((p.pressed&PAD_DOWN)&&selected+1<rows.size())++selected;
                if(p.pressed&PAD_CROSS)return int(selected);
            }
            frame(title,detail);size_t first=(selected/6)*6;
            for(size_t i=first;i<rows.size()&&i<first+6;++i){
                int y=library_rows_top+int(i-first)*library_row_step;
                if(i==selected){box(80,y-8,1120,44,focus);box(80,y-8,5,44,0xffefbb77,2);}
                line(100,y,rows[i],i==selected?0xfffafcfd:ink,3,1072);
            }
            if(selected<details.size())for(size_t i=0;i<details[selected].size()&&i<2;++i)line(88,538+int(i)*30,details[selected][i],muted,3,1104);
            box(88,612,1104,1,0xff587c8c,0);
            line(88,640,rows.empty()?"Keine Eintraege":std::to_string(selected+1)+" / "+std::to_string(rows.size()),muted,3,170);
            button(390,628,PAD_UP,"Auswahl");button(668,628,PAD_CROSS,"Waehlen");button(948,628,PAD_CIRCLE,"Zurueck");if(!present())return -1;
        }
    }
    void details_page(const std::string&title,const std::vector<std::string>&lines){
        std::vector<std::string> rows;
        for(const auto&line:lines){if(line.empty())rows.push_back(" ");else {
            std::string rest=visible(line,2048);while(!rest.empty()){
                size_t n=0;while(n<rest.size()&&menuTextWidth(rest.substr(0,n+1),3)<=1050)++n;
                size_t cut=n;if(n<rest.size()){auto space=rest.rfind(' ',n);if(space!=std::string::npos&&space>n/2)cut=space;}
                rows.push_back(rest.substr(0,cut));rest.erase(0,cut);while(!rest.empty()&&rest.front()==' ')rest.erase(0,1);
            }}}
        menu(title,"Details",rows);
    }
    bool input(const std::string&title,std::string&value,bool secret=false,bool uppercase=false,size_t maximum=1024){
        navigation.enter();TextKeyboard keyboard(uppercase);std::string edited=value;
        for(;;){pad p=poll();if(p.pressed&(PAD_CIRCLE|PAD_OPTIONS)){std::fill(edited.begin(),edited.end(),'\0');return false;}
            if(p.pressed&PAD_TRIANGLE){value=std::move(edited);return true;}
            keyboard.apply(p.pressed,edited,maximum);
            frame(title,"");
            glaze(88,174,1104,74,0xff517f96,70);glaze(90,176,1100,70,0xfff4f9fb,90);
            auto field=keyboard_field(edited,secret);bool scrolled=false;
            while(!field.empty()&&menuTextWidth("..."+field+"|",4)>1056){field.erase(0,1);scrolled=true;}
            line(112,194,(scrolled?"...":"")+field+"|",ink,4,1056);
            const char* modes[]={"abc","ABC","#+="};
            for(unsigned i=0;i<3;++i){int x=88+int(i)*102;bool active=keyboard.page==i;
                if(active)box(x,266,92,42,focus);else glaze(x,266,92,42,0xfff4f9fb,48);centered(x,275,92,modes[i],active?0xfffafcfd:muted,3);
                if(active)box(x+20,302,52,3,0xffefbb77,1);
            }
            button(468,266,PAD_L1,"Zurueck");button(706,266,PAD_R1,"Weiter");
            line(948,276,keyboard.page==0?"Kleinbuchstaben":keyboard.page==1?"Grossbuchstaben":"Sonderzeichen",muted,3,244);
            unsigned size=unsigned(keyboard.characters().size()+1);
            for(unsigned i=0;i<size;++i){int x=112+int(i%12)*88,y=330+int(i/12)*62;bool selected=i==keyboard.index;
                if(selected){box(x-3,y-3,82,60,0xffefbb77);box(x,y,76,54,focus);box(x+12,y+3,52,2,0xffa6c3cb,1);}
                else glaze(x,y,76,54,0xfff4f9fb,64);
                auto label=keyboard.key_label(i);int scale=label.size()>1?3:selected?5:4;
                centered(x,y+(54-8*scale)/2,76,label,selected?0xfffafcfd:ink,scale);
            }
            box(88,606,1104,1,0xff587c8c,0);
            button(88,628,PAD_CROSS,"Zeichen");button(368,628,PAD_TRIANGLE,"Fertig");
            button(648,628,PAD_SQUARE,"Loeschen");button(928,628,PAD_CIRCLE,"Zurueck");if(!present())return false;
        }
    }
    void message(const std::string&text){menu("HINWEIS",text,{"Weiter"});}
    template<class Work>bool wait(const std::string&title,Transfer&t,std::string&error,Work work,bool show_bytes=true,bool cancellable=true){
        navigation.enter();bool result=false,abort_sent=false;std::atomic<bool>done{false};std::thread worker([&]{result=work();done=true;});
        while(!done){pad p=poll();if(cancellable&&(p.pressed&(PAD_CIRCLE|PAD_OPTIONS))&&!abort_sent){t.cancelled=true;http.abort();abort_sent=true;}
            frame(title,abort_sent?"Vorgang wird abgebrochen":cancellable?"Bitte warten...":"Vorhandene Editionen werden geprueft...");uint64_t received=t.received.load(),total=t.total.load();
            std::string text=std::to_string(received/1024)+" KiB";
            if(total)text+=" / "+std::to_string(total/1024)+" KiB ("+std::to_string(received*100/total)+"%)";
            line(88,270,show_bytes?text:"Daten werden gelesen und geprueft...",ink,4);if(cancellable)button(928,628,PAD_CIRCLE,"Abbrechen");if(!present()){t.cancelled=true;http.abort();abort_sent=true;}
        }
        worker.join();if(!result)message(error);return result;
    }
    std::string browse(){
        for(;;){
            int which=menu("LOKALE QUELLE","Speicherort waehlen - einzelne .nds-Dateien",{"ROM-Eingangsordner /data/apricotgs-roms","App-Eingangsordner /app0/roms","USB 0 /mnt/usb0","USB 1 /mnt/usb1","Anderen absoluten Pfad eingeben"});
            if(which<0)return {};
            if(which==0){root="/data/apricotgs-roms";if(!mkdir(root.c_str(),0777)){if(chmod(root.c_str(),0777)){message("ROM-Eingangsordner nicht verfuegbar");continue;}}else if(errno!=EEXIST){message("ROM-Eingangsordner nicht verfuegbar");continue;}}
            else if(which==1)root="/app0/roms";else if(which==2)root="/mnt/usb0";else if(which==3)root="/mnt/usb1";else if(!input("SPEICHERWURZEL",root))continue;
            while(root.size()>1&&root.back()=='/')root.pop_back();std::string path=root;
            for(;;){
                std::vector<Entry>entries;std::string error;Transfer scan;
                if(!wait("ORDNER LESEN",scan,error,[&]{return list(root,path,entries,error,&scan.cancelled);},false))break;
                std::vector<std::string>rows={path==root?"[..] Speicherort wechseln":"[..] Uebergeordneter Ordner"};
                std::vector<std::vector<std::string>> details={{path,"Ordner und einzelne .nds-Dateien"}};
                for(const auto&e:entries){rows.push_back((e.folder?"[ORDNER] ":"[NDS] ")+e.name);details.push_back({e.path,e.folder?"Ordner oeffnen":"NDS-Kandidat - "+size_text(e.bytes)});}
                if(entries.empty())details[0][1]="Keine passenden .nds-Dateien oder Unterordner an diesem Ort";
                int selected=menu("LOKALE SPIELDATEIEN",path,rows,details);if(selected<0)break;
                if(!selected){if(path==root)break;path=path.substr(0,path.rfind('/'));if(path.empty())path="/";continue;}
                const auto&e=entries[size_t(selected-1)];if(e.folder){path=e.path;continue;}
                int action=menu("SPIEL IMPORTIEREN",e.name,{"Pruefen und importieren","Dateipfad und Groesse anzeigen","Abbrechen"},{{e.path,size_text(e.bytes)},{e.path,size_text(e.bytes)}});
                if(action==0)return e.path;
                if(action==1)details_page("NDS-DATEI",{e.name,e.path,size_text(e.bytes),"Unterstuetzung wird beim Import nach voller ROM-Identitaet geprueft"});
            }
        }
    }
    bool sign_in(){
        if(!input("ROMM SERVER-URL",server))return false;
        if(!base_url(server)){message("Ungueltige Server-URL");return false;}
        if(server.compare(0,7,"http://")==0&&menu("UNVERSCHLUESSELTE VERBINDUNG","HTTP uebertraegt Anmeldung und Spieldaten unverschluesselt",{"Abbrechen","Mit diesem Server verbinden"})!=1)return false;
        {Transfer t;std::string error;if(!wait("ROMM VERBINDEN",t,error,[&]{return romm.connect(server,t,error);}))return false;}
        int mode=menu("ROMM ANMELDUNG","Anmeldeart waehlen",{"Benutzername und Passwort","Geraet koppeln (8-Zeichen-Code)","Hilfe: Geraet koppeln"});if(mode<0)return false;
        if(mode==2){details_page("QR-VERBINDUNGSCODE",{"Im RomM-Webinterface: Einstellungen / Client API Tokens.","Eigenen ApricotGS-Zugang waehlen, QR-Code / Geraet koppeln.","Den angezeigten kurzen Code eingeben, z.B. ABCD-2345.","Kurz gueltig und einmal verwendbar; bei Fehler neu erzeugen.","Der lange API-Token muss nicht eingegeben werden."});return false;}
        std::string error;
        if(mode==1){
            for(;;){std::string code;if(!input("GERAET KOPPELN (8 ZEICHEN)",code,false,true,32))return false;
                Transfer pairing;bool paired=wait("ROMM KOPPELN",pairing,error,[&]{return romm.pair(code,pairing,error);});
                std::fill(code.begin(),code.end(),'\0');if(paired){message("RomM verbunden - Bibliothek wird geladen");break;}
                if(menu("VERBINDUNGSCODE ERNEUERN","Neuen Code im RomM-QR-Generator erzeugen",{"Neuen Code eingeben","Zurueck"})!=0)return false;
                error.clear();
            }
        }else {
            std::string user,password;if(!input("BENUTZERNAME",user)||!input("PASSWORT",password,true))return false;
            Transfer auth;bool signed_in=wait("ROMM ANMELDEN",auth,error,[&]{return romm.authenticate(user,password,"",auth,error);});
            std::fill(password.begin(),password.end(),'\0');if(!signed_in)return false;
        }
        if(!romm.remember(error))message(error);
        return true;
    }
    bool connect_remote(){
        for(;;){std::string error,saved;bool has_saved=romm.saved_server(saved,error);
            if(has_saved||romm.authenticated()){
                if(has_saved)server=saved;
                int action=menu("ROMM VERBINDUNG",server,{"Bibliothek oeffnen","Erneut anmelden / Geraet koppeln","Anderen Server verbinden","Abmelden und Verbindung loeschen","Zurueck"});
                if(action<0||action==4)return false;
                if(action==3){if(menu("ROMM ABMELDEN","Loescht nur den gespeicherten Zugang dieser App. Serverdateien und Spielstaende bleiben erhalten.",{"Abbrechen","Abmelden und Verbindung loeschen"})==1){if(!romm.forget(error))message(error);else {message("RomM-Verbindung geloescht");return false;}}continue;}
                if(action==0){
                    if(romm.authenticated())return true;
                    Transfer t;if(wait("ROMM VERBINDUNG WIEDERHERSTELLEN",t,error,[&]{return romm.resume(t,error);},false))return true;
                    if(!recover_auth())return false;continue;
                }
                if(action==2)server="https://";
                return sign_in();
            }
            if(!error.empty()){
                int action=menu("ROMM VERBINDUNG",error,{"Neu anmelden","Gespeicherte Verbindung loeschen","Zurueck"});
                if(action<0||action==2)return false;if(action==1){if(!romm.forget(error))message(error);continue;}
            }
            return sign_in();
        }
    }
    bool recover_auth(){
        if(!romm.invalid_auth())return false;
        if(menu("ROMM ERNEUT ANMELDEN","Der gespeicherte Zugang ist abgelaufen oder wurde widerrufen.",{"Erneut anmelden / Geraet koppeln","Zurueck"})!=0)return false;
        std::string error;if(!romm.forget(error)){message(error);return false;}return true;
    }
    std::string remote(){
      for(;;){
        if(!connect_remote())return {};
        std::string error;std::vector<uint64_t>platforms;{Transfer t;if(!wait("NDS BIBLIOTHEK LADEN",t,error,[&]{return romm.platforms(platforms,t,error);})) {if(recover_auth())continue;return {};}}
        size_t platform=0;uint64_t offset=0;bool reconnect=false;
        for(;;){std::vector<RemoteRom>rows;bool more=false;Transfer t;
            if(!wait("ROMM SPIELE LADEN",t,error,[&]{return romm.page(platforms[platform],offset,rows,more,t,error);})) {reconnect=recover_auth();if(reconnect)break;return {};}
            auto storage_warning=romm.take_storage_warning();if(!storage_warning.empty())message(storage_warning);
            std::vector<std::string>labels;for(const auto&r:rows)labels.push_back(r.title.empty()?r.filename:r.title);
            size_t next=labels.size();if(more)labels.push_back("Naechste Seite");size_t previous=labels.size();if(offset)labels.push_back("Vorherige Seite");size_t other=labels.size();if(platforms.size()>1)labels.push_back("Naechste NDS-Plattform");
            if(labels.empty())labels.push_back("Keine einzelnen NDS-Dateien - Zurueck");
            int selected=menu("ROMM NDS BIBLIOTHEK","Unterstuetzung wird vor dem Import vollstaendig geprueft",labels);if(selected<0)return {};
            if(more&&size_t(selected)==next){offset+=50;continue;}if(offset&&size_t(selected)==previous){offset-=50;continue;}
            if(platforms.size()>1&&size_t(selected)==other){platform=(platform+1)%platforms.size();offset=0;continue;}
            if(size_t(selected)>=rows.size())return {};const auto&r=rows[size_t(selected)];
            if(menu("SPIEL HERUNTERLADEN",r.filename+" - "+std::to_string(r.bytes/(1024*1024))+" MiB",{"Herunterladen und pruefen","Abbrechen"})!=0)continue;
            std::string pattern=app+"/download-XXXXXX";std::vector<char>tmp(pattern.begin(),pattern.end());tmp.push_back(0);int fd=mkstemp(tmp.data());
            if(fd<0){message("Kein Download-Zwischenspeicher verfuegbar");return {};}close(fd);unlink(tmp.data());
            std::string path=tmp.data();Transfer download;
            if(wait("ROMM DOWNLOAD",download,error,[&]{return romm.download(r,path,download,error);}))return path;
            if(romm.invalid_auth()){reconnect=recover_auth();break;}
        }
        if(!reconnect)return {};
      }
    }
    bool can_play()const{
        for(const auto&path:playable)if(owned_folder(app,path)&&!access((path+"/edition.bin").c_str(),R_OK))return true;return false;
    }
    void manage(){
        for(;;){auto games=catalog(app,playable);std::vector<std::string>labels;std::vector<std::vector<std::string>> details;
            for(const auto&game:games){labels.push_back(game.title+" ["+game.code+"] - "+std::to_string(game.bytes/(1024*1024))+" MiB "+(game.ready?"[startbereit]":"[pruefen]"));details.push_back({game.path,game.status});}
            if(games.empty()){message("Keine importierten Editionen vorhanden");return;}
            int selected=menu("IMPORTIERTE SPIELE VERWALTEN","Import waehlen, um Dateien und Aktionen anzuzeigen",labels,details);if(selected<0)return;
            const auto&game=games[size_t(selected)];
            int action=menu(game.title,"Importdaten: "+size_text(game.bytes),{"Dateien, Pfad und Status anzeigen",game.removable?"Import entfernen - Spielstand behalten":"Entfernen gesperrt - Grund anzeigen","Zurueck"},{{game.path,game.status},{game.path,game.removable?"Originale und Spielstaende bleiben erhalten":game.remove_error}});
            if(action<0||action==2)continue;
            if(action==0){
                std::vector<std::string> lines={game.title+" ["+game.code+"]",game.status,"Importordner: "+game.path,"Gesamte Vorbereitung: "+size_text(game.bytes),"NDS-Kopie: game-data.bin - "+size_text(game.rom_bytes),"Urspruenglicher NDS-Dateiname: nicht gespeichert"};
                for(const auto&file:game.files)lines.push_back(file.name+" - "+(file.regular?size_text(file.bytes):"FEHLT / kein regulaeres Dateiziel"));
                if(!game.removable)lines.push_back(game.remove_error);details_page("IMPORTDATEN UND DATEIEN",lines);continue;
            }
            if(!game.removable){message(game.remove_error);continue;}
            if(menu("ENTFERNEN BESTAETIGEN",game.title,{"Abbrechen","Import entfernen - Spielstand behalten"},{{game.path,size_text(game.bytes)},{game.path,"Saves, Originaldateien und Backups bleiben erhalten"}})!=1)continue;
            std::string error;if(!remove_edition(app,game.path,"",error))message(error);
            else {playable.erase(std::remove(playable.begin(),playable.end(),game.path),playable.end());message("Import entfernt - Spielstand erhalten");}
        }
    }
public:
    LibraryUi(Ps5GpuDisplay&d,std::vector<uint32_t>&c,std::string data):display(d),canvas(c),app(std::move(data)){romm.use_store(app);}
    void set_playable(std::vector<std::string> paths){playable=std::move(paths);}
    // Empty input means use installed editions. Only the explicit exit item leaves the app.
    bool choose(std::string&input_path,bool&temporary){
        input_path.clear();temporary=false;prepare_background();
        for(;;){int selected=menu("SPIELBIBLIOTHEK","Quelle waehlen oder importierte Edition starten",{"Importierte Edition spielen","Lokal: Spieldatei auswaehlen","RomM: Bibliothek verbinden","Importierte Spiele verwalten","Beenden"});
            if(selected==4)return false;
            if(selected<0){if(can_play())return true;continue;}
            if(selected==0){if(can_play())return true;message("Keine startbereite Edition - eigene NDS ueber Lokal oder RomM importieren");continue;}
            if(selected==3){manage();if(can_play())return true;continue;}
            if(selected==1)input_path=browse();else {input_path=remote();temporary=!input_path.empty();}
            if(!input_path.empty())return true;
        }
    }
    void notice(const std::string&text){message(text);}
    template<class Work>void inspect_editions(Work work){Transfer t;std::string error;wait("SPIELEBIBLIOTHEK PRUEFEN",t,error,[&]{work();return true;},false,false);}
    bool verify_source(const std::string&path){
        Transfer t;std::string error;
        return wait("ROM IDENTITAET UND PROFIL PRUEFEN",t,error,[&]{
            apricot::aot::PreparedRom checked;
            bool ok=apricot::aot::prepare_rom_file(path.c_str(),checked,error);
            if(t.cancelled){error="Pruefung abgebrochen";return false;}return ok;
        });
    }
};
}
