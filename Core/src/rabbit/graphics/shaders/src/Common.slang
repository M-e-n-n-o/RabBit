module Common;

#include "../shared/Common.h"

// Global constant buffers
// ---------------------------------------------------------------
public uniform ConstantBuffer<FrameConstants> g_FC;

// Static samplers
// ---------------------------------------------------------------
public uniform SamplerState g_ClampAnisoSampler;
public uniform SamplerState g_ClampPointSampler;
public uniform SamplerState g_ClampLinearSampler;
public uniform SamplerState g_WrapAnisoSampler;