// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "library_files.h"
#include "library_json.h"
namespace apricot::library {
inline void erase_secret(std::string&value){std::fill(value.begin(),value.end(),'\0');value.clear();}
inline bool session_token(const std::string&value,bool optional=false){
    if(value.empty())return optional;if(value.size()>8192)return false;
    for(unsigned char c:value)if(c<=32||c>=127)return false;return true;
}
struct RommSession {
    std::string server,access,refresh;
    ~RommSession(){clear();}
    void clear(){server.clear();erase_secret(access);erase_secret(refresh);}
    bool valid()const{std::string url=server;return base_url(url)&&url==server&&session_token(access)&&session_token(refresh,true);}
};
// Private app data, never /app0, package files, support reports or saves.
// Restrictive POSIX permissions, bounded records, no links, atomic replacement.
// This is not encryption against a privileged console/file-manager process.
class RommSessionStore {
    std::string root;
    int directory_fd(bool create,std::string&error)const{
        if(!directory(root)){error="Privater RomM-Konfigurationspfad nicht verfuegbar";return -1;}
        int app=open(root.c_str(),O_RDONLY|O_DIRECTORY|O_NOFOLLOW);if(app<0){error="Privater RomM-Konfigurationspfad nicht lesbar";return -1;}
        if(create&&mkdirat(app,"private-romm",0700)&&errno!=EEXIST){close(app);error="Privater RomM-Konfigurationsordner konnte nicht erstellt werden";return -1;}
        int fd=openat(app,"private-romm",O_RDONLY|O_DIRECTORY|O_NOFOLLOW);int why=errno;close(app);
        if(fd<0){if(create||why!=ENOENT)error="Privater RomM-Konfigurationsordner nicht lesbar";return -1;}
        struct stat st{};if(fstat(fd,&st)||!S_ISDIR(st.st_mode)||(st.st_mode&0077)){close(fd);error="RomM-Konfiguration hat keine privaten Zugriffsrechte";return -1;}return fd;
    }
public:
    explicit RommSessionStore(std::string app):root(std::move(app)){}
    bool load(RommSession&session,std::string&error)const{
        error.clear();session.clear();int dir=directory_fd(false,error);if(dir<0)return false;
        int fd=openat(dir,"session",O_RDONLY|O_NOFOLLOW|O_NONBLOCK);int why=errno;close(dir);
        if(fd<0){if(why!=ENOENT)error="Gespeicherte RomM-Verbindung nicht lesbar";return false;}
        struct stat st{};bool ok=!fstat(fd,&st)&&S_ISREG(st.st_mode)&&!(st.st_mode&0077)&&st.st_nlink==1&&st.st_size>0&&st.st_size<=18000;
        std::string data;char buf[1024];
        while(ok){ssize_t n=read(fd,buf,sizeof(buf));if(n<0&&errno==EINTR)continue;if(n<0){ok=false;break;}if(!n)break;if(data.size()+size_t(n)>18000){ok=false;break;}data.append(buf,size_t(n));}
        close(fd);std::fill(std::begin(buf),std::end(buf),0);
        if(ok){size_t at=0;auto next=[&](std::string&value){auto end=data.find('\n',at);if(end==std::string::npos)return false;value=data.substr(at,end-at);at=end+1;return true;};
            std::string magic;ok=next(magic)&&magic=="APRICOT-ROMM-1"&&next(session.server)&&next(session.access)&&next(session.refresh)&&at==data.size()&&session.valid();}
        erase_secret(data);if(!ok){session.clear();error="Gespeicherte RomM-Verbindung beschaedigt oder nicht privat; neu anmelden oder Verbindung loeschen";}return ok;
    }
    bool save(const RommSession&session,std::string&error)const{
        error.clear();if(!session.valid()){error="RomM-Verbindungsdaten ungueltig";return false;}
        int dir=directory_fd(true,error);if(dir<0)return false;
        // Exclusive temporary file, bounded retry; unrelated existing files are never overwritten.
        static std::atomic<unsigned>serial{0};std::string temp;int fd=-1;
        for(unsigned attempt=0;attempt<16;++attempt){temp=".pending-"+std::to_string(getpid())+"-"+std::to_string(++serial);fd=openat(dir,temp.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd>=0||errno!=EEXIST)break;}
        if(fd<0){close(dir);error="RomM-Verbindung konnte nicht privat gespeichert werden";return false;}
        std::string data="APRICOT-ROMM-1\n"+session.server+"\n"+session.access+"\n"+session.refresh+"\n";bool ok=true;size_t at=0;
        struct stat st{};if(fstat(fd,&st)||(st.st_mode&0077))ok=false;
        while(ok&&at<data.size()){ssize_t n=write(fd,data.data()+at,data.size()-at);if(n<0&&errno==EINTR)continue;if(n<=0){ok=false;break;}at+=size_t(n);}
        erase_secret(data);if(ok&&fsync(fd))ok=false;if(close(fd))ok=false;
        if(ok&&renameat(dir,temp.c_str(),dir,"session"))ok=false;
        if(!ok)unlinkat(dir,temp.c_str(),0);close(dir);
        if(!ok)error="RomM-Verbindung konnte nicht dauerhaft gespeichert werden; nur diese Sitzung verbunden";return ok;
    }
    bool clear(std::string&error)const{
        error.clear();int dir=directory_fd(false,error);if(dir<0)return error.empty();
        bool ok=!unlinkat(dir,"session",0)||errno==ENOENT;close(dir);if(!ok)error="Gespeicherte RomM-Verbindung konnte nicht geloescht werden";return ok;
    }
};
}
