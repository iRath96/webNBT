function escapeHtml(unsafe) {
  return unsafe
       .replace(/&/g, "&amp;")
       .replace(/</g, "&lt;")
       .replace(/>/g, "&gt;")
       .replace(/"/g, "&quot;")
       .replace(/'/g, "&#039;");
}

var TagLibrary = (function() {
  var self = this;
  
  this.tagHash = {};
  this.tagId = 1;
  
  this.getRootTag = function( ) { return self.tagHash[1]; };
  this.setRootTag = function(t) { self.tagHash[1] = t;    };
  
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
    'IntArrayTag'  : 11,
    'LongArrayTag' : 12
  };
  
  this.nameForType = function(type) {
    for(var i in self.typeMap) if(self.typeMap[i] == type) return i;
    return '?';
  };
  
  this.createTree = function(tag, tagKey) {
    var isRoot = false;
    if(tag === undefined) {
      tag = self.getRootTag();
      isRoot = true;
    }
    
    var myTagId = isRoot ? 1 : ++self.tagId;
    
    self.tagHash[myTagId] = tag;
    
    var children = [];
    var icon = 'nbt-icon ' + tag.constructor.name.toLowerCase();
    if(tag instanceof Module.CompoundTag) {
      var hash = tag.getValue();
      for(hash.begin(); !hash.atEnd(); hash.next()) {
        var child = self.createTree(hash.getTag());
        child.data.parentTagId = myTagId;
        child.data.key = hash.getName();
        children.push(child);
      }
    } else if(tag instanceof Module.ListTag)
      for(var i = 0, j = tag.getCount(); i < j; ++i) {
        var child = self.createTree(tag.getElement(i), i);
        child.data.parentTagId = myTagId;
        child.data.key = i;
        children.push(child);
      }
    
    return {
      data: { tagId:myTagId, isRoot:isRoot, key:tag.getName() },
      text: tag.toString(tagKey),
      icon: icon,
      state: { opened: isRoot },
      children: children
    };
  };
  
  function pluralize(quantity, name) {
    if(quantity != 1)
      if(name.substr(-1) == 'y') name = name.substr(0, name.length - 1) + 'ies';
      else name += 's';
    return quantity + ' ' + name;
  }
  
  this.setupTagMethods = function() {
    var _ = Module.Tag.prototype;
    
    _.isEditable = function() { return !(this instanceof Module.ListTag || this instanceof Module.CompoundTag); };
    _.isNumeric = function() { return this.tagType() >= 1 && this.tagType() <= 6; };
    _.usesNumericValues = function() { return this.isNumeric() && !(this instanceof Module.LongTag); };
    _.needsSerialization = function() { return (this instanceof Module.LongTag || this instanceof Module.ByteArrayTag || this instanceof Module.IntArrayTag || this instanceof Module.LongArrayTag); };
    
    _.getJSValue = function( ) { return this.needsSerialization() ? this.serializeValue()    : this.getValue( ); };
    _.setJSValue = function(v) { return this.needsSerialization() ? this.deserializeValue(v) : this.setValue(v); };
    
    _.toString = function(key) {
      var prefix = '<b>' + escapeHtml(this.getName()) + '</b>';
      if(!this.hasName()) prefix = (key !== '' && key !== undefined ? '<i>' + key + '</i>' : '(unnamed)');
      
      if(this instanceof Module.CompoundTag ) return prefix;
      if(this instanceof Module.ListTag     ) return prefix + ' ' + pluralize(this.getCount(), nameForType(this.getEntryKind()) + ' entry');
      if(this instanceof Module.IntArrayTag ) return prefix + ' ' + pluralize(this.getValue().getCount(), 'int');
      if(this instanceof Module.LongArrayTag) return prefix + ' ' + pluralize(this.getValue().getCount(), 'long');
      if(this instanceof Module.ByteArrayTag) return prefix + ' ' + pluralize(this.getValue().getCount(), 'byte');
      
      var vstr = '' + this.getJSValue();
      if(this instanceof Module.StringTag) vstr = '"' + vstr + '"';
      return prefix + ': ' + escapeHtml(vstr);
    };
  };
  
  /* Serialization functions for debugging */
  
  this.makeString = function() {
    var str = Module.Tag.serialize(self.tagHash[1], -1);
    var ostr = "";
    for(var i = 0; i < str.length; ++i) {
      var chr = str.charCodeAt(i);
      ostr += "\\x" + (chr < 16 ? '0' : '') + chr.toString(16); }
    return ostr;
  };
  
  this.makeCompressedString = function() {
    var str = Module.Tag.serializeCompressed(self.tagHash[1], -1);
    var ostr = "";
    for(var i = 0; i < str.length; ++i) {
      var chr = str.charCodeAt(i);
      ostr += (chr < 16 ? '0' : '') + chr.toString(16) + ' '; }
    return ostr;
  };
  
  return this;
}());