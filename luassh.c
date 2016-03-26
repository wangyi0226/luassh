#include <string.h>
#include "ssh_common.h"

static const int get_valbykey(lua_State *L,const struct ssh_kv tbl[],const char* key){
	struct ssh_kv const *kv=&tbl[0];
	while(kv->key){
		if(strcmp(key,kv->key)==0){
			return kv->value;
		}
		++kv;
	}
	return luaL_error(L,"not find '%s'", key);
}

const int get_valbyindex(lua_State *L,const struct ssh_kv tbl[],const int index){
	int flag =0;
	int i=0;
	if(lua_istable(L,index)){
		int size = lua_rawlen(L,index);
		for(i=1;i<=size;i++){
			lua_rawgeti(L,index,i);
			flag=flag|get_valbykey(L,tbl,luaL_checkstring(L,-1));
		}
	}else if(lua_isstring(L,3)){
		flag=get_valbykey(L,tbl,luaL_checkstring(L, 3));
	}
	return flag;
}

ssh_session  get_session(lua_State *L){
	struct session_ud * sessionud = lua_touserdata(L, 1);
	if (sessionud == NULL || sessionud->session == NULL)
		luaL_error(L, "session %p already closed", sessionud);
	return sessionud->session;
}
static void set_options(lua_State *L,ssh_session session,int opt,const void *value ){
	if(ssh_options_set(session,opt,value) < 0) {
		char err[1024];
		snprintf(err,sizeof(err),"%s",ssh_get_error(session));
		ssh_free(session);
		luaL_error(L,err);
	}
}

static int 
ldisconnect(lua_State *L) {
	struct session_ud * sessionud = lua_touserdata(L, 1);
	if (sessionud == NULL || sessionud->session == NULL)
		return 0;
	ssh_disconnect(sessionud->session);
	ssh_free(sessionud->session);
	sessionud->session=NULL;
	return 0;
}

static int 
lauth(lua_State *L){
	ssh_session session=get_session(L);
	const char * password = luaL_checkstring(L, 2);
	int rc = ssh_userauth_password(session, NULL, password);	
	if(rc != SSH_AUTH_SUCCESS){
		return luaL_error(L,"ssh authentication failed: %s\n",ssh_get_error(session));
	}
	return 0;
}

static int 
lbanner(lua_State *L){
	ssh_session session=get_session(L);
	char *banner=ssh_get_issue_banner(session);
	if(banner){
		lua_pushlstring(L, banner,strlen(banner));
		ssh_string_free_char(banner);
		return 1;
	}
	return 0;
}

static int 
loptions(lua_State *L){
	//ssh_session session=get_session(L);
	return 0;
}

static int 
lnew(lua_State *L) {
    const char * host = luaL_checkstring(L, 1);
	const char * user = luaL_checkstring(L, 2);
	
	ssh_session	session=ssh_new();
	if (session == NULL) {
		return 0;
	}

	int verbosity=0;
	set_options(L,session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
	set_options(L,session, SSH_OPTIONS_HOST, host);
	set_options(L,session, SSH_OPTIONS_USER, user);

	if(lua_isinteger(L,3)){
		int port=luaL_checkinteger(L,3);
		set_options(L,session, SSH_OPTIONS_PORT,&port);
	}
	if(ssh_connect(session)){
		char err[1024];
		snprintf(err,sizeof(err),"%s",ssh_get_error(session));
		ssh_free(session);
		return luaL_error(L,err);
	}

	struct session_ud * sessionud = lua_newuserdata(L, sizeof(*sessionud));
	sessionud->session=session;
	
    if (luaL_newmetatable(L, "sessionmeta")) {
        lua_pushcfunction(L, ldisconnect);
        lua_setfield(L, -2, "__gc");
        luaL_Reg l[] = {
			{ "options",loptions},
			{ "auth",lauth},
			{ "banner",lbanner},
			{ "disconnect",ldisconnect},
			{ "scp",lscp_new},
			{ "sftp",lsftp_new},
			{ "channel",lchannel_new},
            { NULL, NULL },
        };
        luaL_newlib(L, l);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);
    return 1;
}

int luaopen_ssh_c(lua_State *L) {
    luaL_checkversion(L);
	luaL_Reg l[] = {
		{"new",lnew},
		{ NULL, NULL },
	};
	luaL_newlib(L,l);
	return 1;
}

