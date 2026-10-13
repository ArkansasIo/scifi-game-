#include "server/protocol.h"
#include <cassert>
#include <cstring>
int main() {
  protomessage m{};
  assert(protoParse("PING",m)==1);
  assert(m.verb==VERB_PING && m.argCount==0);
  assert(protoParse("login Pilot_7",m)==1);
  assert(m.verb==VERB_LOGIN && std::strcmp(protoArgText(m,0),"Pilot_7")==0);
  assert(protoArgInt(m,0,-1)==-1);
  assert(protoParse("NOT_A_VERB",m)==0);
  assert(protoParse("",m)==0);
  char out[128]{};
  protoOk(out,sizeof(out),VERB_PING,"pong");
  assert(std::strncmp(out,"OK PING pong",12)==0);
  protoErr(out,sizeof(out),ERR_NOT_LOGGED_IN);
  assert(std::strncmp(out,"ERR 3",5)==0);
  assert(protoRequiresLogin(VERB_TRAVEL)==1);
  assert(protoRequiresLogin(VERB_HELLO)==0);
  return 0;
}
