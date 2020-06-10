NBT_CPP=$(wildcard nbt-utils/*.cpp)
NBT_O=$(NBT_CPP:.cpp=.bc)

build: $(NBT_O)
	em++ -O2 -s ASSERTIONS=2 -s ALLOW_MEMORY_GROWTH=1 -s DISABLE_EXCEPTION_CATCHING=0 --bind $(NBT_O) -s USE_ZLIB=1 -o web-app/NBT.js

test: build
	node NBT.js

clean:
	rm -f $(NBT_O)

%.bc: %.cpp
	echo $? -> $@
	em++ -O2 $? -c -o $@ -std=c++0x
