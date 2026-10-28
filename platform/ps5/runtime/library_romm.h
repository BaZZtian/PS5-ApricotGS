// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "library_json.h"
#include "library_files.h"
#include "library_pairing.h"
#include "library_session.h"
#include <memory>
#include <atomic>
#include <functional>
namespace apricot::library {
struct Transfer {
    std::atomic<int> http_status{0};
    std::atomic<bool> cancelled{false};std::atomic<uint64_t> received{0},total{0};
};
using Sink=std::function<bool(const void*,size_t)>;
struct Http {
    virtual ~Http()=default;
    virtual bool request(const char*method,const std::string&url,const std::string&bearer,const std::string&body,uint64_t maximum,Transfer&,const Sink&,std::string&error,const char*content_type="application/x-www-form-urlencoded")=0;
    virtual void abort()=0;
};
struct RemoteRom{uint64_t id=0,bytes=0;std::string filename,title;};
class Romm {
    Http& http;std::string base,token,refresh_token,list_path="/api/roms",filter_key;bool pairing_supported=false,refresh_supported=false,auth_invalid=false;
    std::unique_ptr<RommSessionStore>store;std::string storage_warning;
    bool json(const std::string&path,Json&out,Transfer&t,std::string&error,const char*method="GET",const std::string&body="",const char*content_type="application/x-www-form-urlencoded",bool refresh_allowed=true){
        std::string text;if(!request(method,base+path,body,8*1024*1024,t,[&](const void*p,size_t n){text.append(static_cast<const char*>(p),n);return true;},error,content_type,refresh_allowed)){erase_secret(text);return false;}
        bool parsed=Json::parse(text,out);std::fill(text.begin(),text.end(),'\0');if(!parsed){error="RomM antwortet nicht mit gueltigem JSON";return false;}return true;
    }
    bool request(const char*method,const std::string&url,const std::string&body,uint64_t maximum,Transfer&t,const Sink&sink,std::string&error,const char*content_type="application/x-www-form-urlencoded",bool refresh_allowed=true){
        bool ok=http.request(method,url,token,body,maximum,t,sink,error,content_type);
        if(!ok&&!t.cancelled&&t.http_status==401&&!token.empty()){
            if(refresh_allowed&&refresh_supported&&!refresh_token.empty()){
                if(!renew(t,error))return false;
                ok=http.request(method,url,token,body,maximum,t,sink,error,content_type);
            }
            if(!ok&&t.http_status==401){auth_invalid=true;error="RomM-Anmeldung abgelaufen oder widerrufen. Bitte erneut anmelden oder Geraet koppeln.";}
        }return ok;
    }
    bool renew(Transfer&t,std::string&error){
        std::string body="grant_type=refresh_token&refresh_token="+url_escape(refresh_token);
        std::string old=std::move(token);token.clear();Json result;
        bool ok=json("/api/token",result,t,error,"POST",body,"application/x-www-form-urlencoded",false);erase_secret(body);
        if(!ok){token=std::move(old);if(t.http_status==400||t.http_status==401||t.http_status==403){auth_invalid=true;error="RomM-Anmeldung abgelaufen oder widerrufen. Bitte erneut anmelden oder Geraet koppeln.";}return false;}
        erase_secret(old);token=std::move(result.fields["access_token"].text);erase_secret(refresh_token);refresh_token=std::move(result.fields["refresh_token"].text);
        if(!session_token(token)||!session_token(refresh_token,true)){logout();auth_invalid=true;error="RomM lieferte ungueltige erneuerte Anmeldedaten";return false;}
        auth_invalid=false;if(store)remember(storage_warning);return true;
    }
public:
    explicit Romm(Http&h):http(h){}
    ~Romm(){logout();}
    void logout(){erase_secret(token);erase_secret(refresh_token);auth_invalid=false;}
    bool authenticated()const{return !token.empty();}
    bool invalid_auth()const{return auth_invalid;}
    void use_store(const std::string&app){store=std::make_unique<RommSessionStore>(app);}
    bool saved_server(std::string&url,std::string&error){if(!store)return false;RommSession session;if(!store->load(session,error))return false;url=session.server;return true;}
    bool remember(std::string&error){
        if(!store){error="Privater RomM-Speicher nicht eingerichtet";return false;}RommSession session;session.server=base;session.access=token;session.refresh=refresh_token;return store->save(session,error);
    }
    std::string take_storage_warning(){auto warning=std::move(storage_warning);storage_warning.clear();return warning;}
    bool forget(std::string&error){logout();return !store||store->clear(error);}
    bool resume(Transfer&t,std::string&error){
        if(!store){error="Keine gespeicherte RomM-Verbindung";return false;}RommSession session;if(!store->load(session,error))return false;
        if(!connect(session.server,t,error))return false;token=std::move(session.access);refresh_token=std::move(session.refresh);
        Json result;return json("/api/platforms",result,t,error);
    }
    bool connect(std::string url,Transfer&t,std::string&error){
        logout();pairing_supported=false;refresh_supported=false;if(!base_url(url)){error="Ungueltige Server-URL";return false;}base=std::move(url);
        Json spec;if(!json("/openapi.json",spec,t,error))return false;
        const auto&paths=spec.get("paths");
        const auto&schema=paths.get("/api/token").get("post").get("requestBody").get("content").get("application/x-www-form-urlencoded").get("schema");
        const auto&ref=schema.get("$ref").text;const Json&form=ref.compare(0,21,"#/components/schemas/")==0?spec.get("components").get("schemas").get(ref.substr(21)):schema;
        refresh_supported=form.get("properties").get("grant_type").kind==Json::Object&&form.get("properties").get("refresh_token").kind==Json::Object;
        pairing_supported=paths.get("/api/client-tokens/exchange").get("post").get("requestBody").get("content").get("application/json").kind==Json::Object;
        if(paths.get("/api/roms").kind!=Json::Object&&paths.get("/api/roms/").kind!=Json::Object){error="RomM API nicht kompatibel";return false;}
        if(paths.get("/api/roms/{id}/content/{file_name}").kind!=Json::Object&&paths.get("/api/roms/{rom_id}/content/{file_name}").kind!=Json::Object){error="RomM Download-API nicht kompatibel";return false;}
        list_path=paths.get("/api/roms").kind==Json::Object?"/api/roms":"/api/roms/";filter_key.clear();
        bool limit=false,offset=false;
        for(const auto&p:paths.get(list_path).get("get").get("parameters").items){
            auto name=p.get("name").text;
            if(name=="platform_ids")filter_key=name;else if(name=="platform_id"&&filter_key.empty())filter_key=name;
            if(name=="limit")limit=true;if(name=="offset")offset=true;
        }
        if(filter_key.empty()||!limit||!offset){error="RomM Plattformfilter oder Pagination nicht kompatibel";return false;}
        return true;
    }
    bool pair(const std::string&code,Transfer&t,std::string&error){
        logout();std::string normalized;
        if(!pairing_supported){error="Dieser RomM-Server bietet keinen kompatiblen QR-Verbindungscode an";return false;}
        if(!pairing_code(code,normalized)){error="Verbindungscode: 8 Buchstaben/Ziffern, optional XXXX-XXXX";return false;}
        if(t.cancelled){error="Kopplung abgebrochen";return false;}
        std::string body="{\"code\":\""+normalized+"\"}";Json result;
        bool ok=json("/api/client-tokens/exchange",result,t,error,"POST",body,"application/json");
        std::fill(body.begin(),body.end(),'\0');std::fill(normalized.begin(),normalized.end(),'\0');
        if(!ok){
            if(t.cancelled){error="Kopplung abgebrochen";return false;}
            int status=t.http_status.load();
            if(status==400||status==404||status==410||status==422)error="Code ungueltig, abgelaufen oder bereits benutzt. Im QR-Generator neu erzeugen.";
            else if(status==429)error="Zu viele Versuche. Bitte kurz warten und einen neuen Code erzeugen.";
            else if(status==403)error="RomM verweigert die Kopplung. Konto und Berechtigungen pruefen.";
            return false;
        }
        token=std::move(result.fields["raw_token"].text);
        if(!client_token_format(token)){logout();error="RomM hat kein gueltiges Zugangstoken fuer den Code geliefert";return false;}
        Json platforms;if(!json("/api/platforms",platforms,t,error)){logout();return false;}
        return true;
    }
    bool authenticate(const std::string&user,const std::string&password,const std::string&client_token,Transfer&t,std::string&error){
        logout();
        if(!client_token.empty())token=client_token;
        else {Json result;if(!json("/api/token",result,t,error,"POST","grant_type=password&username="+url_escape(user)+"&password="
            +url_escape(password)+"&scope=roms.read%20platforms.read"))return false;token=std::move(result.fields["access_token"].text);refresh_token=std::move(result.fields["refresh_token"].text);}
        if(!session_token(token)||!session_token(refresh_token,true)){logout();error="Ungueltiges Anmeldetoken";return false;}
        Json platforms;if(!json("/api/platforms",platforms,t,error)){logout();return false;}return true;
    }
    bool platforms(std::vector<uint64_t>&ids,Transfer&t,std::string&error){
        Json result;if(!json("/api/platforms",result,t,error))return false;
        const auto&rows=result.kind==Json::Array?result.items:result.get("items").items;ids.clear();
        for(const auto&p:rows){auto slug=p.get("slug").text;if(slug=="nds"||slug=="nintendo-ds"){auto id=p.get("id").integer();if(id)ids.push_back(id);}}
        if(ids.empty()){error="Keine Nintendo-DS-Plattform in RomM gefunden";return false;}return true;
    }
    bool page(uint64_t platform,uint64_t offset,std::vector<RemoteRom>&roms,bool&more,Transfer&t,std::string&error){
        Json result;if(!json(list_path+"?"+filter_key+"="+std::to_string(platform)+"&limit=50&offset="+std::to_string(offset),result,t,error))return false;
        const Json&array=result.kind==Json::Array?result:result.get("items");if(array.kind!=Json::Array){error="RomM Bibliotheksformat nicht unterstuetzt";return false;}
        roms.clear();more=array.items.size()==50;
        if(result.get("total").kind==Json::Number)more=offset+array.items.size()<result.get("total").integer();
        for(const auto&r:array.items){RemoteRom item;item.id=r.get("id").integer();item.filename=r.get("fs_name").text;item.title=r.get("name").text;item.bytes=r.get("fs_size_bytes").integer();
            auto returned_platform=r.get("platform_id").integer(platform);
            if(returned_platform==platform&&item.id&&plain_name(item.filename)&&nds_name(item.filename)&&item.bytes<=0x20000000)roms.push_back(std::move(item));}
        return true;
    }
    bool download(const RemoteRom&rom,const std::string&file,Transfer&t,std::string&error){
        if(!rom.id||!plain_name(rom.filename)||!nds_name(rom.filename)||rom.bytes>0x20000000){error="Keine einzelne unterstuetzte NDS-Datei";return false;}
        int fd=open(file.c_str(),O_WRONLY|O_CREAT|O_EXCL|O_NOFOLLOW,0600);if(fd<0){error="Download-Zwischenspeicher nicht verfuegbar";return false;}
        uint64_t bytes=0;
        bool ok=request("GET",base+"/api/roms/"+std::to_string(rom.id)+"/content/"+url_escape(rom.filename),"",0x20000000,t,[&](const void*p,size_t n){
            const char*b=static_cast<const char*>(p);size_t at=0;while(at<n){ssize_t done=write(fd,b+at,n-at);if(done<0&&errno==EINTR)continue;if(done<=0)return false;at+=size_t(done);}bytes+=n;return true;
        },error);
        if(ok&&(bytes<1024*1024||(rom.bytes&&bytes!=rom.bytes))){error="NDS-Download hat eine unerwartete Groesse";ok=false;}
        if(ok&&fsync(fd)){error="Download konnte nicht gespeichert werden";ok=false;}
        if(close(fd)&&ok){error="Download konnte nicht geschlossen werden";ok=false;}
        if(t.cancelled){error="Download abgebrochen";ok=false;}if(!ok)unlink(file.c_str());return ok;
    }
};
}
