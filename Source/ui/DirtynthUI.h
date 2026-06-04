#pragma once

#include <cstring>

#include <JuceHeader.h>
#include "LM_slider.h"
#include "LMKnobDirect.h"
#include "../dsp/DirtySystem.h"
#include "../dsp/DirtyParams.h"

using namespace Dirtynth;

namespace DirtynthUIHelpers
{
	using ParamDesc = DirtynthParamSystem::ParamDesc;

	static constexpr const char* EnvelopeModeNames[] = {
		"PolyReset",
		"PolyNoReset",
		"GlobalNoReset",
		"GlobalFirst",
		"GlobalAny"
	};

	inline int ClampInt(int value, int minValue, int maxValue)
	{
		if (value < minValue) return minValue;
		if (value > maxValue) return maxValue;
		return value;
	}

	template <int NumNames>
	inline void AddNameItems(LMCombox& combo, const char* const (&names)[NumNames])
	{
		for (int i = 0; i < NumNames; ++i)
			combo.addItem(names[i], i + 1);
	}

	inline void LinkIndexedCombo(LMCombox& combo, float& param, int itemCount, std::function<void()> onParamChanged = {})
	{
		const int selectedIndex = ClampInt(static_cast<int>(param), 0, itemCount - 1);
		combo.setSelectedID(selectedIndex + 1);

		float* linkedParam = &param;
		combo.setChangedCallback([linkedParam, itemCount, onParamChanged](int selectedID)
			{
				const int selectedIndex = ClampInt(selectedID - 1, 0, itemCount - 1);
				*linkedParam = static_cast<float>(selectedIndex);
				if (onParamChanged)
					onParamChanged();
			});
	}

	inline ParamDesc* FindDescByID(std::vector<ParamDesc>& descs, int id)
	{
		if (id <= 0 || id > static_cast<int>(descs.size()))
			return nullptr;
		return &descs[id - 1];
	}

	inline ParamDesc* FindDescByName(DirtynthParamSystem& paramTools,
		std::vector<ParamDesc>& descs, const std::string& indexName)
	{
		return FindDescByID(descs, paramTools.SearchID(indexName));
	}

	inline void ApplyKnobFeelRule(LMKnobDirect& knob, const ParamDesc& desc)
	{
		LMKnobDirect::KnobFeelRule rule;

		switch (desc.paramType)
		{
		case DirtynthParamSystem::Integer:
			rule.sliderMin = desc.minValue;
			rule.sliderMax = desc.maxValue;
			rule.sliderDefault = desc.defValue;
			rule.interval = 1.0;
			rule.SliderToValue = [](float x) { return x; };
			rule.ValueToSlider = [](float x) { return x; };
			knob.SetKnobFeelRule(rule);
			break;
		case DirtynthParamSystem::NyquistFreqExp:
			rule.sliderDefault = Dirtynth::CutoffToParam(desc.defValue, desc.minValue, desc.maxValue);
			rule.SliderToValue = [minValue = desc.minValue, maxValue = desc.maxValue](float x)
				{
					return Dirtynth::ParamToCutoff(x, minValue, maxValue);
				};
			rule.ValueToSlider = [minValue = desc.minValue, maxValue = desc.maxValue](float x)
				{
					return Dirtynth::CutoffToParam(x, minValue, maxValue);
				};
			knob.SetKnobFeelRule(rule);
			break;
		case DirtynthParamSystem::FilterQ:
			rule.sliderDefault = Dirtynth::ResoToParam(desc.defValue, desc.minValue, desc.maxValue);
			rule.SliderToValue = [minValue = desc.minValue, maxValue = desc.maxValue](float x)
				{
					return Dirtynth::ParamToReso(x, minValue, maxValue);
				};
			rule.ValueToSlider = [minValue = desc.minValue, maxValue = desc.maxValue](float x)
				{
					return Dirtynth::ResoToParam(x, minValue, maxValue);
				};
			knob.SetKnobFeelRule(rule);
			break;
		case DirtynthParamSystem::TimeMsExp:
			rule.sliderDefault = Dirtynth::EnveTimeToParam(desc.defValue, 9.0f, desc.maxValue);
			rule.SliderToValue = [maxValue = desc.maxValue](float x)
				{
					return Dirtynth::ParamToEnveTime(x, 9.0f, maxValue);
				};
			rule.ValueToSlider = [maxValue = desc.maxValue](float x)
				{
					return Dirtynth::EnveTimeToParam(x, 9.0f, maxValue);
				};
			knob.SetKnobFeelRule(rule);
			break;
		case DirtynthParamSystem::Linear:
		default:
			knob.ClearKnobFeelRule();
			break;
		}
	}

	inline void BindKnob(LMKnobDirect& knob,
		DirtynthParamSystem& paramTools,
		DirtynthParams& params,
		std::vector<ParamDesc>& descs,
		const std::string& indexName,
		std::function<void()> onParamChanged = {})
	{
		const int id = paramTools.SearchID(indexName);
		ParamDesc* desc = FindDescByID(descs, id);
		if (desc == nullptr)
			return;

		float& value = paramTools.GetParamRefByID(params, id);
		knob.setText(desc->name);
		ApplyKnobFeelRule(knob, *desc);
		knob.ParamLink(desc->minValue, desc->maxValue, desc->defValue, value,
			[&paramTools, &params, id, onParamChanged](float x)
			{
				paramTools.GetParamRefByID(params, id) = x;
				if (onParamChanged)
					onParamChanged();
			});
	}

	inline void SetKnobValue(LMKnobDirect& knob,
		DirtynthParamSystem& paramTools,
		DirtynthParams& params,
		const std::string& indexName)
	{
		const int id = paramTools.SearchID(indexName);
		if (id <= 0)
			return;
		knob.setValue(paramTools.GetParamRefByID(params, id));
	}

	inline void AddModTargetItems(LMCombox& combo, const std::vector<ParamDesc>& descs)
	{
		combo.addItem("--none--", 1);
		for (int i = 0; i < static_cast<int>(descs.size()); ++i)
		{
			const ParamDesc& desc = descs[i];
			if (desc.isModulated)
				combo.addItem(juce::String(desc.indexName), i + 2);
		}
	}
}

class GlobalSetting : public juce::Component
{
private:
	DirtynthParams& params;
	DirtynthSystem& instance;
	DirtynthParamSystem& paramTools;
	std::vector<DirtynthUIHelpers::ParamDesc>& paramDescs;
	LMKnobDirect masterVol;
	LMKnobDirect octave;
	LMCombox preset;
	std::function<void()> onPresetLoaded;

	void SendParams()
	{
		instance.SetParams(params);
	}
public:
	GlobalSetting(DirtynthParams& params, DirtynthSystem& instance,
		DirtynthParamSystem& paramTools,
		std::vector<DirtynthUIHelpers::ParamDesc>& paramDescs,
		std::function<void()> onPresetLoaded = {})
		: params(params), instance(instance),
		paramTools(paramTools), paramDescs(paramDescs),
		onPresetLoaded(std::move(onPresetLoaded))
	{
		DirtynthUIHelpers::BindKnob(masterVol, paramTools, params, paramDescs, "masterVol", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(octave, paramTools, params, paramDescs, "octave", [this]() { SendParams(); });
		preset.addItem("--Preset--", 1);
		preset.setSelectedID(1);
		preset.setChangedCallback([this](int selectedID)
			{
				juce::ignoreUnused(selectedID);
			});
		addAndMakeVisible(masterVol);
		addAndMakeVisible(octave);
		addAndMakeVisible(preset);
	}

	void Refresh()
	{
		DirtynthUIHelpers::BindKnob(masterVol, paramTools, params, paramDescs, "masterVol", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(octave, paramTools, params, paramDescs, "octave", [this]() { SendParams(); });
	}

	void resized() override
	{
		masterVol.setBounds(64 * 0, 0, 64, 64);
		octave.setBounds(64 * 1, 0, 64, 64);
		preset.setBounds(64 * 2 + 16, (64 - 32) / 2, 128, 24);
	}

	int GetTotalWidth() { return 64 * 2 + 16 + 128 + 32; }
};

class OscSetting : public juce::Component
{
private:
	DirtynthParams& params;
	DirtynthSystem& instance;
	DirtynthParamSystem& paramTools;
	std::vector<DirtynthUIHelpers::ParamDesc>& paramDescs;
	std::function<void()> onSemanticChanged;
	const int oscIndex;
	LMCombox wtpreset;
	LMKnobDirect wtpos;
	LMKnobDirect pitch;
	LMKnobDirect detune;
	LMCombox mutantAtype;
	LMKnobDirect pA1;
	LMKnobDirect pA2;
	LMKnobDirect pA3;
	LMCombox mutantBtype;
	LMKnobDirect pB1;
	LMKnobDirect pB2;
	LMKnobDirect pB3;

	DirtynthParams::OscParams& GetOscParams()
	{
		return (oscIndex == 1) ? params.osc1Params : params.osc2Params;
	}

	void SendParams()
	{
		instance.SetParams(params);
	}

public:
	OscSetting(DirtynthParams& params, DirtynthSystem& instance,
		DirtynthParamSystem& paramTools,
		std::vector<DirtynthUIHelpers::ParamDesc>& paramDescs,
		int osc,
		std::function<void()> onSemanticChanged = {})
		: params(params), instance(instance),
		paramTools(paramTools), paramDescs(paramDescs),
		onSemanticChanged(std::move(onSemanticChanged)),
		oscIndex(osc)
	{
		using namespace DirtynthUIHelpers;

		wtpreset.addItem("InitSaw", 1);
		AddNameItems(mutantAtype, RegMutant::MutantNames);
		AddNameItems(mutantBtype, RegMutant::MutantNames);

		auto& oscParams = GetOscParams();
		LinkIndexedCombo(wtpreset, oscParams.oscWtPreset, 1, [this]() { SendParams(); });
		LinkIndexedCombo(mutantAtype, oscParams.mutantA.mutantType, RegMutant::NumRegMutant, [this]() { SendParams(); if (this->onSemanticChanged) this->onSemanticChanged(); });
		LinkIndexedCombo(mutantBtype, oscParams.mutantB.mutantType, RegMutant::NumRegMutant, [this]() { SendParams(); if (this->onSemanticChanged) this->onSemanticChanged(); });

		addAndMakeVisible(wtpreset);
		addAndMakeVisible(wtpos);
		addAndMakeVisible(pitch);
		addAndMakeVisible(detune);
		addAndMakeVisible(mutantAtype);
		addAndMakeVisible(pA1);
		addAndMakeVisible(pA2);
		addAndMakeVisible(pA3);
		addAndMakeVisible(mutantBtype);
		addAndMakeVisible(pB1);
		addAndMakeVisible(pB2);
		addAndMakeVisible(pB3);

		Refresh();
	}

	void Refresh()
	{
		auto& oscParams = GetOscParams();
		wtpreset.setSelectedID(DirtynthUIHelpers::ClampInt(static_cast<int>(oscParams.oscWtPreset), 0, 0) + 1);
		mutantAtype.setSelectedID(DirtynthUIHelpers::ClampInt(static_cast<int>(oscParams.mutantA.mutantType), 0, RegMutant::NumRegMutant - 1) + 1);
		mutantBtype.setSelectedID(DirtynthUIHelpers::ClampInt(static_cast<int>(oscParams.mutantB.mutantType), 0, RegMutant::NumRegMutant - 1) + 1);
		const std::string prefix = "osc" + std::to_string(oscIndex);
		DirtynthUIHelpers::BindKnob(wtpos, paramTools, params, paramDescs, prefix + "WtPos", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(pitch, paramTools, params, paramDescs, prefix + "Pitch", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(detune, paramTools, params, paramDescs, prefix + "Detune", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(pA1, paramTools, params, paramDescs, prefix + "MutAP1", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(pA2, paramTools, params, paramDescs, prefix + "MutAP2", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(pA3, paramTools, params, paramDescs, prefix + "MutAP3", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(pB1, paramTools, params, paramDescs, prefix + "MutBP1", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(pB2, paramTools, params, paramDescs, prefix + "MutBP2", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(pB3, paramTools, params, paramDescs, prefix + "MutBP3", [this]() { SendParams(); });
	}

	int totalWidth = 0;
	void resized() override
	{
		totalWidth = 0;
		int comboxWidth = 96;
		wtpreset.setBounds(0, (64 - 32) / 2, comboxWidth, 24), totalWidth += comboxWidth;
		wtpos.setBounds(totalWidth + 16, 0, 64, 64), totalWidth += 16 + 64;
		pitch.setBounds(totalWidth, 0, 64, 64), totalWidth += 64;
		detune.setBounds(totalWidth, 0, 64, 64), totalWidth += 64;
		mutantAtype.setBounds(totalWidth + 16, (64 - 32) / 2, comboxWidth, 24), totalWidth += 16 + comboxWidth;
		pA1.setBounds(totalWidth + 16, 0, 64, 64), totalWidth += 16 + 64;
		pA2.setBounds(totalWidth, 0, 64, 64), totalWidth += 64;
		pA3.setBounds(totalWidth, 0, 64, 64), totalWidth += 64;
		mutantBtype.setBounds(totalWidth + 16, (64 - 32) / 2, comboxWidth, 24), totalWidth += 16 + comboxWidth;
		pB1.setBounds(totalWidth + 16, 0, 64, 64), totalWidth += 16 + 64;
		pB2.setBounds(totalWidth, 0, 64, 64), totalWidth += 64;
		pB3.setBounds(totalWidth, 0, 64, 64), totalWidth += 64;
	}

	int GetTotalWidth() { resized(); return totalWidth; }
};

class FilterSetting : public juce::Component
{
private:
	DirtynthParams& params;
	DirtynthSystem& instance;
	DirtynthParamSystem& paramTools;
	std::vector<DirtynthUIHelpers::ParamDesc>& paramDescs;
	std::function<void()> onSemanticChanged;
	const int filtIndex;
	LMCombox filterType;
	LMKnobDirect cutoff;
	LMKnobDirect keyTrack;
	LMKnobDirect reso;
	LMKnobDirect morph;

	DirtynthParams::FiltParams& GetFiltParams()
	{
		return (filtIndex == 1) ? params.filt1Params : params.filt2Params;
	}

	void SendParams()
	{
		instance.SetParams(params);
	}

public:
	FilterSetting(DirtynthParams& params, DirtynthSystem& instance,
		DirtynthParamSystem& paramTools,
		std::vector<DirtynthUIHelpers::ParamDesc>& paramDescs,
		int filt,
		std::function<void()> onSemanticChanged = {})
		: params(params), instance(instance),
		paramTools(paramTools), paramDescs(paramDescs),
		onSemanticChanged(std::move(onSemanticChanged)),
		filtIndex(filt)
	{
		using namespace DirtynthUIHelpers;

		AddNameItems(filterType, RegFilter::FilterNames);

		auto& filtParams = GetFiltParams();
		LinkIndexedCombo(filterType, filtParams.type, RegFilter::NumRegFilter, [this]() { SendParams(); if (this->onSemanticChanged) this->onSemanticChanged(); });

		addAndMakeVisible(filterType);
		addAndMakeVisible(cutoff);
		addAndMakeVisible(keyTrack);
		addAndMakeVisible(reso);
		addAndMakeVisible(morph);

		Refresh();
	}

	void Refresh()
	{
		auto& filtParams = GetFiltParams();
		filterType.setSelectedID(DirtynthUIHelpers::ClampInt(static_cast<int>(filtParams.type), 0, RegFilter::NumRegFilter - 1) + 1);
		const std::string prefix = "filt" + std::to_string(filtIndex);
		DirtynthUIHelpers::BindKnob(cutoff, paramTools, params, paramDescs, prefix + "Cutoff", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(keyTrack, paramTools, params, paramDescs, prefix + "KeyTrack", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(reso, paramTools, params, paramDescs, prefix + "Reso", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(morph, paramTools, params, paramDescs, prefix + "Morph", [this]() { SendParams(); });
	}

	void resized() override
	{
		int comboxWidth = 96;
		filterType.setBounds(0, (64 - 32) / 2, comboxWidth, 24);
		cutoff.setBounds(comboxWidth + 16, 0, 64, 64);
		keyTrack.setBounds(comboxWidth + 16 + 64, 0, 64, 64);
		reso.setBounds(comboxWidth + 16 + 128, 0, 64, 64);
		morph.setBounds(comboxWidth + 16 + 192, 0, 64, 64);
	}

	int GetTotalWidth() { return 96 + 16 + 64 * 4; }
};

class OscModulatorSetting : public juce::Component
{
private:
	DirtynthParams& params;
	DirtynthSystem& instance;
	DirtynthParamSystem& paramTools;
	std::vector<DirtynthUIHelpers::ParamDesc>& paramDescs;
	LMKnobDirect pmDepth;
	LMKnobDirect osc1gain;
	LMKnobDirect osc2gain;

	void SendParams()
	{
		instance.SetParams(params);
	}
public:
	OscModulatorSetting(DirtynthParams& params, DirtynthSystem& instance,
		DirtynthParamSystem& paramTools,
		std::vector<DirtynthUIHelpers::ParamDesc>& paramDescs)
		: params(params), instance(instance),
		paramTools(paramTools), paramDescs(paramDescs)
	{
		addAndMakeVisible(pmDepth);
		addAndMakeVisible(osc1gain);
		addAndMakeVisible(osc2gain);
		Refresh();
	}

	void Refresh()
	{
		DirtynthUIHelpers::BindKnob(pmDepth, paramTools, params, paramDescs, "pmDepth", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(osc1gain, paramTools, params, paramDescs, "osc1gain", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(osc2gain, paramTools, params, paramDescs, "osc2gain", [this]() { SendParams(); });
	}

	void resized() override
	{
		pmDepth.setBounds(64 * 0, 0, 64, 64);
		osc1gain.setBounds(64 * 1, 0, 64, 64);
		osc2gain.setBounds(64 * 2, 0, 64, 64);
	}

	int GetTotalWidth() { return 64 * 3; }
};

class FilterModulatorSetting : public juce::Component
{
private:
	DirtynthParams& params;
	DirtynthSystem& instance;
	DirtynthParamSystem& paramTools;
	std::vector<DirtynthUIHelpers::ParamDesc>& paramDescs;
	LMKnobDirect filt1gain;
	LMKnobDirect filt2gain;
	LMKnobDirect outputAmp;
	LMCombox sysTopo;

	void SendParams()
	{
		instance.SetParams(params);
	}
public:
	FilterModulatorSetting(DirtynthParams& params, DirtynthSystem& instance,
		DirtynthParamSystem& paramTools,
		std::vector<DirtynthUIHelpers::ParamDesc>& paramDescs)
		: params(params), instance(instance),
		paramTools(paramTools), paramDescs(paramDescs)
	{
		sysTopo.addItem("Parallel", 1);
		sysTopo.addItem("F1->F2", 2);
		sysTopo.addItem("Serial", 3);
		sysTopo.addItem("Dual", 4);
		DirtynthUIHelpers::LinkIndexedCombo(sysTopo, params.sysTopo, 4, [this]() { SendParams(); });
		addAndMakeVisible(filt1gain);
		addAndMakeVisible(filt2gain);
		addAndMakeVisible(outputAmp);
		addAndMakeVisible(sysTopo);
		Refresh();
	}

	void Refresh()
	{
		DirtynthUIHelpers::BindKnob(filt1gain, paramTools, params, paramDescs, "filt1gain", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(filt2gain, paramTools, params, paramDescs, "filt2gain", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(outputAmp, paramTools, params, paramDescs, "outputAmp", [this]() { SendParams(); });
		sysTopo.setSelectedID(DirtynthUIHelpers::ClampInt(static_cast<int>(params.sysTopo), 0, 3) + 1);
	}

	void resized() override
	{
		filt1gain.setBounds(64 * 0, 0, 64, 64);
		filt2gain.setBounds(64 * 1, 0, 64, 64);
		outputAmp.setBounds(64 * 2, 0, 64, 64);
		sysTopo.setBounds(64 * 3 + 16, (64 - 32) / 2, 96, 24);
	}

	int GetTotalWidth() { return 64 * 3 + 16 + 96; }
};

class EnvelopeSetting : public juce::Component
{
private:
	DirtynthParams& params;
	DirtynthSystem& instance;
	DirtynthParamSystem& paramTools;
	std::vector<DirtynthUIHelpers::ParamDesc>& paramDescs;
	std::function<void()> onSemanticChanged;
	int selectedEnvelope = 0;
	LMCombox envView;
	LMCombox envType;
	LMCombox envMode;
	LMCombox envTarget1;
	LMCombox envTarget2;
	LMKnobDirect amount1;
	LMKnobDirect amount2;
	LMKnobDirect p1;
	LMKnobDirect p2;
	LMKnobDirect p3;
	LMKnobDirect p4;
	LMKnobDirect p5;
	LMKnobDirect p6;

	DirtynthParams::EnveParams& GetSelectedParams()
	{
		return params.enveParams[selectedEnvelope];
	}

	void SendParams()
	{
		instance.SetParams(params);
	}

	void UpdateEnvelopeTargetSelection()
	{
		const int target1 = static_cast<int>(GetSelectedParams().targetID1);
		const int target2 = static_cast<int>(GetSelectedParams().targetID2);
		envTarget1.setSelectedID(target1 > 0 ? target1 + 1 : 1);
		envTarget2.setSelectedID(target2 > 0 ? target2 + 1 : 1);
	}

public:
	EnvelopeSetting(DirtynthParams& params, DirtynthSystem& instance,
		DirtynthParamSystem& paramTools,
		std::vector<DirtynthUIHelpers::ParamDesc>& paramDescs,
		std::function<void()> onSemanticChanged = {})
		: params(params), instance(instance),
		paramTools(paramTools), paramDescs(paramDescs),
		onSemanticChanged(std::move(onSemanticChanged))
	{
		using namespace DirtynthUIHelpers;

		for (int i = 0; i < NumEnvelopes; ++i)
			envView.addItem("Enve" + juce::String(i + 1), i + 1);

		AddNameItems(envType, RegEnvelope::EnvelopeNames);
		AddNameItems(envMode, EnvelopeModeNames);
		AddModTargetItems(envTarget1, paramDescs);
		AddModTargetItems(envTarget2, paramDescs);

		envView.setChangedCallback([this](int selectedID)
			{
				SelectEnve(DirtynthUIHelpers::ClampInt(selectedID - 1, 0, NumEnvelopes - 1));
			});
		envType.setChangedCallback([this](int selectedID)
			{
				GetSelectedParams().type = static_cast<float>(DirtynthUIHelpers::ClampInt(selectedID - 1, 0, RegEnvelope::NumRegEnvelope - 1));
				SendParams();
				if (this->onSemanticChanged)
					this->onSemanticChanged();
				SelectEnve(selectedEnvelope);
			});
		envMode.setChangedCallback([this](int selectedID)
			{
				const int modeCount = static_cast<int>(sizeof(DirtynthUIHelpers::EnvelopeModeNames) / sizeof(DirtynthUIHelpers::EnvelopeModeNames[0]));
				GetSelectedParams().mode = static_cast<float>(DirtynthUIHelpers::ClampInt(selectedID - 1, 0, modeCount - 1));
				SendParams();
			});
		envTarget1.setChangedCallback([this](int selectedID)
			{
				GetSelectedParams().targetID1 = static_cast<float>(selectedID > 1 ? selectedID - 1 : 0);
				SendParams();
			});
		envTarget2.setChangedCallback([this](int selectedID)
			{
				GetSelectedParams().targetID2 = static_cast<float>(selectedID > 1 ? selectedID - 1 : 0);
				SendParams();
			});

		addAndMakeVisible(envView);
		addAndMakeVisible(envType);
		addAndMakeVisible(envMode);
		addAndMakeVisible(envTarget1);
		addAndMakeVisible(envTarget2);
		addAndMakeVisible(amount1);
		addAndMakeVisible(amount2);
		addAndMakeVisible(p1);
		addAndMakeVisible(p2);
		addAndMakeVisible(p3);
		addAndMakeVisible(p4);
		addAndMakeVisible(p5);
		addAndMakeVisible(p6);

		SelectEnve(0);
	}

	void SelectEnve(int index)
	{
		selectedEnvelope = DirtynthUIHelpers::ClampInt(index, 0, NumEnvelopes - 1);
		DirtynthParams::EnveParams& enveParams = GetSelectedParams();

		envView.setSelectedID(selectedEnvelope + 1);
		envType.setSelectedID(DirtynthUIHelpers::ClampInt(static_cast<int>(enveParams.type), 0, RegEnvelope::NumRegEnvelope - 1) + 1);
		const int modeCount = static_cast<int>(sizeof(DirtynthUIHelpers::EnvelopeModeNames) / sizeof(DirtynthUIHelpers::EnvelopeModeNames[0]));
		envMode.setSelectedID(DirtynthUIHelpers::ClampInt(static_cast<int>(enveParams.mode), 0, modeCount - 1) + 1);
		UpdateEnvelopeTargetSelection();

		const std::string prefix = "enve" + std::to_string(selectedEnvelope + 1);
		DirtynthUIHelpers::BindKnob(amount1, paramTools, params, paramDescs, prefix + "Amount1", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(amount2, paramTools, params, paramDescs, prefix + "Amount2", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(p1, paramTools, params, paramDescs, prefix + "P1", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(p2, paramTools, params, paramDescs, prefix + "P2", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(p3, paramTools, params, paramDescs, prefix + "P3", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(p4, paramTools, params, paramDescs, prefix + "P4", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(p5, paramTools, params, paramDescs, prefix + "P5", [this]() { SendParams(); });
		DirtynthUIHelpers::BindKnob(p6, paramTools, params, paramDescs, prefix + "P6", [this]() { SendParams(); });
	}

	void Refresh()
	{
		SelectEnve(selectedEnvelope);
	}

	void resized() override
	{
		envView.setBounds(0, (64 - 32) / 2, 96, 24);
		envType.setBounds(96 + 16, (64 - 32) / 2, 96, 24);
		envMode.setBounds(192 + 32, (64 - 32) / 2, 96, 24);
		envTarget1.setBounds(288 + 48, 4, 128, 24);
		envTarget2.setBounds(288 + 48, 36, 128, 24);
		amount1.setBounds(416 + 64, 0, 64, 64);
		amount2.setBounds(480 + 64, 0, 64, 64);
		p1.setBounds(544 + 64, 0, 64, 64);
		p2.setBounds(608 + 64, 0, 64, 64);
		p3.setBounds(672 + 64, 0, 64, 64);
		p4.setBounds(736 + 64, 0, 64, 64);
		p5.setBounds(800 + 64, 0, 64, 64);
		p6.setBounds(864 + 64, 0, 64, 64);
	}

	int GetTotalWidth() { return 416 + 64 + 64 * 8; }
};

class DirtynthUI : public juce::Component, private juce::Timer
{
private:
	DirtynthSystem& instance;
	DirtynthParams params;
	DirtynthParamSystem paramTools;
	std::vector<DirtynthUIHelpers::ParamDesc> paramDescs;

	GlobalSetting globalSetting;
	OscModulatorSetting oscModulatorSetting;
	FilterModulatorSetting filterModulatorSetting;
	OscSetting osc1Setting;
	OscSetting osc2Setting;
	FilterSetting filter1Setting;
	FilterSetting filter2Setting;
	EnvelopeSetting envelopeSetting;

	void RefreshParamDescs()
	{
		paramDescs = paramTools.GetParamList(params);
	}

	void RefreshFromParams()
	{
		RefreshParamDescs();
		globalSetting.Refresh();
		oscModulatorSetting.Refresh();
		filterModulatorSetting.Refresh();
		osc1Setting.Refresh();
		osc2Setting.Refresh();
		filter1Setting.Refresh();
		filter2Setting.Refresh();
		envelopeSetting.Refresh();
	}

	void RefreshFromInstance()
	{
		DirtynthParams incomingParams = instance.GetParams();
		if (std::memcmp(&incomingParams, &params, sizeof(DirtynthParams)) == 0)
			return;

		params = incomingParams;
		RefreshFromParams();
	}
public:
	DirtynthUI(DirtynthSystem& instance)
		: instance(instance),
		params(instance.GetParams()),
		paramDescs(paramTools.GetParamList(params)),
		globalSetting(params, instance, paramTools, paramDescs, [this]() { RefreshFromParams(); }),
		oscModulatorSetting(params, instance, paramTools, paramDescs),
		filterModulatorSetting(params, instance, paramTools, paramDescs),
		osc1Setting(params, instance, paramTools, paramDescs, 1, [this]() { RefreshFromParams(); }),
		osc2Setting(params, instance, paramTools, paramDescs, 2, [this]() { RefreshFromParams(); }),
		filter1Setting(params, instance, paramTools, paramDescs, 1, [this]() { RefreshFromParams(); }),
		filter2Setting(params, instance, paramTools, paramDescs, 2, [this]() { RefreshFromParams(); }),
		envelopeSetting(params, instance, paramTools, paramDescs, [this]() { RefreshFromParams(); })
	{
		addAndMakeVisible(globalSetting);
		addAndMakeVisible(oscModulatorSetting);
		addAndMakeVisible(filterModulatorSetting);
		addAndMakeVisible(osc1Setting);
		addAndMakeVisible(osc2Setting);
		addAndMakeVisible(filter1Setting);
		addAndMakeVisible(filter2Setting);
		addAndMakeVisible(envelopeSetting);

		RefreshFromParams();
		startTimerHz(20);
	}

	void timerCallback() override
	{
		RefreshFromInstance();
	}

	void resized() override
	{
		globalSetting.setBounds(32, 32 + 64 * 0, globalSetting.GetTotalWidth(), 64);
		oscModulatorSetting.setBounds(globalSetting.GetTotalWidth() + 32, 32, oscModulatorSetting.GetTotalWidth(), 64);
		filterModulatorSetting.setBounds(globalSetting.GetTotalWidth() + oscModulatorSetting.GetTotalWidth() + 32 + 16, 32, filterModulatorSetting.GetTotalWidth(), 64);
		osc1Setting.setBounds(32, 32 + 64 * 1, osc1Setting.GetTotalWidth(), 64);
		osc2Setting.setBounds(32, 32 + 64 * 2, osc2Setting.GetTotalWidth(), 64);
		filter1Setting.setBounds(32, 32 + 64 * 3, filter1Setting.GetTotalWidth(), 64);
		filter2Setting.setBounds(32, 32 + 64 * 4, filter2Setting.GetTotalWidth(), 64);
		envelopeSetting.setBounds(32, 32 + 64 * 5, envelopeSetting.GetTotalWidth(), 64);
	}
};
