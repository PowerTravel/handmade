#pragma once

#include "bitmap.h"
#include "data_containers.h"

struct component_sprite_animation
{
  u32 MaterialHande;
  bitmap* Bitmap;
  hash_map< list <m4> > Animation;
  list<m4>* ActiveSeries;
  b32 InvertX;
};
