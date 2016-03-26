#include <string.h>
#include "ssh_common.h"
#include <fcntl.h>
#include "libssh/sftp.h"

struct sftp_ud{
	sftp_session sftp;
};

struct sftpdir_ud{
	sftp_dir dir;
};

struct sftpfile_ud{
	sftp_file file;
};

void attributes2tbl(lua_State *L,sftp_attributes file){
	lua_createtable(L,0,22);

	lua_pushstring(L,file->name);
	lua_setfield(L, -2,"name");

	lua_pushstring(L,file->longname);
	lua_setfield(L, -2,"longname");

	lua_pushinteger(L,file->flags);
	lua_setfield(L,-2,"flags");

	lua_pushinteger(L,file->type);
	lua_setfield(L,-2,"type");

	lua_pushinteger(L,file->size);
	lua_setfield(L,-2,"size");

	lua_pushinteger(L,file->uid);
	lua_setfield(L,-2,"uid");

	lua_pushinteger(L,file->gid);
	lua_setfield(L,-2,"gid");

	lua_pushstring(L,file->owner);
	lua_setfield(L,-2,"owner");

    lua_pushstring(L,file->group);
	lua_setfield(L,-2,"group");

    lua_pushinteger(L,file->permissions);
	lua_setfield(L,-2,"permissions");

    lua_pushinteger(L,file->atime64);
	lua_setfield(L,-2,"atime64");

	lua_pushinteger(L,file->atime);
    lua_setfield(L,-2,"atime");

    lua_pushinteger(L,file->atime_nseconds);
	lua_setfield(L,-2,"atime_nseconds");

    lua_pushinteger(L,file->createtime);
	lua_setfield(L,-2,"createtime");

    lua_pushinteger(L,file->createtime_nseconds);
	lua_setfield(L,-2,"createtime_nseconds");

    lua_pushinteger(L,file->mtime64);
	lua_setfield(L,-2,"mtime64");

    lua_pushinteger(L,file->mtime);
	lua_setfield(L,-2,"mtime");

    lua_pushinteger(L,file->mtime_nseconds);
	lua_setfield(L,-2,"mtime_nseconds");

	lua_pushlightuserdata(L,file->acl);
	lua_setfield(L,-2,"acl");
	
	lua_pushinteger(L,file->extended_count);
	lua_setfield(L,-2,"extended_count");

	lua_pushlightuserdata(L,file->extended_type);
	lua_setfield(L,-2,"extended_type");

	lua_pushlightuserdata(L,file->extended_data);
	lua_setfield(L,-2,"extended_data");

	sftp_attributes_free(file);
}

void statvfs2tbl(lua_State *L,sftp_statvfs_t sftpstatvfs){
	lua_createtable(L,0,11);
	
	lua_pushinteger(L,sftpstatvfs->f_bsize);
	lua_setfield(L, -2,"bsize");

	lua_pushinteger(L,sftpstatvfs->f_frsize);
	lua_setfield(L, -2,"frsize");

    lua_pushinteger(L,sftpstatvfs->f_blocks);
	lua_setfield(L, -2,"blocks");

	lua_pushinteger(L,sftpstatvfs->f_bfree);
	lua_setfield(L, -2,"bfree");

	lua_pushinteger(L,sftpstatvfs->f_bavail);
	lua_setfield(L, -2,"bavail");

	lua_pushinteger(L,sftpstatvfs->f_files);
	lua_setfield(L, -2,"files");

	lua_pushinteger(L,sftpstatvfs->f_ffree);
	lua_setfield(L, -2,"ffree");

	lua_pushinteger(L,sftpstatvfs->f_favail);
	lua_setfield(L, -2,"favail");

    lua_pushinteger(L,sftpstatvfs->f_fsid);
	lua_setfield(L, -2,"fsid");

    lua_pushinteger(L,sftpstatvfs->f_flag);
	lua_setfield(L, -2,"flag");

    lua_pushinteger(L,sftpstatvfs->f_namemax);
	lua_setfield(L, -2,"namemax");

	sftp_statvfs_free(sftpstatvfs);
}

static struct  ssh_kv access_types[]={
    {"O_RDONLY",O_RDONLY},
	{"O_WRONLY",O_WRONLY},
	{"O_RDWR",O_RDWR},
	{"O_CREAT",O_CREAT},
	{"O_EXCL",O_EXCL},
	{"O_CREAT",O_CREAT},
	{"O_TRUNC",O_TRUNC},
	{NULL,0}
};

static const int get_access(lua_State *L,const int index){
		return get_valbyindex(L,access_types,index);
}

static int error(lua_State *L,struct sftp_ud *ud){
	return luaL_error(L,ssh_get_error(ud->sftp->session));
}

static struct sftp_ud*  get_sftpud(lua_State *L){
    struct sftp_ud * sftpud = lua_touserdata(L, 1);
	if (sftpud == NULL || sftpud->sftp==NULL)
			luaL_error(L, "sftp %p already closed", sftpud);
	return sftpud;
}

static struct sftpdir_ud*  get_dirud(lua_State *L){
    struct sftpdir_ud* ud = lua_touserdata(L, 1);
	if (ud == NULL || ud->dir == NULL)
		luaL_error(L, "dir %p already closed", ud);
	return ud;
}

static struct sftpfile_ud*  get_fileud(lua_State *L){
    struct sftpfile_ud* ud = lua_touserdata(L, 1);
	if (ud == NULL || ud->file == NULL)
		luaL_error(L, "file %p already closed", ud);
	return ud;
}

static int ldir_close(lua_State *L){
	struct sftpdir_ud *ud=get_dirud(L);
	if(sftp_closedir(ud->dir)){
		luaL_error(L,ssh_get_error(ud->dir->sftp->session));
	}
	return 0;
}

static int ldir_read(lua_State *L){
	struct sftpdir_ud *ud=get_dirud(L);
	sftp_attributes file=sftp_readdir(ud->dir->sftp,ud->dir);
	if(file==NULL){
		return 0;
	}
	attributes2tbl(L,file);
	return 1;
}

static int lsftp_opendir(lua_State *L){
	struct sftp_ud* sftpud = get_sftpud(L);
	const char *path=luaL_checkstring(L,2);	
	sftp_dir dir=sftp_opendir(sftpud->sftp,path);
	if(!dir) {
		return error(L,sftpud);
	}
	struct sftpdir_ud *ud=lua_newuserdata(L,sizeof(*ud));
	ud->dir=dir;

	if(luaL_newmetatable(L, "stpdirmeta")){
		lua_pushcfunction(L, ldir_close);
		lua_setfield(L, -2, "__gc");
		luaL_Reg l[] = {
			{"close",ldir_close},
			{"read",ldir_read},
			{ NULL, NULL },
		};
		luaL_newlib(L, l);
		lua_setfield(L, -2, "__index");

	}

	lua_setmetatable(L, -2);
	return 1;
}

static int lfile_close(lua_State *L){
    struct sftpfile_ud *ud=get_fileud(L);
	if(sftp_close(ud->file)){
		luaL_error(L,ssh_get_error(ud->file->sftp->session));
	}
	return 0;
}

static int lfile_read(lua_State *L){
    struct sftpfile_ud *ud=get_fileud(L);
	const int size  = lua_isinteger(L,2)==1?lua_tointeger(L,2):1024;
	char buffer[size];
	int r=sftp_read(ud->file,buffer,sizeof(buffer));
	if(r<0){
		return luaL_error(L,ssh_get_error(ud->file->sftp->session));
	}
	if(r==0){
		return 0;
	}
	lua_pushlstring(L,buffer,r);
	return 1;
}

static int lfile_write(lua_State *L){
    struct sftpfile_ud *ud=get_fileud(L);
	size_t len=0;
	const char *buffer=luaL_checklstring(L,2,&len);
	return sftp_write(ud->file,buffer,len)==len?0:luaL_error(L,ssh_get_error(ud->file->sftp->session));
}

static int lfile_fstat(lua_State *L){
	struct sftpfile_ud *ud=get_fileud(L);
	sftp_attributes attributes=sftp_fstat(ud->file);
	if(attributes == NULL){
		return luaL_error(L,ssh_get_error(ud->file->sftp->session));
	}
	attributes2tbl(L,attributes);
	return 1;
}

static int lfile_fstatvfs(lua_State *L){
	struct sftpfile_ud *ud=get_fileud(L);
	sftp_statvfs_t vfs=sftp_fstatvfs(ud->file);
	if(vfs == NULL){
		return luaL_error(L,ssh_get_error(ud->file->sftp->session));
	}
	statvfs2tbl(L,vfs);
	return 1;
}

static int lsftp_open(lua_State *L){
	struct sftp_ud* sftpud = get_sftpud(L);
	const char *filename=luaL_checkstring(L,2);	
	const int accesstype=get_access(L,3);
	const int mode=lua_isinteger(L,4)==1?lua_tointeger(L,4):0;

	sftp_file file=sftp_open(sftpud->sftp,filename,accesstype,mode);
	if(!file){
		return error(L,sftpud);
	}

	struct sftpfile_ud *ud=lua_newuserdata(L,sizeof(*ud));
	ud->file=file;

	if(luaL_newmetatable(L, "stpfilemeta")){
		lua_pushcfunction(L, lfile_close);
		lua_setfield(L, -2, "__gc");
		luaL_Reg l[] = {
			{"close",lfile_close},
			{"read",lfile_read},
			{"write",lfile_write},
			{"fstat",lfile_fstat},
			{"fstatvfs",lfile_fstatvfs},
			{ NULL, NULL },
		};
		luaL_newlib(L, l);
		lua_setfield(L, -2, "__index");

	}

	lua_setmetatable(L, -2);

	return 1;
}

static int lsftp_close(lua_State *L){
	struct sftp_ud * sftpud = lua_touserdata(L, 1);
	if(sftpud==NULL || sftpud->sftp == NULL){
		return 0;
	}
	sftp_free(sftpud->sftp);
	sftpud->sftp=NULL;
	return 0;
}

static int lsftp_extensions_list(lua_State *L){
	struct sftp_ud* sftpud = get_sftpud(L);
	int count = sftp_extensions_get_count(sftpud->sftp);
	int i=0;
	lua_createtable(L,0,count);
	for (i = 0; i < count; i++) {
		const char *name=sftp_extensions_get_name(sftpud->sftp, i);
		lua_pushstring(L,sftp_extensions_get_data(sftpud->sftp, i));
		lua_setfield(L, -2, name);
	}
	return 1;
}

static int lsftp_extension_supported(lua_State *L){
	struct sftp_ud* sftpud	= get_sftpud(L);
	const char * name		= luaL_checkstring(L, 2);
	const char * data		= luaL_checkstring(L, 3);
	lua_pushboolean(L,sftp_extension_supported(sftpud->sftp,name,data));
	return 1;
}

static int lsftp_symlink(lua_State *L){
	struct sftp_ud* sftpud = get_sftpud(L);
	const char * target  = luaL_checkstring(L, 2);
	const char * dest	 = luaL_checkstring(L, 3);
    if (sftp_symlink(sftpud->sftp, target,dest)< 0) {
		return error(L,sftpud);
	}
	return 0;
}

static int lsftp_readlink(lua_State *L){
	struct sftp_ud* sftpud = get_sftpud(L);
	const char * path  = luaL_checkstring(L, 2);
	const char * lnk = sftp_readlink(sftpud->sftp,path);
	if (lnk == NULL) {
		return error(L,sftpud);
	}
	lua_pushstring(L,lnk);
	return 1;
}

static int lsftp_unlink(lua_State *L){
	struct sftp_ud* sftpud = get_sftpud(L);
	const char * lnk  = luaL_checkstring(L, 2);
	if(sftp_unlink(sftpud->sftp,lnk)){
		return error(L,sftpud);
	}
	return 0;
}

static int lsftp_statvfs(lua_State *L){
	struct sftp_ud* sftpud = get_sftpud(L);
	const char * path  = luaL_checkstring(L, 2);
	sftp_statvfs_t sftpstatvfs=sftp_statvfs(sftpud->sftp,path);
	if (sftpstatvfs == NULL) {
		return error(L,sftpud);
	}
	statvfs2tbl(L,sftpstatvfs);
	
	return 1;	
}

static int lsftp_rmdir(lua_State *L){
	struct sftp_ud* sftpud = get_sftpud(L);
	const char * dir  = luaL_checkstring(L, 2);
	if(sftp_rmdir(sftpud->sftp,dir)){
		return error(L,sftpud);
	}
	return 0;
}

static int lsftp_mkdir(lua_State *L){
	struct sftp_ud* sftpud = get_sftpud(L);
	const char * dir  = luaL_checkstring(L, 2);
	const int  mode  = luaL_checkinteger(L, 3);
	if(sftp_mkdir(sftpud->sftp,dir,mode)){
		return error(L,sftpud);
	}
	return 0;
}

static int lsftp_rename(lua_State *L){
	struct sftp_ud* sftpud = get_sftpud(L);
	const char* original = luaL_checkstring(L, 2);
	const char* newname  = luaL_checkstring(L, 3);
	if(sftp_rename(sftpud->sftp,original,newname)){
		return error(L,sftpud);
	}
	return 0;
}

int 
lsftp_new(lua_State *L) {
	ssh_session session=get_session(L);
	sftp_session sftp=sftp_new(session);
	if(!sftp || sftp_init(sftp)){
		luaL_error(L,ssh_get_error(session));
	}
	struct sftp_ud *sftpud=lua_newuserdata(L,sizeof(*sftpud));
	sftpud->sftp=sftp;
	if (luaL_newmetatable(L, "sftpmeta")) {
		lua_pushcfunction(L, lsftp_close);
		lua_setfield(L, -2, "__gc");
		luaL_Reg l[] = {
			{"close",lsftp_close},
			{"extensions_list",lsftp_extensions_list},
			{"extension_supported",lsftp_extension_supported},
			{"symlink",lsftp_symlink},
			{"readlink",lsftp_readlink},
			{"unlink",lsftp_unlink},
			{"statvfs",lsftp_statvfs},
			{"opendir",lsftp_opendir},
			{"open",lsftp_open},
			{"mkdir",lsftp_mkdir},
			{"rmdir",lsftp_rmdir},
			{"rename",lsftp_rename},
			{ NULL, NULL },
		};
		luaL_newlib(L, l);
		lua_setfield(L, -2, "__index");
	}
	lua_setmetatable(L, -2);
	return 1;

}
