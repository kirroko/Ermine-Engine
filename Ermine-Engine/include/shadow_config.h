#pragma once

#define MAX_LIGHTS 32
#define NUM_CASCADES 4
#define SHADOW_MAX_LAYERS_DESIRED 16 // 32MB per layer for 4096x4096 (16 layers = 512MB) (about 4 lights with shadows, each with 4 cascades)
extern unsigned int SHADOW_MAX_LAYERS;
#define SHADOW_MAP_RESOLUTION 4096
#define SHADOW_MAP_ARRAY_LAMBDA 0.95f