local ssh=require "ssh.c"
--"SSH_SCP_WRITE","SSH_SCP_READ"不能组合

local function test_write(session)
	local scp=session:scp("/root/db/test",{"SSH_SCP_WRITE","SSH_SCP_RECURSIVE"})
	scp:push_directory("haha",447)
	scp:leave_directory()
	scp:push_file("Haha",10,447)
	scp:write("test scp");
end

local function test_read(session)
	local scp=session:scp("/root/db/test",{"SSH_SCP_READ","SSH_SCP_RECURSIVE"})
	while true do
		local ret=scp:pull_request()
		print("pull_request",ret)
		if ret == "SSH_SCP_REQUEST_EOF" then
			break
		end
		if ret == "SSH_SCP_REQUEST_WARNING" then
			print(scp:request_get_warning())
		elseif ret == "SSH_SCP_REQUEST_NEWDIR" then
			print("dir:",scp:request_get_filename(),scp:request_get_permissions())
			scp:accept_request()
		elseif ret == "SSH_SCP_REQUEST_NEWFILE" then
			print("file:",scp:request_get_filename(),scp:request_get_permissions(),scp:request_get_size())
			scp:accept_request()
			local buffer=scp:read()
			local size=0 
			while buffer do
				size=size+#buffer
				buffer=scp:read()
			end
			print(size)
		end
	end
end

local session=ssh.new("192.168.0.110","root",1235)
session:auth("123456")
print(session:banner())

test_write(session)
test_read(session)
