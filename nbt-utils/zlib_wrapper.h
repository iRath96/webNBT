//
//  zlib_wrapper.h
//  nbt-utils
//
//  Created by Alexander Rath on 15.02.15.
//  Copyright (c) 2015 Alexander Rath. All rights reserved.
//

#ifndef __nbt_utils__zlib_wrapper__
#define __nbt_utils__zlib_wrapper__

#include <string>
std::string zlibInflate(const std::string &);
std::string zlibDeflate(const std::string &);

#endif /* defined(__nbt_utils__zlib_wrapper__) */