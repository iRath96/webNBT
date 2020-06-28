//
//  nbt_utils.cpp
//  nbt-utils
//
//  Created by Alexander Rath on 15.02.15.
//  Copyright (c) 2015 Alexander Rath. All rights reserved.
//

#include "nbt_utils.h"

#include <iostream>
#include <vector>
#include <map>

int64_t ntoh64(int64_t input) {
  uint64_t rval;
  uint8_t *data = (uint8_t *)&rval;
  data[0] = input >> 56;
  data[1] = input >> 48;
  data[2] = input >> 40;
  data[3] = input >> 32;
  data[4] = input >> 24;
  data[5] = input >> 16;
  data[6] = input >> 8;
  data[7] = input >> 0;
  return rval;
}

int64_t hton64(int64_t input) {
  return ntoh64(input);
}

namespace nbt {
  class Tag;
}

using namespace nbt;

//! Factory method
Tag *nbt::makeTag(TagType::Enum type) {
  Tag *tag = NULL;
  switch(type) {
#define do_case(type) case TagType::type: tag = new type##Tag(); break;
      do_case(End);
      
      do_case(Byte);
      do_case(Short);
      do_case(Int);
      do_case(Long);
      do_case(Float);
      do_case(Double);
      do_case(ByteArray);
      do_case(String);
      do_case(List);
      do_case(Compound);
      do_case(IntArray);
      do_case(LongArray);
#undef do_case
    case TagType::Unknown: break;
  }
  return tag;
}

Tag *Tag::read(std::istream &stream, bool withName, TagType::Enum type) {
  size_t startIndex = stream.tellg();
  if(type == TagType::Unknown) stream.get((char &)type);
  if(type == 0) {
    Tag *t = new EndTag();
    t->startIndex = startIndex;
    t->endIndex = startIndex+1;
    return t;
  }
  
  std::string name;
  if(withName) {
    uint16_t nameLength;
    stream.read((char *)&nameLength, 2);
    
    nameLength = ntohs(nameLength);
    
    char nameBuf[nameLength+1];
    nameBuf[nameLength] = 0;
    
    stream.read(nameBuf, nameLength);
    
    name = nameBuf;
  } else
    name = "";
  
  Tag *tag = makeTag(type);
  tag->readPayload(stream);
  
  tag->name = name;
  tag->hasName = withName;
  
  tag->startIndex = startIndex;
  tag->endIndex = stream.tellg();
  
  return tag;
}

void Tag::write(Tag *tag, std::ostream &stream, TagType::Enum type) {
  tag->startIndex = stream.tellp();
  
  if(type == TagType::Unknown) stream.put(tag->tagType());
  tag->writePayload(stream);
  
  tag->endIndex = stream.tellp();
}

void Tag::write(Tag *tag, std::ostream &stream, const std::string &name, TagType::Enum type) {
  tag->startIndex = stream.tellp();
  
  if(type == TagType::Unknown) stream.put(tag->tagType());
  
  uint16_t nameLength = htons((uint16_t)name.length());
  stream.write((char *)&nameLength, 2);
  stream << name;
  
  tag->writePayload(stream);
  
  tag->endIndex = stream.tellp();
}

template<> void ListTagBase::readPayload(std::istream &) {}
template<> void ListTagBase::writePayload(std::ostream &) const {}

void ListTag::readPayload(std::istream &stream) {
  stream.get((char &)entryKind);
  
  uint32_t count;
  stream.read((char *)&count, 4);
  count = ntohl(count);
  
  value.resize(count);
  for(uint32_t i = 0; i < count; ++i) {
    size_t startIndex = stream.tellg();
    Tag *t = Tag::read(stream, false, entryKind);
    t->startIndex = startIndex;
    t->endIndex = stream.tellg();
    value[i].reset(t);
  }
}

void ListTag::writePayload(std::ostream &stream) const {
  stream.put(entryKind);
  
  uint32_t j = (uint32_t)value.size();
  uint32_t count = htonl(j);
  stream.write((char *)&count, 4);
  
  for(uint32_t i = 0; i < j; ++i) {
    value[i]->startIndex = stream.tellp();
    value[i]->writePayload(stream);
    value[i]->endIndex = stream.tellp();
  }
}

#pragma mark - Payload parsing

#define rd_payload(klass) template<> void nbt::klass::readPayload(std::istream &stream)
#define wr_payload(klass) template<> void nbt::klass::writePayload(std::ostream &stream) const

rd_payload(ByteTag) { stream.get((char &)value); }
wr_payload(ByteTag) { stream.put(value); }

rd_payload(ShortTag) {
  int16_t value;
  stream.read((char *)&value, 2);
  this->value = ntohs(value); }
wr_payload(ShortTag) {
  int16_t value = htons(this->value);
  stream.write((char *)&value, 2); }

rd_payload(IntTag) {
  int32_t value;
  stream.read((char *)&value, 4);
  this->value = ntohl(value); }
wr_payload(IntTag) {
  int32_t value = htonl(this->value);
  stream.write((char *)&value, 4); }

rd_payload(LongTag) {
  int64_t value;
  stream.read((char *)&value, 8);
  this->value = ntohll(value); }
wr_payload(LongTag) {
  int64_t value = htonll(this->value);
  stream.write((char *)&value, 8); }

rd_payload(FloatTag) { uint32_t val; stream.read((char *)&val, 4); val = ntohl(val); this->value = *(float *)&val; }
wr_payload(FloatTag) { uint32_t val = htonl(*(uint32_t *)&this->value); stream.write((char *)&val, 4); }

rd_payload(DoubleTag) { uint64_t val; stream.read((char *)&val, 8); val = ntohll(val); this->value = *(double *)&val; }
wr_payload(DoubleTag) { uint64_t val = htonll(*(uint64_t *)&this->value); stream.write((char *)&val, 8); }

rd_payload(ByteArrayTag) {
  uint32_t count;
  stream.read((char *)&count, 4);
  count = ntohl(count);
  
  value.count = count;
  value.data.reset((uint8_t *)malloc(count));
  stream.read((char *)value.data.get(), count); }
wr_payload(ByteArrayTag) {
  uint32_t count = htonl(value.count);
  stream.write((char *)&count, 4);
  stream.write((char *)value.data.get(), value.count); }

rd_payload(StringTag) {
  uint16_t count;
  stream.read((char *)&count, 2);
  count = ntohs(count);
  
  char buffer[count];
  stream.read(buffer, count);
  this->value = std::string(buffer, count); }
wr_payload(StringTag) {
  uint16_t count = htons(value.length());
  stream.write((char *)&count, 2);
  stream << value; }

rd_payload(CompoundTag) {
  while(true) {
    Tag *e = Tag::read(stream, true);
    if(e->tagType() == 0) break; // EndTag
    value[e->name] = std::shared_ptr<Tag>(e); // What about multiple tags with the same name?
  } }
wr_payload(CompoundTag) {
  for(auto it = value.begin(); it != value.end(); ++it)
    Tag::write(it->second.get(), stream, it->first);
  
  EndTag e;
  Tag::write(&e, stream); }

rd_payload(IntArrayTag) {
  uint32_t count;
  stream.read((char *)&count, 4);
  count = ntohl(count);
  
  value.count = count;
  value.data.reset((int32_t *)(malloc(count * 4)));
  stream.read((char *)value.data.get(), count * 4);
  for(uint32_t i = 0; i < count; ++i) value.data.get()[i] = ntohl(value.data.get()[i]); }
wr_payload(IntArrayTag) {
  uint32_t output[value.count];
  for(uint32_t i = 0; i < value.count; ++i) output[i] = htonl(value.data.get()[i]);
  
  uint32_t count = htonl(value.count);
  stream.write((char *)&count, 4);
  stream.write((char *)output, value.count * 4); }

rd_payload(LongArrayTag) {
  uint32_t count;
  stream.read((char *)&count, 4);
  count = ntohl(count);
  
  value.count = count;
  value.data.reset((int64_t *)(malloc(count * 8)));
  stream.read((char *)value.data.get(), count * 8);
  for(uint32_t i = 0; i < count; ++i) value.data.get()[i] = ntoh64(value.data.get()[i]); }
wr_payload(LongArrayTag) {
  uint64_t output[value.count];
  for(uint32_t i = 0; i < value.count; ++i) output[i] = hton64(value.data.get()[i]);
  
  uint32_t count = htonl(value.count);
  stream.write((char *)&count, 4);
  stream.write((char *)output, value.count * 8); }

#undef rd_payload
#undef wr_payload

#pragma mark - Array
template<> std::string U8Array::serialize() const {
  std::stringstream ss;
  ss << std::hex << std::setfill('0');
  for(size_t i = 0; i < count; ++i)
    ss << std::setw(2) << static_cast<unsigned>(data.get()[i]) << " ";
  return ss.str();
}

template<> void U8Array::deserialize(std::string str) {
  std::stringstream ss(str);
  size_t count = (str.length()+1) / 3; // +1 to account for a possibly missing space at the end.
  resize(count);
  
  for(size_t i = 0; i < count; ++i) {
    uint8_t byte = ((str[i*3] - '0') << 4) | (str[i*3+1] - '0');
    data.get()[i] = byte;
    
    //ss >> std::hex >> (unsigned &)data.get()[i];
  }
}

template<> std::string I32Array::serialize() const {
  std::stringstream ss;
  for(size_t i = 0; i < count; ++i) ss << data.get()[i] << " ";
  return ss.str();
}

template<> void I32Array::deserialize(std::string str) {
  size_t count = std::count(str.begin(), str.end(), ' ');
  if(*str.rbegin() != ' ')
    ++count; // Our last character is not a delimiter, so we didn't account for one number.
  
  std::stringstream ss(str);
  resize(count);
  for(size_t i = 0; i < count; ++i)
    ss >> data.get()[i];
}

template<> std::string I64Array::serialize() const {
  std::stringstream ss;
  for(size_t i = 0; i < count; ++i) ss << data.get()[i] << " ";
  return ss.str();
}

template<> void I64Array::deserialize(std::string str) {
  size_t count = std::count(str.begin(), str.end(), ' ');
  if(*str.rbegin() != ' ')
    ++count; // Our last character is not a delimiter, so we didn't account for one number.
  
  std::stringstream ss(str);
  resize(count);
  for(size_t i = 0; i < count; ++i)
    ss >> data.get()[i];
}

#pragma mark - Value serialization
// (emscripten)
// These are used to interface with JavaScript in cases
// where emscripten does not support type bindings
// (for example for arrays or 64bit integers)

template<> std::string ByteArrayTag::serializeValue() const { return value.serialize(); }
template<> void ByteArrayTag::deserializeValue(std::string str) { value.deserialize(str); }

template<> std::string IntArrayTag::serializeValue() const { return value.serialize(); }
template<> void IntArrayTag::deserializeValue(std::string str) { value.deserialize(str); }

template<> std::string LongArrayTag::serializeValue() const { return value.serialize(); }
template<> void LongArrayTag::deserializeValue(std::string str) { value.deserialize(str); }

template<> std::string LongTag::serializeValue() const { return std::to_string(value); }
template<> void LongTag::deserializeValue(std::string str) { value = std::stoll(str); }