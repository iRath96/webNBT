const DEP_EMSCRIPTEN = 'emscripten';
const DEP_WINDOW     = 'window';

var Dependencies = (function() {
  var queue = [];
  var callbacks = [];
  
  function fireReady() {
    callbacks.forEach(function(cb) { cb(); });
  }
  
  this.onReady = function(cb) { callbacks.push(cb); };
  this.waitFor = function(name) { queue.push(name); };
  this.handleLoaded = function(name) {
    var index = queue.indexOf(name);
    if(index !== -1) queue.splice(index, 1);
    if(queue.length == 0) fireReady();
  };
  
  return this;
})();

Dependencies.waitFor(DEP_EMSCRIPTEN);
Dependencies.waitFor(DEP_WINDOW);

Dependencies.onReady(function() {
  TagLibrary.setupTagMethods();
  App.bootstrap();
});

// - - -

var App = (function() {
  this.runsInSafari = null;
  
  this.selectedTag = null;
  this.hexviewShown = true;
  
  const DATAMODE_COMPRESSED = 'compressed';
  const DATAMODE_UNCOMPRESSED = 'uncompressed';
  
  this.dataMode = null;
  
  // jsTree variables
  this.treeElement = null;
  this.treeRef = null;
  this.treeData = "";
  
  // methods
  this.bootstrap = function() {
    App.checkBrowser();
    App.setupJSTree();
  };
  
  this.checkBrowser = function() {
    var please_switch = "\nPlease use a recent version of Firefox, Chrome or Opera.";
    try {
      !!new Blob; // check if blobs are supported.
    } catch(e) {
      try {
        alert("Your browser is behind." + please_switch);
      } catch(e) {
        document.write("1) Your browser sucks,\n2) It blocks alert.");
      }
    }
    
    App.runsInSafari = navigator.userAgent.indexOf("Safari/") > -1;
    if(navigator.userAgent.indexOf("Chrome/") > -1) runsInSafari = false;
    if(navigator.userAgent.indexOf("OPR/") > -1) runsInSafari = false;
    
    //if(App.runsInSafari)
    //  alert("Safari does not properly support file downloads.\nYou can view and edit files, but saving them might not work properly." + please_switch);
  };
  
  this.refreshTree = function() {
    App.treeData = TagLibrary.createTree();
    App.treeRef.refresh();
    
    App.updateHexview();
  };
  
  this.save = function() {
    var type = "application/octet-stream";
    if(App.runsInSafari) type = "application/binary"; // Safari dislikes official MIMEs.
    
    var data = Module.Tag[App.dataMode == DATAMODE_COMPRESSED ? 'serializeCompressed' : 'serialize'](TagLibrary.tagHash[1], -1);
    var bytes = new Uint8Array(data.length);
    for(var i = 0; i < data.length; ++i) bytes[i] = data.charCodeAt(i);
    
    saveAs(new Blob([ bytes ], { type:type }), lastFilename);
  };
  
  this.toggleHexview = function(btn) {
    App.hexviewShown = !document.body.classList.toggle("hide-hexview");
    if(App.hexviewShown) App.updateHexview();
    btn.value = hexviewShown ? '>' : '<';
  };
  
  function tryMode(data, mode, isNamed) {
    var fn = mode == DATAMODE_COMPRESSED ? 'deserializeCompressed' : 'deserialize';
    try {
      TagLibrary.setRootTag(Module.Tag[fn](data, isNamed, -1));
      this.dataMode = mode;
      
      App.refreshTree();
      
      return true;
    } catch(e) {
      console.log(e);
      return false;
    }
  }
  
  this.loadData = function(data) {
    if(tryMode(data, DATAMODE_COMPRESSED, true)) return;
    if(tryMode(data, DATAMODE_COMPRESSED, false)) return;
    if(tryMode(data, DATAMODE_UNCOMPRESSED, true)) return;
    if(tryMode(data, DATAMODE_UNCOMPRESSED, false)) return;
    
    if(confirm("Could not parse your file.\nIf you are sure this is a NBT-file you should report a bug.\n\nClick OK to contact the developer."))
      location.href = "http://irath96.github.io/contact/";
  };
  
  function nodeEditValue(node, isNew) {
    var tag = TagLibrary.tagHash[node.data.tagId];
    
    if(tag.isEditable()) {
      var value = prompt("New value?", tag.getJSValue());
      if((value === null || value === '') && !isNew) return;
      
      if(tag.usesNumericValues()) value = Number(value);
      tag.setJSValue(value);
      
      App.updateHexview();
    }
    
    App.treeRef.set_text(node, tag.toString(node.data.key));
  }
  
  function actionsForNode(node) {
    var tag = TagLibrary.tagHash[node.data.tagId];
    var actions = {};
    
    if(tag instanceof Module.CompoundTag) {
      var makeAction = function(type) {
        return function(obj) {
          var myTagId = ++tagId;
          TagLibrary.tagHash[tagId] = Module.makeTag(type);
          
          var icon = 'nbt-icon ' + TagLibrary.tagHash[tagId].constructor.name.toLowerCase();
          var nnode = App.treeRef.create_node(node, {
            icon:icon,
            text:"",
            data:{ tagId:myTagId, isRoot:false, parentTagId:node.data.tagId }
          }, "first");
          
          App.treeRef.edit(nnode);
        };
      };
      
      var submenu = {};
      for(var type in TagLibrary.typeMap)
        if(TagLibrary.typeMap.hasOwnProperty(type))
          submenu[type] = {
            "icon" : 'nbt-icon ' + type.toLowerCase(),
            "label" : type,
            "action" : makeAction(TagLibrary.typeMap[type])
          };
      
      actions["create"] = {
        "label": "New",
        "submenu" : submenu
      };
    } else if(tag instanceof Module.ListTag) {
      actions["add"] = {
        "label": "Add",
        "action": function() {
          tag.addElement();
          
          App.refreshTree();
          App.updateHexview();
        }
      };
      
      var makeAction = function(type) {
        return function(obj) {
          var tagWasEmpty = tag.getCount() == 0;
          if(!tagWasEmpty) tag.clear();
          tag.setEntryKind(type);
          
          if(tagWasEmpty) App.treeRef.set_text(node, tag.toString(node.data.key));
          else App.refreshTree();
          
          App.updateHexview();
        };
      };
      
      var submenu = {};
      for(var type in TagLibrary.typeMap)
        if(TagLibrary.typeMap.hasOwnProperty(type))
          submenu[type] = {
            "icon" : 'nbt-icon ' + type.toLowerCase(),
            "label" : type,
            "action" : makeAction(TagLibrary.typeMap[type])
          };
      
      actions["change_element_type"] = {
        "separator_after" : true,
        "label": "Change Element Type",
        "submenu" : submenu
      };
    } else
      actions["edit"] = {
        "label": "Edit value...",
        "action": function(obj) {
          nodeEditValue(node);
        }
      };
    
    if(tag instanceof Module.ByteTag) {
      var setByte = function(value) {
        tag.setValue(value);
        App.treeRef.set_text(node, tag.toString(node.data.key));
        App.updateHexview();
      }
      
      actions["set_true"] = {
        "separator_before" : true,
        "label": "Set True",
        "action": function() { setByte(1) }
      };
      
      actions["set_false"] = {
        "separator_after" : true,
        "label": "Set False",
        "action": function() { setByte(0) }
      }
    }
    
    /* Change type */
    
    var parentTag = TagLibrary.tagHash[node.data.parentTagId];
    if(parentTag instanceof Module.CompoundTag || parentTag === undefined) {
      var makeAction = function(type) {
        return function(obj) {
          var ntag = Module.makeTag(type);
          TagLibrary.tagHash[node.data.tagId] = ntag;
          
          if(parentTag !== undefined)
            parentTag.getValuePtr().set(node.data.key, ntag);
          
          ntag.setHasName(true);
          ntag.setName(node.data.key);
          
          var icon = 'nbt-icon ' + ntag.constructor.name.toLowerCase();
          node.icon = icon;
          
          App.treeRef.set_text(node, ntag.toString(node.data.key));
          
          if(node.children.length > 0) App.refreshTree();
          
          App.updateHexview();
        };
      };
      
      var submenu = {};
      for(var type in TagLibrary.typeMap)
        if(TagLibrary.typeMap.hasOwnProperty(type))
          submenu[type] = {
            "icon" : 'nbt-icon ' + type.toLowerCase(),
            "label" : type,
            "action" : makeAction(TagLibrary.typeMap[type])
          };
      
      actions["change_type"] = {
        "separator_after" : true,
        "label": "Change Type",
        "submenu" : submenu
      };
    }
    
    /* Other stuff */
    
    if(tag.hasName() || node.data.isRoot) {
      actions["rename"] = {
        "label": (tag.hasName() ? "Rename..." : "Name..."),
        "action": function(obj) {
          App.treeRef.settings.core.force_text = true;
          App.treeRef.edit(node, TagLibrary.tagHash[node.data.tagId].getName());
        }
      };
      
      if(!tag.hasName() || tag.getName() != '')
        actions["rename_empty"] = {
          "label": (tag.hasName() ? "Rename to \"\"" : "Name \"\""),
          "action": function(obj) {
            var tag = TagLibrary.tagHash[node.data.tagId];
            tag.setHasName(true);
            tag.setName("");
            
            var parentTag = TagLibrary.tagHash[node.data.parentTagId];
            if(parentTag) {
              var phash = parentTag.getValuePtr();
              
              phash.rename(node.data.key, "");
              node.data.key = "";
            }
            
            App.treeRef.set_text(node, tag.toString(node.data.key));
            
            App.updateHexview();
          }
        };
    }
    
    if(node.data.isRoot)
      if(tag.hasName())
        actions["set_unnamed"] = {
          "label": "Set unnamed",
          "action": function(obj) {
            var tag = TagLibrary.tagHash[node.data.tagId];
            tag.setHasName(false);
            
            App.treeRef.set_text(node, tag.toString(node.data.key));
            
            App.updateHexview();
          }
        };
      else;
    else
      actions["delete"] = {
        "separator_before": true,
        "label": "Delete",
        "action": function(obj) {
          var parentTag = TagLibrary.tagHash[node.data.parentTagId];
          if(parentTag instanceof Module.CompoundTag) {
            parentTag.getValuePtr().remove(node.data.key);
            App.treeRef.delete_node(node);
          } else {
            // ListTag
            parentTag.remove(node.data.key);
            App.refreshTree();
          }
          
          App.updateHexview();
        }
      };
    
    return actions;
  }
  
  function setupSearchField() {
    var to = false;
    $('#search_field').keyup(function () {
      if(to) { clearTimeout(to); }
      to = setTimeout(function () {
        var v = $('#search_field').val();
        $('#tree').jstree(true).search(v);
      }, 250);
    });
  }
  
  this.setupJSTree = function() {
    setupSearchField();
    
    (App.treeElement = $('#tree')).jstree({
      "plugins" : [ "contextmenu", "search" ], // , "dnd" ],
      'core' : {
        "check_callback" : true,
        'data' : function(obj, cb) { return cb(treeData); }
      },
      "contextmenu": {
        "items": actionsForNode
      }
    });
    
    App.treeRef = App.treeElement.jstree(true);
    
    App.treeElement.bind("dblclick.jstree", function(event) {
      var nodeId = $(event.target).closest("li")[0].id;
      var node = App.treeRef.get_node(nodeId);
      
      nodeEditValue(node);
    });
    
    App.treeElement.on('changed.jstree', function (e, data) {
      var i, j, r = [];
      for(i = 0, j = data.selected.length; i < j; i++)
        r.push(data.instance.get_node(data.selected[i]));
      
      var tag = undefined;
      if(r.length > 0) tag = TagLibrary.tagHash[r[0].data.tagId];
      
      App.selectedTag = tag;
      App.updateHexview();
    });
    
    App.treeElement.bind('rename_node.jstree', function(r, e) {
      App.treeRef.settings.core.force_text = false;
      
      var tag = TagLibrary.tagHash[e.node.data.tagId];
      
      tag.setHasName(true);
      tag.setName(e.text);
      
      if(!e.node.data.isRoot) {
        // A new node was created and named
        
        dbgn = e.node;
        dbgt = tag;
        
        var parentTag = TagLibrary.tagHash[e.node.data.parentTagId];
        var phash = parentTag.getValuePtr();
        
        if(e.node.data.key) phash.rename(e.node.data.key, e.text);
        else { // This is a freshly created node
          phash.set(e.text, tag);
          nodeEditValue(e.node, true);
        }
        
        e.node.data.key = e.text;
        
        rhash = phash;
      }
      
      App.treeRef.set_text(e.node, tag.toString(e.node.data.key));
      
      App.updateHexview();
    });
  };
  
  // - - -
  
  this.updateHexview = function() {
    if(!hexviewShown) return;
    
    var firstHighlightedLine = false;
    
    var hexStr = "";
    var rawStr = Module.Tag.serialize(TagLibrary.tagHash[1], -1);
    for(var i = 0; i < rawStr.length; ++i) {
      var chr = rawStr.charCodeAt(i);
      hexStr += (chr < 16 ? '0' : '') + chr.toString(16) + ' ';
    }
    
    var bytesPerRow = 12;
    
    var selA = App.selectedTag ? App.selectedTag.getStartIndex() : 0;
    var selB = App.selectedTag ? App.selectedTag.getEndIndex()   : 0;

    var leftStr = new HighlightedString(hexStr, [ selA * 3, selB * 3 ]);
    var rightStr = new HighlightedString(rawStr.replace(/[^\x20-\x7e]/g, '.'), [ selA, selB ]);
    
    var str2 = "";
    var rows = Math.ceil(rawStr.length / bytesPerRow);
    for(var i = 0; i < rows; ++i) {
      var hexPart = leftStr.substring(i * bytesPerRow * 3, (i+1) * bytesPerRow   * 3);
      var rhPart  = rightStr.substring(i * bytesPerRow, (i+1) * bytesPerRow);
      if(firstHighlightedLine === false && rhPart.indexOf('<font') !== -1)
        firstHighlightedLine = i;
      
      var addr = (i*bytesPerRow).toString(16);
      while(addr.length < 4) addr = '0' + addr;
      str2 += '0x' + addr + ': ' + hexPart + '| ' + rhPart + "\n";
    }
    
    var pre = document.querySelector('pre');
    pre.innerHTML = str2;
    if(firstHighlightedLine !== false) pre.scrollTop = Math.floor((pre.scrollHeight - 20) * firstHighlightedLine / rows);
  };
  
  return this;
})();