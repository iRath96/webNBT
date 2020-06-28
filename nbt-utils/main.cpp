//
//  main.cpp
//  nbt-utils
//
//  Created by Alexander Rath on 14.02.15.
//  Copyright (c) 2015 Alexander Rath. All rights reserved.
//

#include "nbt_utils.h"
using namespace nbt;

#ifndef EMSCRIPTEN
#pragma mark Standalone example
#include <fstream>
int main(int argc, const char * argv[]) {
  std::ifstream is;
  is.open("/Users/alex/Desktop/level.dat", std::ios::binary);
  
  std::stringstream buffer;
  buffer << is.rdbuf();
  
  Tag *t = Tag::deserializeCompressed(buffer.str());
  std::string out = Tag::serializeCompressed(t);
  
  std::stringstream ostream {};
  Tag::write(t, ostream, "Example");
  
  printf("%p\n", t);
  
  return 0;
}
#else
#pragma mark - Emscripten bindings
#include <emscripten/bind.h>

using namespace emscripten;

EMSCRIPTEN_BINDINGS(my_module) {
  function("makeTag", &makeTag, allow_raw_pointers());
  
  // enum_<TagType::Enum>("TagType");
  
  class_<Tag>("Tag")
  //.class_function("read", &Tag::read)
  .class_function("serialize", &Tag::serialize, allow_raw_pointers())
  .class_function("serializeCompressed", &Tag::serializeCompressed, allow_raw_pointers())
  .class_function("deserialize", &Tag::deserialize, allow_raw_pointer<ret_val>())
  .class_function("deserializeCompressed", &Tag::deserializeCompressed, allow_raw_pointers())
  .function("getName", &Tag::getName)
  .function("setName", &Tag::setName)
  .function("hasName", &Tag::getHasName)
  .function("setHasName", &Tag::setHasName)
  .function("getStartIndex", &Tag::getStartIndex)
  .function("getEndIndex", &Tag::getEndIndex)
  .function("tagType", &Tag::tagType)
  ;
  
#define array_class(klass) \
  class_<klass>(#klass) \
  .function("getElement", &klass::getElement) \
  .function("setElement", &klass::setElement) \
  .function("getCount", &klass::getCount) \
  .function("resize", &klass::resize) \
  .function("serialize", &klass::serialize) \
  .function("deserialize", &klass::deserialize)
  
  array_class(U8Array);
  array_class(I32Array);
  array_class(I64Array);
#undef array_class
  
  class_<TagHash>("TagHash") // Not thread-safe
  .function("begin", &TagHash::jsBegin)
  .function("atEnd", &TagHash::jsAtEnd)
  .function("next", &TagHash::jsNext)
  .function("getName", &TagHash::getName)
  .function("getTag", &TagHash::getTag, allow_raw_pointers())
  .function("set", &TagHash::jsSet, allow_raw_pointers())
  .function("remove", &TagHash::jsRemove)
  .function("rename", &TagHash::jsRename)
  ;
  
#define subclass(klass) \
  class_<klass, base<Tag>>(#klass) \
  .function("getValue", &klass::getValue) \
  .function("getValuePtr", &klass::getValuePtr, allow_raw_pointers()) \
  .function("setValue", &klass::setValue) \
  .function("serializeValue", &klass::serializeValue) \
  .function("deserializeValue", &klass::deserializeValue)
  
  subclass(ByteTag);
  subclass(ShortTag);
  subclass(IntTag);
  subclass(LongTag);
  subclass(FloatTag);
  subclass(DoubleTag);
  subclass(StringTag);
  subclass(CompoundTag);
  subclass(ByteArrayTag);
  subclass(IntArrayTag);
  subclass(LongArrayTag);
  subclass(ListTagBase);
#undef subclass
  
  class_<ListTag, base<ListTagBase>>("ListTag")
  .function("getElement", &ListTag::getElement, allow_raw_pointers())
  .function("addElement", &ListTag::addElement, allow_raw_pointers())
  .function("remove", &ListTag::removeElement)
  .function("getCount", &ListTag::getCount)
  .function("clear", &ListTag::clear)
  .function("getEntryKind", &ListTag::getEntryKind)
  .function("setEntryKind", &ListTag::setEntryKind)
  ;
}
#endif