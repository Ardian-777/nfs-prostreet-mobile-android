#version 450
#extension GL_EXT_control_flow_attributes : require
#extension GL_EXT_samplerless_texture_functions : require
#extension GL_GOOGLE_include_directive : require

// The same Edge Direction reconstruction with Plus final-pass 8bpc dither.
#define XE_GUEST_OUTPUT_DITHER 1
#include "guest_output_sgsr_edge_direction.glsli"
