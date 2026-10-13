// Line-oriented protocol parser and formatter.
#include "server/protocol.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cctype>

namespace {
const char *names[VERB_COUNT] = {
 "UNKNOWN","HELLO","LOGIN","LOGOUT","PING","QUIT","WHOAMI","STATS","INVENTORY",
 "SECTOR","SYSTEM","SYSTEMS","UNIVERSE","TRAVEL","ENCOUNTER","FIGHT","FLEE",
 "LOOT","STORY","WHO","ROLL","SEED","MOTD","HELP"
};
const char *errors[ERR_COUNT] = {
 "none","bad syntax","unknown verb","not logged in","already logged in",
 "no such system","no such sector","unreachable","no such table","not in combat",
 "server full","internal error"
};
bool equalNoCase(const char *a,const char *b) {
 if(!a||!b) return false;
 while(*a&&*b) { if(std::toupper((unsigned char)*a)!=std::toupper((unsigned char)*b)) return false; ++a;++b; }
 return *a==*b;
}
}
int protoParse(const char *line, protomessage &out) {
 std::memset(&out,0,sizeof(out)); out.verb=VERB_UNKNOWN;
 if(!line) return 0;
 size_t n=std::strlen(line);
 if(n==0 || n>=sizeof(out.raw)) return 0;
 std::memcpy(out.raw,line,n+1);
 char buffer[PROTO_MAX_LINE]; std::memcpy(buffer,line,n+1);
 char *p=buffer;
 while(*p && std::isspace((unsigned char)*p)) ++p;
 if(!*p) return 0;
 char *save=nullptr; char *tok=strtok_r(p," \t\r\n",&save);
 if(!tok) return 0;
 out.verb=protoVerbFromName(tok);
 while((tok=strtok_r(nullptr," \t\r\n",&save))!=nullptr) {
   if(out.argCount>=PROTO_MAX_ARGS) return 0;
   if(std::strlen(tok)>=sizeof(out.args[0])) return 0;
   std::strcpy(out.args[out.argCount++],tok);
 }
 return out.verb!=VERB_UNKNOWN;
}
const char *protoVerbName(protocolverb verb) {
 return verb>=0 && verb<VERB_COUNT ? names[verb] : "UNKNOWN";
}
protocolverb protoVerbFromName(const char *name) {
 for(int i=1;i<VERB_COUNT;++i) if(equalNoCase(name,names[i])) return (protocolverb)i;
 return VERB_UNKNOWN;
}
const char *protoErrorText(int code) {
 return code>=0 && code<ERR_COUNT ? errors[code] : "unknown error";
}
void protoOk(char out[],int outSize,protocolverb verb,const char *payload) {
 if(!out||outSize<=0) return;
 std::snprintf(out,(size_t)outSize,"OK %s%s%s\n",protoVerbName(verb),
   payload&&*payload?" ":"",payload?payload:"");
}
void protoErr(char out[],int outSize,int code) {
 if(!out||outSize<=0) return;
 std::snprintf(out,(size_t)outSize,"ERR %d %s\n",code,protoErrorText(code));
}
int protoArgInt(const protomessage &msg,int index,int fallback) {
 if(index<0||index>=msg.argCount) return fallback;
 char *end=nullptr; long value=std::strtol(msg.args[index],&end,10);
 if(end==msg.args[index]||*end!='\0'||value<(-2147483647L-1)||value>2147483647L) return fallback;
 return (int)value;
}
const char *protoArgText(const protomessage &msg,int index) {
 return index>=0&&index<msg.argCount?msg.args[index]:"";
}
int protoRequiresLogin(protocolverb verb) {
 switch(verb) {
 case VERB_WHOAMI: case VERB_STATS: case VERB_INVENTORY: case VERB_SECTOR:
 case VERB_SYSTEM: case VERB_SYSTEMS: case VERB_UNIVERSE: case VERB_TRAVEL:
 case VERB_ENCOUNTER: case VERB_FIGHT: case VERB_FLEE: case VERB_LOOT:
 case VERB_STORY: return 1;
 default: return 0;
 }
}
