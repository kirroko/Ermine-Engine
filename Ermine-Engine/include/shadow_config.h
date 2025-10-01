#pragma once

#define MAX_LIGHTS 32u
#define NUM_CASCADES 4u
#define SHADOW_MAX_LAYERS_DESIRED 16u // 32MB per layer for 4096x4096 (16 layers = 512MB) (about 4 lights with shadows, each with 4 cascades)
extern unsigned int SHADOW_MAX_LAYERS; // defined in Renderer.cpp
#define SHADOW_MAP_RESOLUTION 4096u
#define SHADOW_MAP_ARRAY_LAMBDA 0.95f
#define SHADOW_MAP_REFRESH_INTERVAL_IN_FRAMES 1 // 1 in x frames