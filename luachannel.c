#include <string.h>
#include "ssh_common.h"
#include "libssh/channels.h"
struct channel_ud{
	ssh_channel channel;	
};

static void check_channel_rc(lua_State *L,ssh_channel channel,int rc){
	if(rc != SSH_OK){
		luaL_error(L,ssh_get_error(channel->session));
	}
}

static ssh_channel get_channel(lua_State *L){
    struct channel_ud * ud = lua_touserdata(L, 1);
	if (ud == NULL || ud->channel==NULL)
		luaL_error(L, "channel %p already closed", ud);
	return ud->channel;
}

static int 
lchannel_close(lua_State *L) {
	struct channel_ud * ud = lua_touserdata(L, 1);
	if (ud == NULL || ud->channel == NULL)
		return 0;
	ssh_channel_close(ud->channel);
	ssh_channel_free(ud->channel);
	ud->channel=NULL;
	return 0;
}

static int 
lchannel_read(lua_State *L) {
	ssh_channel channel = get_channel(L);
	const int size  = lua_isinteger(L,2)==1?lua_tointeger(L,2):1024;
	char buffer[size];
	int nbytes = ssh_channel_read(channel,buffer,sizeof(buffer),0);
	if(nbytes<0)luaL_error(L,ssh_get_error(channel->session));
	if(nbytes==0)return 0;
	lua_pushlstring(L,buffer,nbytes);
	return 1;	
}	

static int 
lchannel_write(lua_State *L) {
	ssh_channel channel = get_channel(L);
	size_t len=0;
	const char *buffer=luaL_checklstring(L,2,&len);
	len  = lua_isinteger(L,3)==1?lua_tointeger(L,3):len;
	int r = ssh_channel_write(channel,buffer,sizeof(buffer));
	if(r==SSH_ERROR){
		luaL_error(L,ssh_get_error(channel->session));
	}
	return 0;	
}
static int 
lchannel_is_closed(lua_State *L) {
	ssh_channel channel = get_channel(L);
	lua_pushboolean(L,ssh_channel_is_closed(channel));
	return 1;	
}

static int 
lchannel_is_eof(lua_State *L) {
	ssh_channel channel = get_channel(L);
	lua_pushboolean(L,ssh_channel_is_eof(channel));
	return 1;	
}

static int 
lchannel_is_open(lua_State *L) {
	ssh_channel channel = get_channel(L);
	lua_pushboolean(L,ssh_channel_is_open(channel));
	return 1;	
}

static int 
lchannel_request_exec(lua_State *L) {
	ssh_channel channel=get_channel(L);	
	const char *cmd=luaL_checkstring(L,2);
	check_channel_rc(L,channel,ssh_channel_request_exec(channel,cmd));
	return 0;
}

static int 
lchannel_request_env(lua_State *L) {
	ssh_channel channel=get_channel(L);	
	const char *key=luaL_checkstring(L,2);
	const char *value=luaL_checkstring(L,3);
	check_channel_rc(L,channel,ssh_channel_request_env(channel,key,value));
	return 0;
}

static int 
lchannel_request_send_signal(lua_State *L) {
	ssh_channel channel=get_channel(L);	
	const char *sig=luaL_checkstring(L,2);
	check_channel_rc(L,channel,ssh_channel_request_send_signal(channel,sig));
	return 0;
}

static int 
lchannel_send_eof(lua_State *L) {
	ssh_channel channel=get_channel(L);	
	check_channel_rc(L,channel,ssh_channel_send_eof(channel));
	return 0;
}

int 
lchannel_new(lua_State *L) {
	ssh_session	session=get_session(L);
	ssh_channel channel = ssh_channel_new(session);;
	if (channel == NULL) {
		luaL_error(L,ssh_get_error(session));
	}

	check_channel_rc(L,channel,ssh_channel_open_session(channel));

	struct channel_ud * ud = lua_newuserdata(L, sizeof(*ud));
	ud->channel=channel;
    if (luaL_newmetatable(L, "sshchannelmeta")) {
        lua_pushcfunction(L, lchannel_close);
        lua_setfield(L, -2, "__gc");
        luaL_Reg l[] = {
			{ "close",lchannel_close},
			{ "request_exec",lchannel_request_exec},
			{ "request_env",lchannel_request_env},
			{ "request_send_signal",lchannel_request_send_signal},
			{ "send_eof",lchannel_send_eof},
			{ "read",lchannel_read},
			{ "write",lchannel_write},
			{ "isclosed",lchannel_is_closed},
			{ "iseof",lchannel_is_eof},
			{ "isopen",lchannel_is_open},
            { NULL, NULL },
        };
        luaL_newlib(L, l);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);
    return 1;
}
