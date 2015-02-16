NBT_CPP=$(wildcard nbt-utils/*.cpp)
NBT_O=$(NBT_CPP:.cpp=.o)

build: $(NBT_O)
	em++ -s ASSERTIONS=2 -s DISABLE_EXCEPTION_CATCHING=0 --bind -O2 $(NBT_O) zlib/libz.a -o web-app/NBT.js

test: build
	node NBT.js

clean:
	rm -f $(NBT_O)

%.o: %.cpp
	echo $? -> $@
	em++ -Izlib -O2 $? -o $@ -std=c++0x