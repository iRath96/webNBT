//
//  endianness.h
//  nbt-utils
//
//  Created by Alexander Rath on 15.02.15.
//  Copyright (c) 2015 Alexander Rath. All rights reserved.
//

#ifndef __nbt_utils__endianness__
#define __nbt_utils__endianness__

#include <arpa/inet.h>
#include <stdint.h>

#ifndef htonll
uint64_t htonll_impl(uint64_t);
#define htonll htonll_impl
#define ntohll htonll_impl
#endif

#endif /* defined(__nbt_utils__endianness__) */