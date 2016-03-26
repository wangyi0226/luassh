#include <string.h>
#include "ssh_common.h"
#include "libssh/scp.h"

struct scp_ud{
	ssh_scp scp;
};

static struct  ssh_kv scp_modes[]={
	{"SSH_SCP_WRITE",SSH_SCP_WRITE},
	{"SSH_SCP_READ",SSH_SCP_READ},
	{"SSH_SCP_RECURSIVE",SSH_SCP_RECURSIVE},
	{NULL,0}
};

static const int get_scp_mode(lua_State *L,const int index){
	return get_valbyindex(L,scp_modes,index);
}

static struct scp_ud*  get_scpud(lua_State *L){
	struct scp_ud * scpud = lua_touserdata(L, 1);
	if (scpud == NULL ||  scpud->scp==NULL)
		luaL_error(L, "stp %p already closed", scpud);
	return scpud;
}

static ssh_session scp_close(struct scp_ud *scpud){
	if (scpud == NULL || scpud->scp == NULL)return NULL;
	ssh_session session= scpud->scp->session;
	ssh_scp_close(scpud->scp);
	ssh_scp_free(scpud->scp);
	scpud->scp=NULL;
	return session;
}

static void scp_error(lua_State *L,struct scp_ud *scpud){
	ssh_session session=scp_close(scpud);
	if(session==NULL){
		luaL_error(L, "stp %p already closed", scpud);
	}
	luaL_error(L,ssh_get_error(session));
}

static int 
lscp_close(lua_State *L) {
	struct scp_ud * scpud = lua_touserdata(L, 1);
	scp_close(scpud);
	return 0;
}

static int 
lscp_write(lua_State *L) {
	struct scp_ud * scpud = get_scpud(L);
	if(scpud->scp->state==SSH_SCP_WRITE_INITED){
		return 0;
	}
	size_t len=0;
	const char *buffer=luaL_checklstring(L,2,&len);
	int r=ssh_scp_write(scpud->scp,buffer,len);
	if(r==SSH_ERROR){
		scp_error(L,scpud);
	}
	return 0;
}

static int 
lscp_deny_request(lua_State *L) {
	struct scp_ud * scpud = get_scpud(L);
	const char *reason=luaL_checkstring(L,2);
	int r=ssh_scp_deny_request(scpud->scp,reason);
	if(r==SSH_ERROR){
		scp_error(L,scpud);
	}
	return 0;
}

static int 
lscp_read(lua_State *L) {
	struct scp_ud * scpud = get_scpud(L);
	if(scpud->scp->state==SSH_SCP_READ_INITED){
		return 0;
	}
    const int size  = lua_isinteger(L,2)==1?lua_tointeger(L,2):1024;
	char buffer[size];
	int r=ssh_scp_read(scpud->scp,buffer,sizeof(buffer));
	if(r==SSH_ERROR){
		scp_error(L,scpud);
	}
	lua_pushlstring(L,buffer,r);
	return 1;
}

static int 
lscp_pull_request(lua_State *L) {
	struct scp_ud * scpud = get_scpud(L);
	int  r=ssh_scp_pull_request(scpud->scp);
	if(r==SSH_ERROR){
		scp_error(L,scpud);
	}
	switch(r){
		case SSH_SCP_REQUEST_NEWFILE:lua_pushstring(L,"SSH_SCP_REQUEST_NEWFILE");break;
		case SSH_SCP_REQUEST_NEWDIR:lua_pushstring(L,"SSH_SCP_REQUEST_NEWDIR");break;
		case SSH_SCP_REQUEST_ENDDIR:lua_pushstring(L,"SSH_SCP_REQUEST_ENDDIR");break;
		case SSH_SCP_REQUEST_EOF:lua_pushstring(L,"SSH_SCP_REQUEST_EOF");break;
		case SSH_SCP_REQUEST_WARNING:lua_pushstring(L,"SSH_SCP_REQUEST_WARNING");break;
	}
	return 1;
}

static int lscp_request_get_warning(lua_State *L){
	struct scp_ud * scpud = get_scpud(L);
	lua_pushstring(L,ssh_scp_request_get_warning(scpud->scp));
	return 1;
}

static int lscp_request_get_size(lua_State *L){
	struct scp_ud * scpud = get_scpud(L);
	lua_pushinteger(L,ssh_scp_request_get_size(scpud->scp));
	return 1;
}

static int lscp_request_get_filename(lua_State *L){
	struct scp_ud * scpud = get_scpud(L);
	lua_pushstring(L,ssh_scp_request_get_filename(scpud->scp));
	return 1;
}

static int lscp_request_get_permissions(lua_State *L){
	struct scp_ud * scpud = get_scpud(L);
	lua_pushinteger(L,ssh_scp_request_get_permissions(scpud->scp));
	return 1;
}

static int lscp_leave_directory(lua_State *L){
	struct scp_ud * scpud = get_scpud(L);
	int r=ssh_scp_leave_directory(scpud->scp);
	if(r==SSH_ERROR){
		scp_error(L,scpud);
	}
	return 0;
}

static int lscp_push_directory(lua_State *L){
	struct scp_ud * scpud = get_scpud(L);
    const char * dirname  = luaL_checkstring(L, 2);
    const int mode  = luaL_checkinteger(L, 3);
	int r=ssh_scp_push_directory(scpud->scp,dirname,mode);
	if(r==SSH_ERROR){
		scp_error(L,scpud);
	}
	return 0;
}

static int lscp_push_file(lua_State *L){
	struct scp_ud * scpud = get_scpud(L);
    const char * filename  = luaL_checkstring(L, 2);
    const int size  = luaL_checkinteger(L, 3);
    const int mode  = luaL_checkinteger(L, 4);
	int r=ssh_scp_push_file(scpud->scp,filename,size,mode);
	if(r==SSH_ERROR){
		scp_error(L,scpud);
	}
	return 0;
}

static int lscp_accept_request(lua_State *L){
	struct scp_ud * scpud = get_scpud(L);
	int r=ssh_scp_accept_request(scpud->scp);
	if(r==SSH_ERROR){
		scp_error(L,scpud);
	}
	lua_pushinteger(L,r);
	return 1;
}

int 
lscp_new(lua_State *L) {
	ssh_session session=get_session(L);
    const char * location  = luaL_checkstring(L, 2);
	int mode=get_scp_mode(L,3);
	ssh_scp scp=ssh_scp_new(session, mode, location);

	if(ssh_scp_init(scp) != SSH_OK){
		ssh_scp_free(scp);
		luaL_error(L,ssh_get_error(session));
	}

	struct scp_ud * scpud = lua_newuserdata(L, sizeof(*scpud));
	scpud->scp	=scp;
	
    if (luaL_newmetatable(L, "scpmeta")) {
        lua_pushcfunction(L, lscp_close);
        lua_setfield(L, -2, "__gc");
        luaL_Reg l[] = {
			{"close",lscp_close},
			{"write",lscp_write},
			{"read",lscp_read},
			{"pull_request",lscp_pull_request},
			{"request_get_warning",lscp_request_get_warning},
			{"request_get_size",lscp_request_get_size},
			{"request_get_filename",lscp_request_get_filename},
			{"request_get_permissions",lscp_request_get_permissions},
			{"accept_request",lscp_accept_request},
			{"leave_directory",lscp_leave_directory},
			{"push_directory",lscp_push_directory},
			{"push_file",lscp_push_file},
			{"deny_request",lscp_deny_request},
            { NULL, NULL },
        };
        luaL_newlib(L, l);
        lua_setfield(L, -2, "__index");
    }
    lua_setmetatable(L, -2);
    return 1;
}
