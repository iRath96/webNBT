//
//  nbt_utils.h
//  nbt-utils
//
//  Created by Alexander Rath on 15.02.15.
//  Copyright (c) 2015 Alexander Rath. All rights reserved.
//

#ifndef __nbt_utils__nbt_utils__
#define __nbt_utils__nbt_utils__

#include <stdint.h>

#include <vector>
#include <map>
#include <sstream>
#include <iomanip>

#include "endianness.h"
#include "zlib_wrapper.h"

namespace nbt {
  struct TagType {
#ifdef EMSCRIPTEN
    // (emscripten)
    // Using chars instead of an enum-values makes bindings easier.
    
    typedef char Enum;
    enum Values : char {
#else
    enum Enum : char {
#endif
      End       = 0,
      Byte      = 1,
      Short     = 2,
      Int       = 3,
      Long      = 4,
      Float     = 5,
      Double    = 6,
      ByteArray = 7,
      String    = 8,
      List      = 9,
      Compound  = 10,
      IntArray  = 11,
      LongArray = 12,
      
      Unknown = -1 //!< Placeholder for when the type needs to be read
    };
  };
  
  class Tag;
  Tag *makeTag(TagType::Enum); //!< Factory method
  
  class Tag {
  public:
    virtual ~Tag() {}

    std::string name;  //!< The name for this tag
    bool hasName;      //!< Whether this tag was named
    size_t startIndex, //!< The stream position this tag started on (set on read/write)
           endIndex;   //!< The stream position this tag ended on (set on read/write)
    
    // Emscripten interface
    void setName(std::string name) { this->name = name; }
    std::string getName() const { return name; }
    
    size_t getStartIndex() const { return startIndex; }
    size_t getEndIndex() const { return endIndex; }
    
    void setHasName(bool hn) { hasName = hn; }
    bool getHasName() const { return hasName; }
    
    virtual TagType::Enum tagType() const = 0;
    
    // Read
    static Tag *read(std::istream &stream, bool withName = true, TagType::Enum type = TagType::Unknown);
    static Tag *deserialize(std::string input, bool withName = true, TagType::Enum type = TagType::Unknown) {
      std::stringstream stream(input);
      return read(stream, withName, type);
    }
    
    static Tag *deserializeCompressed(std::string input, bool withName = true, TagType::Enum type = TagType::Unknown) {
      std::stringstream stream(zlibInflate(input));
      return read(stream, withName, type);
    }
    
    // Write
    static void write(Tag *tag, std::ostream &stream, TagType::Enum type = TagType::Unknown);
    static void write(Tag *tag, std::ostream &stream, const std::string &name, TagType::Enum type = TagType::Unknown);
    
    // I am really not happy about this, but embind wants us to return std::basic_string<unsigned char> here,
    // because otherwise it will assume the output is UTF-8 encoded and mess up our data.
    static std::basic_string<unsigned char> serialize(Tag *tag, TagType::Enum type = TagType::Unknown) {
      std::stringstream stream;
      if(tag->hasName) write(tag, stream, tag->name, type);
      else write(tag, stream, type);
      
      auto c1 = stream.str();
      return *(std::basic_string<unsigned char> *)&c1;
    }
    
    static std::basic_string<unsigned char> serializeCompressed(Tag *tag, TagType::Enum type = TagType::Unknown) {
      auto c1 = serialize(tag, type);
      auto c2 = zlibDeflate(*(std::string *)&c1);
      return *(std::basic_string<unsigned char> *)&c2;
    }
    
    virtual void readPayload(std::istream &stream) = 0;
    virtual void writePayload(std::ostream &stream) const = 0;
  };
  
  class EndTag : public Tag {
  public:
    virtual TagType::Enum tagType() const { return TagType::End; }
    virtual void readPayload(std::istream &stream) {}
    virtual void writePayload(std::ostream &stream) const {}
  };
  
  template<typename T, TagType::Enum type>
  class PrimitiveTag : public Tag {
  public:
    T value;
    
    T getValue() { return value; }
    T *getValuePtr() { return &value; }
    
    void setValue(const T value) { this->value = value; }
    
    virtual std::string serializeValue() const;
    virtual void deserializeValue(std::string str);
    
    PrimitiveTag<T>() {}
    
    PrimitiveTag<T>(const T &value) : value(value) {}
    PrimitiveTag<T>(const T  value) : value(value) {}
    
    virtual TagType::Enum tagType() const { return type; }
    
    virtual void readPayload(std::istream &stream);
    virtual void writePayload(std::ostream &stream) const;
  };
  
  typedef PrimitiveTag<std::vector<std::shared_ptr<Tag>>, TagType::List> ListTagBase;
  
  class ListTag : public ListTagBase {
  public:
    TagType::Enum entryKind;
    static void read(std::istream &stream, ListTag &tag);
    
    virtual void readPayload(std::istream &stream);
    virtual void writePayload(std::ostream &stream) const;
    
    // Emscripten interface
    TagType::Enum getEntryKind() const { return entryKind; }
    void setEntryKind(TagType::Enum k) { entryKind = k; }
    
    void clear() { value.clear(); }
    
    Tag *getElement(size_t i) { return value[i].get(); }
    Tag *addElement() {
      Tag *t = makeTag(entryKind);
      value.push_back(std::shared_ptr<Tag>(t));
      return t;
    }
    
    void removeElement(size_t i) { value.erase(value.begin()+i); }
    
    size_t getCount() { return value.size(); }
  };
  
#pragma mark - Array
  
  template<typename T>
  struct Array {
    std::shared_ptr<T> data;
    size_t count = 0;
    
    // Emscripten interface
    
    T getElement(size_t i) const { return data.get()[i]; }
    void setElement(size_t i, T value) { data.get()[i] = value; }
    
    std::string serialize() const;
    void deserialize(std::string value);
    
    size_t getCount() const { return count; }
    void resize(size_t count) {
      T *newData = (T *)malloc(sizeof(T) * count);
      this->data.reset(newData);
      this->count = count;
    }
  };
  
  typedef Array<uint8_t> U8Array;
  typedef Array<int32_t> I32Array;
  typedef Array<int64_t> I64Array;
  
#pragma mark - Hash
  
  typedef std::map<std::string, std::shared_ptr<Tag>> TagHashBase;
  struct TagHash : public TagHashBase {
    // Emscripten interface
    
    typename TagHashBase::iterator jsIterator;
    
    void jsBegin() { jsIterator = begin(); }
    bool jsAtEnd() { return jsIterator == end(); }
    void jsNext() { ++jsIterator; }
    
    std::string getName() { return jsIterator->first; }
    Tag *getTag() { return jsIterator->second.get(); }
    
    void jsRename(std::string oldKey, std::string newKey) {
      std::shared_ptr<Tag> tag = (*this)[oldKey];
      erase(oldKey);
      
      (*this)[newKey] = tag;
    }
    
    void jsRemove(std::string key) { erase(key); }
    void jsSet(std::string key, Tag *tag) { (*this)[key] = std::shared_ptr<Tag>(tag); }
  };
  
#pragma mark - Typedefs
  
  typedef PrimitiveTag<int8_t      , TagType::Byte      > ByteTag;
  typedef PrimitiveTag<int16_t     , TagType::Short     > ShortTag;
  typedef PrimitiveTag<int32_t     , TagType::Int       > IntTag;
  typedef PrimitiveTag<int64_t     , TagType::Long      > LongTag;
  typedef PrimitiveTag<float       , TagType::Float     > FloatTag;
  typedef PrimitiveTag<double      , TagType::Double    > DoubleTag;
  typedef PrimitiveTag<U8Array     , TagType::ByteArray > ByteArrayTag;
  typedef PrimitiveTag<std::string , TagType::String    > StringTag;
  typedef PrimitiveTag<TagHash     , TagType::Compound  > CompoundTag;
  typedef PrimitiveTag<I32Array    , TagType::IntArray  > IntArrayTag;
  typedef PrimitiveTag<I64Array    , TagType::LongArray > LongArrayTag;
  
#pragma mark - Value serialization
  // (emscripten)
  // These are used to interface with JavaScript in cases
  // where emscripten does not support type bindings
  // (for example for arrays or 64bit integers)
  
  // All non-specalizied Tags do not support serialization, so put stubs.
  template<typename T, TagType::Enum type> std::string PrimitiveTag<T, type>::serializeValue() const { return ""; }
  template<typename T, TagType::Enum type> void PrimitiveTag<T, type>::deserializeValue(std::string) {}
}

#endif /* defined(__nbt_utils__nbt_utils__) */