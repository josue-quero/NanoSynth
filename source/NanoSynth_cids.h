//------------------------------------------------------------------------
// Copyright(c) 2022 quero.
//------------------------------------------------------------------------

#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace Quero {
//------------------------------------------------------------------------
static const Steinberg::FUID kNanoSynthProcessorUID (0xCD89DB86, 0x6EE05237, 0x96F5D7A5, 0xBAAF60DD);
static const Steinberg::FUID kNanoSynthControllerUID (0x1929149B, 0x92185FCD, 0xA1873114, 0x6BD4C756);

#define NanoSynthVST3Category "Instrument"

//------------------------------------------------------------------------
} // namespace Quero
