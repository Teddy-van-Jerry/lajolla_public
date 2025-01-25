#pragma once

#include "image.h"
#include "lajolla.h"
#include <memory>

struct Scene;

Image3 render(const Scene& scene);
