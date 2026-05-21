#pragma once

#include "Wavetable.h"
#include "Filter.h"
#include "Envelope.h"

namespace Dirtynth
{
	using namespace MinusMKI;
	struct RegMutant
	{
		constexpr static int NumRegMutant = 4;
		std::vector<std::shared_ptr<TableMutant>> regMutant{
			std::make_shared<TableMutantSync<WTOscillator::TableWidth>>(),
			std::make_shared<TableMutantSelfPM<WTOscillator::TableWidth>>(),
			std::make_shared<TableMutantKickizer<WTOscillator::TableWidth>>(),
			std::make_shared<TableMutantDisperser<WTOscillator::TableWidth>>() };
		std::shared_ptr<TableMutant> operator[](std::size_t index)
		{
			return regMutant[index];
		}
		std::shared_ptr<const TableMutant> operator[](std::size_t index) const
		{
			return regMutant[index];
		}
	};
	struct RegFilter
	{
		constexpr static int NumRegMutant = 6;
		std::vector<std::shared_ptr<Filter>> regFilter{
			std::make_shared<SVFilter12dB>(),
			std::make_shared<SVFilter24dB>(),
			std::make_shared<Elliptic6order>(),
			std::make_shared<CombFilter>(),
			std::make_shared<CombFilter4Stage>(),
			std::make_shared<PhaserFilter>() };
		std::shared_ptr<Filter> operator[](std::size_t index)
		{
			return regFilter[index];
		}
		std::shared_ptr<const Filter> operator[](std::size_t index) const
		{
			return regFilter[index];
		}
	};
	struct DirtynthParams
	{
		int osc1WtPreset = 0;
		int osc2WtPreset = 0;
		float osc1WtPos = 0.0;
		float osc2WtPos = 0.0;
		float osc1MutantA[3] = { 0 };
		float osc1MutantB[3] = { 0 };
		float osc2MutantA[3] = { 0 };
		float osc2MutantB[3] = { 0 };

	};
	class Dirtynth
	{
	public:
		constexpr static int MaxPolyphony = 8;
	private:
		WavetableGenerator wtgen;
		RegMutant regMutant;
		WTOscillator osc1[MaxPolyphony], osc2[MaxPolyphony];
		RegFilter filt1[MaxPolyphony], filt2[MaxPolyphony];


	public:
	};
}