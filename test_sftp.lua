local ssh=require "ssh.c"

local session=ssh.new("192.168.0.110","root",1235)
session:auth("123456")
print(session:banner())
local sftp=session:sftp()
local extlist=sftp:extensions_list()
print("Additional SFTP extensions provided by the server");
for k,v in pairs(extlist) do
	print(k,v)
end
sftp:symlink("/tmp/this_is_the_link","/tmp/sftp_symlink_test")
print("readlink",sftp:readlink("/tmp/sftp_symlink_test"))
print("rmlink",sftp:unlink("/tmp/sftp_symlink_test"))
print("extension_supported",sftp:extension_supported("statvfs@openssh.com", "2"))
print("-----------------------sftp_statvfs")
local statvfs=sftp:statvfs("/tmp")
for k,v in pairs(statvfs) do
	print(k,v)
end
print("-----------------------read file attr")
local dir = sftp:opendir("/root")
while true do
	local file=dir:read() 
	if not file then
		break
	end
	for k,v in pairs(file) do
		print(k,v)
	end
end
print("-------------------------mkdir && rmrdir && rename")
sftp:mkdir("/root/db/test2",0)
sftp:rename("/root/db/test2","/root/db/test3")
sftp:rmdir("/root/db/test3")

print("-------------------------copy file")
local from=sftp:open("/root/db/test/install.log","O_RDONLY")
local to=sftp:open("/root/db/temp.log",{"O_WRONLY","O_CREAT","O_TRUNC"}, 0700)
--local to=sftp:open("/root/db/temp.log",{"O_WRONLY"}, 0700)
local buffer=from:read()
while buffer do
	to:write(buffer)
	buffer=from:read()
end

sftp:unlink("/root/db/temp.log")
