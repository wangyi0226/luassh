#ifndef SSH_COMMON_H_
#define SSH_COMMON_H_
#include <lua.h>
#include <lauxlib.h>
#include "libssh/libssh.h"
struct session_ud{
	ssh_session session;
};

struct ssh_kv{
    const char *key;
	const int   value;
};
const int get_valbyindex(lua_State *L,const struct ssh_kv *tbl,const int index);
int lscp_new(lua_State *L);
int lsftp_new(lua_State *L);
int lchannel_new(lua_State *L);
ssh_session  get_session(lua_State *L);

#endif /*SSH_COMMON_H_ */

