#pragma once
#include "window.h"
struct Color
{
  float red;
  float green;
  float blue;
  float alpha;
};

void DrawBackground(SDLWindowState state, Color backgroundColor);
