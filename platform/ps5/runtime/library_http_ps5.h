// SPDX-License-Identifier: GPL-3.0-or-later
// Native HTTP2 API bindings, verified against SDK http2_get (John Tornblom,
// GPL-3.0-or-later) and shadPS4 Http2 declarations (GPL-2.0-or-later).
#pragma once
#include "library_romm.h"
#include "app_version.h"
#include <mutex>
extern "C" {
int sceNetInit();int sceNetPoolCreate(const char*,int,int);int sceNetPoolDestroy(int);
int sceSslInit(size_t);int sceSslTerm(int);
int sceHttp2Init(int,int,uint64_t,int);int sceHttp2Term(int);
int sceHttp2CreateTemplate(int,const char*,int,int);int sceHttp2DeleteTemplate(int);
int sceHttp2CreateRequestWithURL(int,const char*,const char*,uint64_t);int sceHttp2DeleteRequest(int);
int sceHttp2AddRequestHeader(int,const char*,const char*,unsigned);
int sceHttp2SendRequest(int,const void*,uint64_t);int sceHttp2GetStatusCode(int,int*);
int sceHttp2GetResponseContentLength(int,int*,uint64_t*);int sceHttp2ReadData(int,void*,uint64_t);
int sceHttp2AbortRequest(int);
int sceHttp2SetAutoRedirect(int,int);
}
namespace apricot::library {
class Ps5Http final:public Http {
    int pool=-1,ssl=-1,ctx=-1,tmpl=-1,active=-1;std::mutex lock;
    bool init(){
        if(tmpl>=0)return true;
        if(pool>=0)return false;
        // The process owns one HTTP client. Net may already have been initialized by the title.
        sceNetInit();pool=sceNetPoolCreate("ApricotGS-RomM",256*1024,0);if(pool<0)return false;
        ssl=sceSslInit(512*1024);if(ssl<0)return false;ctx=sceHttp2Init(pool,ssl,512*1024,1);if(ctx<0)return false;
        tmpl=sceHttp2CreateTemplate(ctx,"ApricotGS/" APRICOT_VERSION,2,0);
        if(tmpl<0)return false;
        if(sceHttp2SetAutoRedirect(tmpl,0)<0){sceHttp2DeleteTemplate(tmpl);tmpl=-1;return false;}
        return true;
    }
public:
    ~Ps5Http(){if(tmpl>=0)sceHttp2DeleteTemplate(tmpl);if(ctx>=0)sceHttp2Term(ctx);if(ssl>=0)sceSslTerm(ssl);if(pool>=0)sceNetPoolDestroy(pool);}
    void abort()override{std::lock_guard<std::mutex>guard(lock);if(active>=0)sceHttp2AbortRequest(active);}
    bool request(const char*method,const std::string&url,const std::string&bearer,const std::string&body,uint64_t maximum,Transfer&t,const Sink&sink,std::string&error,const char*content_type="application/x-www-form-urlencoded")override{
        error.clear();t.http_status=0;t.received=0;t.total=0;if(t.cancelled){error="Vorgang abgebrochen";return false;}
        if(!init()){error="PS5 Netzwerkdienst konnte nicht starten";return false;}
        int req=sceHttp2CreateRequestWithURL(tmpl,method,url.c_str(),body.size());if(req<0){error="Server-URL konnte nicht geoeffnet werden";return false;}
        {std::lock_guard<std::mutex>guard(lock);active=req;}
        bool ok=true;
        if(!bearer.empty()){std::string header="Bearer "+bearer;ok=sceHttp2AddRequestHeader(req,"Authorization",header.c_str(),0)>=0;std::fill(header.begin(),header.end(),'\0');}
        if(ok&&!body.empty())ok=sceHttp2AddRequestHeader(req,"Content-Type",content_type,0)>=0;
        if(ok&&!t.cancelled)ok=sceHttp2SendRequest(req,body.empty()?nullptr:body.data(),body.size())>=0;else ok=false;
        int status=0;if(ok){ok=sceHttp2GetStatusCode(req,&status)>=0;if(ok)t.http_status=status;}
        if(ok&&status!=200){error=status==401?"Anmeldung ungueltig oder abgelaufen":status==403?"RomM Leseberechtigung fehlt":"RomM HTTP-Fehler "+std::to_string(status);ok=false;}
        // Redirects are not followed: credentials must never leave the chosen server.
        uint64_t length=0;int length_type=0;
        if(ok&&sceHttp2GetResponseContentLength(req,&length_type,&length)>=0&&length_type==0){t.total=length;if(length>maximum){error="Antwort ueberschreitet das Groessenlimit";ok=false;}}
        char block[65536];uint64_t count=0;
        while(ok&&!t.cancelled){int n=sceHttp2ReadData(req,block,sizeof(block));if(n<0){ok=false;break;}if(!n)break;
            if(uint64_t(n)>maximum-count){error="Antwort ueberschreitet das Groessenlimit";ok=false;break;}
            if(!sink(block,size_t(n))){error="Daten konnten nicht gespeichert werden";ok=false;break;}count+=unsigned(n);t.received=count;
        }
        if(t.cancelled){error="Vorgang abgebrochen";ok=false;}
        if(ok&&t.total&&count!=t.total){error="Download unvollstaendig";ok=false;}
        {std::lock_guard<std::mutex>guard(lock);active=-1;sceHttp2DeleteRequest(req);}
        if(!ok&&error.empty())error="Netzwerk-, DNS- oder TLS-Verbindung fehlgeschlagen";return ok;
    }
};
}
