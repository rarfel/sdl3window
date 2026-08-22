#pragma once
#include "window.h"

template <typename T>
struct Color
{
  T red;
  T green;
  T blue;
  T alpha;
};

void DrawBackground(SDLWindowState state, Color<float> backgroundColor);
