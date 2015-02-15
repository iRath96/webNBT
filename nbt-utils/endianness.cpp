//
//  endianness.cpp
//  nbt-utils
//
//  Created by Alexander Rath on 15.02.15.
//  Copyright (c) 2015 Alexander Rath. All rights reserved.
//

#include "endianness.h"

uint64_t htonll_impl(uint64_t input) {
  uint64_t rval = 42;
  if(*(char *)&rval != 42) return input;
  
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