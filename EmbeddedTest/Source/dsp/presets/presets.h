#pragma once

#include "../DirtySystem.h"

namespace DirtynthPresets
{
	inline int GetNumPresets()
	{
		return 0;
	}
	inline std::tuple<std::string, Dirtynth::DirtynthParams> GetLocalPresets(int idx)
	{
		Dirtynth::DirtynthParams params = {};
	}
}
