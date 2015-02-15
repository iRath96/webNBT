var TagLibrary = (function() {
  var self = this;
  
  this.tagHash = {};
  this.tagId = 0;
  
  this.typeMap = {
    'EndTag'       :  0,
    'ByteTag'      :  1,
    'ShortTag'     :  2,
    'IntTag'       :  3,
    'LongTag'      :  4,
    'FloatTag'     :  5,
    'DoubleTag'    :  6,
    'ByteArrayTag' :  7,
    'StringTag'    :  8,
    'ListTag'      :  9,
    'CompoundTag'  : 10,
    'IntArrayTag'  : 11
  };
  
  this.nameForType = function(type) {
    for(var i in self.typeMap) if(self.typeMap[i] == type) return i;
    return '?';
  };
  
  this.stringifyTag = function(tag, key) {
    var prefix = '<b>' + tag.getName() + '</b>';
    if(!tag.hasName()) prefix = (key === undefined ? 'unnamed' : '<i>' + key + '</i>');
    
    if(tag instanceof Module.CompoundTag) return prefix;
    if(tag instanceof Module.ListTag) return prefix + ' ' + pluralize(tag.getCount(), nameForType(tag.getEntryKind()) + ' entry');
    if(tag instanceof Module.IntArrayTag) return prefix + ' ' + pluralize(tag.getValue().getCount(), 'int');
    if(tag instanceof Module.ByteArrayTag) return prefix + ' ' + pluralize(tag.getValue().getCount(), 'byte');
    
    var vstr = ''+getTagValue(tag);
    if(tag instanceof Module.StringTag) vstr = '"' + vstr + '"';
    return prefix + ': ' + vstr;
  };
  
  this.createTree = function(tag, isRoot, tagKey) {
    var myTagId = ++self.tagId;
    
    if(isRoot === undefined) isRoot = true;
    if(isRoot) myTagId = 1;
    
    self.tagHash[myTagId] = tag;
    
    var children = [];
    var icon = 'nbt-icon ' + tag.constructor.name.toLowerCase();
    if(tag instanceof Module.CompoundTag) {
      var hash = tag.getValue();
      for(hash.begin(); !hash.atEnd(); hash.next()) {
        var child = self.createTree(hash.getTag(), false);
        child.data.parentTagId = myTagId;
        child.data.key = hash.getName();
        children.push(child);
      }
    } else if(tag instanceof Module.ListTag)
      for(var i = 0, j = tag.getCount(); i < j; ++i) {
        var child = self.createTree(tag.getElement(i), false, i);
        child.data.parentTagId = myTagId;
        child.data.key = i;
        children.push(child);
      }
    
    return {
      data: { tagId:myTagId, isRoot:isRoot, key:tag.getName() },
      text: stringifyTag(tag, tagKey),
      icon: icon,
      state: { opened: isRoot },
      children: children
    };
  };
  
  this.getAccessorNames = function(tag) {
    if(tag instanceof Module.LongTag || tag instanceof Module.ByteArrayTag || tag instanceof Module.IntArrayTag)
      return [ 'serializeValue', 'deserializeValue' ];
    return [ 'getValue', 'setValue' ];
  };
  
  this.getTagValue = function(tag) { return tag[self.getAccessorNames(tag)[0]](); };
  this.setTagValue = function(tag, v) { tag[self.getAccessorNames(tag)[1]](v); }
  
  this.isNumericTag = function(tag) {
    return tag.tagType() >= 1 && tag.tagType() <= 6;
  };
  
  this.isLiteralTag = function(tag) {
    return self.isNumericTag(tag) || (tag instanceof Module.StringTag);
  };
  
  /* Serialization functions for debugging */
  
  function makeString() {
    var str = Module.Tag.serialize(self.tagHash[1], -1);
    var ostr = "";
    for(var i = 0; i < str.length; ++i) {
      var chr = str.charCodeAt(i);
      ostr += "\\x" + (chr < 16 ? '0' : '') + chr.toString(16); }
    return ostr;
  }
  
  function makeStringCompressed() {
    var str = Module.Tag.serializeCompressed(self.tagHash[1], -1);
    var ostr = "";
    for(var i = 0; i < str.length; ++i) {
      var chr = str.charCodeAt(i);
      ostr += (chr < 16 ? '0' : '') + chr.toString(16) + ' '; }
    return ostr;
  }
  
  return this;
}());