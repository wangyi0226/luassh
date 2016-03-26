local ssh=require "ssh.c"
local session=ssh.new("192.168.0.110","root",1235)
--local session=ssh.new("127.0.0.1","nbwk")

session:auth("123456")

print(session:banner())

local channel=session:channel()
channel:request_exec("date")
local str=channel:read()
while str do
	print(str)	
	str=channel:read()
end
print("is_closed",channel:isclosed())
print("is_eof",channel:iseof())
print("is_open",channel:isopen())
