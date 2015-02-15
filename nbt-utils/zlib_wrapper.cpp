//
//  zlib_wrapper.cpp
//  nbt-utils
//
//  Created by Alexander Rath on 15.02.15.
//  Copyright (c) 2015 Alexander Rath. All rights reserved.
//

#include "zlib_wrapper.h"

#include <zlib.h>
#include <string>

// based on http://www.zlib.net/zpipe.c
std::string zlibInflate(const std::string &input) {
  z_stream strm;
  const unsigned char *in = (const unsigned char *)input.c_str();
  
#define CHUNK (256<<10)
  std::string out = "";
  unsigned char out_buffer[CHUNK];
  
  // allocate inflate state
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  
  int zret = inflateInit2(&strm, MAX_WBITS | 16);
  //int zret = inflateInit(&strm);
  if(zret != Z_OK) throw "zlib inflateInit failed.";
  
  do {
    strm.avail_in = (uInt)input.length();
    strm.next_in = const_cast<unsigned char *>(in); // Urgh.
    
    do {
      strm.avail_out = CHUNK;
      strm.next_out = out_buffer;
      
      zret = inflate(&strm, Z_NO_FLUSH);
      if(zret == Z_STREAM_ERROR) throw "zlib inflate stream error.";
      
      switch(zret) {
        case Z_NEED_DICT: zret = Z_DATA_ERROR;
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
          inflateEnd(&strm);
          throw "zlib inflate error.";
      }
      
      int have = CHUNK - strm.avail_out;
      out.append((char *)out_buffer, have);
    } while(strm.avail_out == 0);
  } while(zret != Z_STREAM_END);
  inflateEnd(&strm);
  
  return out;
}

std::string zlibDeflate(const std::string &input) {
  z_stream strm;
  const unsigned char *in = (const unsigned char *)input.c_str();
  
  std::string out = "";
  unsigned char out_buffer[CHUNK];
  
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  
  int zret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, MAX_WBITS | 16, 8, Z_DEFAULT_STRATEGY);
  // deflateInit(&strm, level);
  if(zret != Z_OK) throw "zlib inflateInit failed.";
  
  int flush;
  
  do {
    strm.avail_in = (uInt)input.length();
    strm.next_in = const_cast<unsigned char *>(in); // Urgh.
    
    do {
      strm.avail_out = CHUNK;
      strm.next_out = out_buffer;
      
      flush = Z_FINISH; // Z_NO_FLUSH;
      zret = deflate(&strm, flush);
      if(zret == Z_STREAM_ERROR) throw "zlib inflate stream error.";
      
      int have = CHUNK - strm.avail_out;
      out.append((char *)out_buffer, have);
    } while(strm.avail_out == 0);
    
    if(strm.avail_in != 0) throw "zlib avail_in was above zero";
  } while(flush != Z_FINISH);
  
  if(zret != Z_STREAM_END)
    throw "zlib did not reach end of stream";
  
  (void)deflateEnd(&strm);
  return out;
}

#undef CHUNK