LUA_INC=/usr/local/include
LUA_LIB=/usr/local/lib

SSH3RD_LIB=3rd/build/lib
SSH3RD_INC=3rd/build/include

all: ssh.so

#libmcrypt:
	#cd ./3rd/libmcrypt/ && ./configure && make && make install

#zlib:
	#cd ./3rd/zlib/ && ./configure && make && make install

libssh:
	cd ./3rd/libssh/ && rm -rf build && mkdir build && cd build && cmake ../ -DCMAKE_INSTALL_PREFIX=../../build && make install && cp -r ../include/libssh ../../build/include && cp config.h ../../build/include/libssh

ssh.so:libssh
	gcc --shared -Wall -fPIC -o ssh.so luassh.c luascp.c luasftp.c luachannel.c $(OBJECTS) -I$(LUA_INC) -I$(SSH3RD_INC) $(OBJECTS) -L$(LUA_LIB) -llua -L$(SSH3RD_LIB) -lssh -lssh_threads

clean:
	rm -f obj/*.o
