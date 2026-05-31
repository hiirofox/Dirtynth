#pragma once

#include <cstring>

#include <JuceHeader.h>
#include "LM_slider.h"
#include "LMKnobDirect.h"
#include "../dsp/DirtySystem.h"
#include "../dsp/presets/presets.h"

using namespace Dirtynth;

namespace DirtynthUIHelpers
{
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

	inline int ParamsNameCount()
	{
		return static_cast<int>(sizeof(ParamsNamePack::paramsName) / sizeof(ParamsNamePack::paramsName[0]));
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

	inline DirtynthParams MakeDefaultUIParams()
	{
		DirtynthParams params;
		for (int i = 0; i < NumEnvelopes; ++i)
			params.enveParams[i].enveTarget = -1.0f;
		return params;
	}
}

class GlobalSetting : public juce::Component
{
private:
	DirtynthParams& params;
	DirtynthSystem& instance;
	LMKnobDirect masterVol;
	LMKnobDirect octave;
	LMCombox preset;
	std::function<void()> onPresetLoaded;

	void SendParams()
	{
		instance.SetParams(params);
	}
public:
	GlobalSetting(DirtynthParams& params, DirtynthSystem& instance, std::function<void()> onPresetLoaded = {})
		: params(params), instance(instance), onPresetLoaded(std::move(onPresetLoaded))
	{
		masterVol.setText("Master");
		masterVol.ParamLink(0.0f, 1.0f, 0.5f, params.masterVol, [this](float x) { this->params.masterVol = x; SendParams(); });
		octave.setText("Octave");
		octave.ParamLink(0.0f, 1.0f, 0.5f, params.octave, [this](float x) { this->params.octave = x; SendParams(); });
		preset.addItem("--Preset--", 1);
		for (int i = 0; i < DirtynthPresets::GetNumPresets(); ++i)
		{
			auto presetPack = DirtynthPresets::GetLocalPresets(i);
			preset.addItem(juce::String(std::get<0>(presetPack)), i + 2);
		}
		preset.setSelectedID(1);
		preset.setChangedCallback([this](int selectedID)
			{
				const int presetIndex = selectedID - 2;
				if (presetIndex < 0 || presetIndex >= DirtynthPresets::GetNumPresets())
					return;

				auto presetPack = DirtynthPresets::GetLocalPresets(presetIndex);
				this->params = std::get<1>(presetPack);
				SendParams();
				if (this->onPresetLoaded)
					this->onPresetLoaded();
			});
		addAndMakeVisible(masterVol);
		addAndMakeVisible(octave);
		addAndMakeVisible(preset);
	}

	void Refresh()
	{
		masterVol.setValue(params.masterVol);
		octave.setValue(params.octave);
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
	OscSetting(DirtynthParams& params, DirtynthSystem& instance, int osc) : params(params), instance(instance), oscIndex(osc)
	{
		using namespace DirtynthUIHelpers;

		wtpreset.addItem("InitSaw", 1);
		AddNameItems(mutantAtype, RegMutant::MutantNames);
		AddNameItems(mutantBtype, RegMutant::MutantNames);

		wtpos.setText("TablePos");
		pitch.setText("Pitch");
		detune.setText("Detune");
		pA1.setText("PA1");
		pA2.setText("PA2");
		pA3.setText("PA3");
		pB1.setText("PB1");
		pB2.setText("PB2");
		pB3.setText("PB3");

		auto& oscParams = GetOscParams();
		LinkIndexedCombo(wtpreset, oscParams.oscWtPreset, 1, [this]() { SendParams(); });
		LinkIndexedCombo(mutantAtype, oscParams.mutantA.mutantType, RegMutant::NumRegMutant, [this]() { SendParams(); });
		LinkIndexedCombo(mutantBtype, oscParams.mutantB.mutantType, RegMutant::NumRegMutant, [this]() { SendParams(); });

		wtpos.ParamLink(0.0f, 1.0f, 0.5f, oscParams.oscWtPos, [this](float x) { GetOscParams().oscWtPos = x; SendParams(); });
		pitch.ParamLink(-48.0f, 48.0f, 0.0f, oscParams.oscPitch, [this](float x) { GetOscParams().oscPitch = x; SendParams(); });
		detune.ParamLink(-1.0, 1.0f, 0.0f, oscParams.oscDetune, [this](float x) { GetOscParams().oscDetune = x; SendParams(); });
		pA1.ParamLink(0.0f, 1.0f, 0.0f, oscParams.mutantA.p1, [this](float x) { GetOscParams().mutantA.p1 = x; SendParams(); });
		pA2.ParamLink(0.0f, 1.0f, 0.0f, oscParams.mutantA.p2, [this](float x) { GetOscParams().mutantA.p2 = x; SendParams(); });
		pA3.ParamLink(0.0f, 1.0f, 0.0f, oscParams.mutantA.p3, [this](float x) { GetOscParams().mutantA.p3 = x; SendParams(); });
		pB1.ParamLink(0.0f, 1.0f, 0.0f, oscParams.mutantB.p1, [this](float x) { GetOscParams().mutantB.p1 = x; SendParams(); });
		pB2.ParamLink(0.0f, 1.0f, 0.0f, oscParams.mutantB.p2, [this](float x) { GetOscParams().mutantB.p2 = x; SendParams(); });
		pB3.ParamLink(0.0f, 1.0f, 0.0f, oscParams.mutantB.p3, [this](float x) { GetOscParams().mutantB.p3 = x; SendParams(); });

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
	}

	void Refresh()
	{
		auto& oscParams = GetOscParams();
		wtpreset.setSelectedID(DirtynthUIHelpers::ClampInt(static_cast<int>(oscParams.oscWtPreset), 0, 0) + 1);
		mutantAtype.setSelectedID(DirtynthUIHelpers::ClampInt(static_cast<int>(oscParams.mutantA.mutantType), 0, RegMutant::NumRegMutant - 1) + 1);
		mutantBtype.setSelectedID(DirtynthUIHelpers::ClampInt(static_cast<int>(oscParams.mutantB.mutantType), 0, RegMutant::NumRegMutant - 1) + 1);
		wtpos.setValue(oscParams.oscWtPos);
		pitch.setValue(oscParams.oscPitch);
		detune.setValue(oscParams.oscDetune);
		pA1.setValue(oscParams.mutantA.p1);
		pA2.setValue(oscParams.mutantA.p2);
		pA3.setValue(oscParams.mutantA.p3);
		pB1.setValue(oscParams.mutantB.p1);
		pB2.setValue(oscParams.mutantB.p2);
		pB3.setValue(oscParams.mutantB.p3);
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
	FilterSetting(DirtynthParams& params, DirtynthSystem& instance, int filt) : params(params), instance(instance), filtIndex(filt)
	{
		using namespace DirtynthUIHelpers;

		AddNameItems(filterType, RegFilter::FilterNames);
		cutoff.setText("Cutoff");
		keyTrack.setText("KeyTrack");
		reso.setText("Reso");
		morph.setText("Morph");

		auto& filtParams = GetFiltParams();
		LinkIndexedCombo(filterType, filtParams.type, RegFilter::NumRegFilter, [this]() { SendParams(); });
		cutoff.ParamLink(0.0f, 1.0f, 1.0f, filtParams.cutoff, [this](float x) { GetFiltParams().cutoff = x; SendParams(); });
		keyTrack.ParamLink(0.0f, 1.0f, 0.0f, filtParams.keyTrack, [this](float x) { GetFiltParams().keyTrack = x; SendParams(); });
		reso.ParamLink(0.0f, 1.0f, 0.0f, filtParams.reso, [this](float x) { GetFiltParams().reso = x; SendParams(); });
		morph.ParamLink(0.0f, 1.0f, 0.0f, filtParams.morph, [this](float x) { GetFiltParams().morph = x; SendParams(); });

		addAndMakeVisible(filterType);
		addAndMakeVisible(cutoff);
		addAndMakeVisible(keyTrack);
		addAndMakeVisible(reso);
		addAndMakeVisible(morph);
	}

	void Refresh()
	{
		auto& filtParams = GetFiltParams();
		filterType.setSelectedID(DirtynthUIHelpers::ClampInt(static_cast<int>(filtParams.type), 0, RegFilter::NumRegFilter - 1) + 1);
		cutoff.setValue(filtParams.cutoff);
		keyTrack.setValue(filtParams.keyTrack);
		reso.setValue(filtParams.reso);
		morph.setValue(filtParams.morph);
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
	LMKnobDirect pmDepth;
	LMKnobDirect oscMix;
	LMKnobDirect oscAmp;

	void SendParams()
	{
		instance.SetParams(params);
	}
public:
	OscModulatorSetting(DirtynthParams& params, DirtynthSystem& instance) : params(params), instance(instance)
	{
		pmDepth.setText("1PM2");
		oscMix.setText("OscMix");
		oscAmp.setText("OscAmp");
		pmDepth.ParamLink(0.0f, 1.0f, 0.0f, params.pmDepth, [this](float x) { this->params.pmDepth = x; SendParams(); });
		oscMix.ParamLink(0.0f, 1.0f, 0.0f, params.oscMix, [this](float x) { this->params.oscMix = x; SendParams(); });
		oscAmp.ParamLink(0.0f, 1.0f, 0.0f, params.oscAmp, [this](float x) { this->params.oscAmp = x; SendParams(); });
		addAndMakeVisible(pmDepth);
		addAndMakeVisible(oscMix);
		addAndMakeVisible(oscAmp);
	}

	void Refresh()
	{
		pmDepth.setValue(params.pmDepth);
		oscMix.setValue(params.oscMix);
		oscAmp.setValue(params.oscAmp);
	}

	void resized() override
	{
		pmDepth.setBounds(64 * 0, 0, 64, 64);
		oscMix.setBounds(64 * 1, 0, 64, 64);
		oscAmp.setBounds(64 * 2, 0, 64, 64);
	}

	int GetTotalWidth() { return 64 * 3; }
};

class FilterModulatorSetting : public juce::Component
{
private:
	DirtynthParams& params;
	DirtynthSystem& instance;
	LMKnobDirect filt2SwitchIn;
	LMKnobDirect filtMix;

	void SendParams()
	{
		instance.SetParams(params);
	}
public:
	FilterModulatorSetting(DirtynthParams& params, DirtynthSystem& instance) : params(params), instance(instance)
	{
		filt2SwitchIn.setText("F2Input");
		filtMix.setText("FiltMix");
		filt2SwitchIn.ParamLink(0.0f, 1.0f, 0.0f, params.filt2SwitchIn, [this](float x) { this->params.filt2SwitchIn = x; SendParams(); });
		filtMix.ParamLink(0.0f, 1.0f, 0.0f, params.filtMix, [this](float x) { this->params.filtMix = x; SendParams(); });
		addAndMakeVisible(filt2SwitchIn);
		addAndMakeVisible(filtMix);
	}

	void Refresh()
	{
		filt2SwitchIn.setValue(params.filt2SwitchIn);
		filtMix.setValue(params.filtMix);
	}

	void resized() override
	{
		filt2SwitchIn.setBounds(64 * 0, 0, 64, 64);
		filtMix.setBounds(64 * 1, 0, 64, 64);
	}

	int GetTotalWidth() { return 64 * 2; }
};

class EnvelopeSetting : public juce::Component
{
private:
	DirtynthParams& params;
	DirtynthSystem& instance;
	int selectedEnvelope = 0;
	LMCombox envView;
	LMCombox envType;
	LMCombox envMode;
	LMCombox envTarget;
	LMKnobDirect amount;
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
		const int target = static_cast<int>(GetSelectedParams().enveTarget);
		if (target < 0 || target >= DirtynthUIHelpers::ParamsNameCount())
			envTarget.setSelectedID(1);
		else
			envTarget.setSelectedID(target + 2);
	}

public:
	EnvelopeSetting(DirtynthParams& params, DirtynthSystem& instance) : params(params), instance(instance)
	{
		using namespace DirtynthUIHelpers;

		for (int i = 0; i < NumEnvelopes; ++i)
			envView.addItem("Enve" + juce::String(i + 1), i + 1);

		AddNameItems(envType, RegEnvelope::EnvelopeNames);
		AddNameItems(envMode, EnvelopeModeNames);
		envTarget.addItem("--none--", 1);
		for (int i = 0; i < ParamsNameCount(); ++i)
			envTarget.addItem(ParamsNamePack::paramsName[i], i + 2);

		envView.setChangedCallback([this](int selectedID)
			{
				SelectEnve(DirtynthUIHelpers::ClampInt(selectedID - 1, 0, NumEnvelopes - 1));
			});
		envType.setChangedCallback([this](int selectedID)
			{
				GetSelectedParams().enveType = static_cast<float>(DirtynthUIHelpers::ClampInt(selectedID - 1, 0, RegEnvelope::NumRegEnvelope - 1));
				SendParams();
				SelectEnve(selectedEnvelope);
			});
		envMode.setChangedCallback([this](int selectedID)
			{
				const int modeCount = static_cast<int>(sizeof(DirtynthUIHelpers::EnvelopeModeNames) / sizeof(DirtynthUIHelpers::EnvelopeModeNames[0]));
				GetSelectedParams().enveMode = static_cast<float>(DirtynthUIHelpers::ClampInt(selectedID - 1, 0, modeCount - 1));
				SendParams();
			});
		envTarget.setChangedCallback([this](int selectedID)
			{
				if (selectedID <= 1)
					GetSelectedParams().enveTarget = -1.0f;
				else
					GetSelectedParams().enveTarget = static_cast<float>(DirtynthUIHelpers::ClampInt(selectedID - 2, 0, DirtynthUIHelpers::ParamsNameCount() - 1));
				SendParams();
			});

		addAndMakeVisible(envView);
		addAndMakeVisible(envType);
		addAndMakeVisible(envMode);
		addAndMakeVisible(envTarget);
		addAndMakeVisible(amount);
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
		envType.setSelectedID(DirtynthUIHelpers::ClampInt(static_cast<int>(enveParams.enveType), 0, RegEnvelope::NumRegEnvelope - 1) + 1);
		const int modeCount = static_cast<int>(sizeof(DirtynthUIHelpers::EnvelopeModeNames) / sizeof(DirtynthUIHelpers::EnvelopeModeNames[0]));
		envMode.setSelectedID(DirtynthUIHelpers::ClampInt(static_cast<int>(enveParams.enveMode), 0, modeCount - 1) + 1);
		UpdateEnvelopeTargetSelection();

		amount.setText("Amount");
		if (enveParams.enveType == 0)
		{
			p1.setText("Attack");
			p2.setText("AttShape");
			p3.setText("Decay");
			p4.setText("DecShape");
			p5.setText("Sustain");
			p6.setText("Release");
		}
		else
		{
			p1.setText("P1");
			p2.setText("P2");
			p3.setText("P3");
			p4.setText("P4");
			p5.setText("P5");
			p6.setText("P6");
		}

		amount.ParamLink(-1.0f, 1.0f, 0.0f, enveParams.enveAmount, [this, index = selectedEnvelope](float x) { this->params.enveParams[index].enveAmount = x; SendParams(); });
		p1.ParamLink(0.0f, 1.0f, 0.0f, enveParams.enveP1, [this, index = selectedEnvelope](float x) { this->params.enveParams[index].enveP1 = x; SendParams(); });
		p2.ParamLink(0.0f, 1.0f, 0.0f, enveParams.enveP2, [this, index = selectedEnvelope](float x) { this->params.enveParams[index].enveP2 = x; SendParams(); });
		p3.ParamLink(0.0f, 1.0f, 0.0f, enveParams.enveP3, [this, index = selectedEnvelope](float x) { this->params.enveParams[index].enveP3 = x; SendParams(); });
		p4.ParamLink(0.0f, 1.0f, 0.0f, enveParams.enveP4, [this, index = selectedEnvelope](float x) { this->params.enveParams[index].enveP4 = x; SendParams(); });
		p5.ParamLink(0.0f, 1.0f, 0.0f, enveParams.enveP5, [this, index = selectedEnvelope](float x) { this->params.enveParams[index].enveP5 = x; SendParams(); });
		p6.ParamLink(0.0f, 1.0f, 0.0f, enveParams.enveP6, [this, index = selectedEnvelope](float x) { this->params.enveParams[index].enveP6 = x; SendParams(); });
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
		envTarget.setBounds(288 + 48, (64 - 32) / 2, 128, 24);
		amount.setBounds(416 + 64, 0, 64, 64);
		p1.setBounds(480 + 64, 0, 64, 64);
		p2.setBounds(544 + 64, 0, 64, 64);
		p3.setBounds(608 + 64, 0, 64, 64);
		p4.setBounds(672 + 64, 0, 64, 64);
		p5.setBounds(736 + 64, 0, 64, 64);
		p6.setBounds(800 + 64, 0, 64, 64);
	}

	int GetTotalWidth() { return 416 + 64 + 64 * 7; }
};

class DirtynthUI : public juce::Component, private juce::Timer
{
private:
	DirtynthSystem& instance;
	DirtynthParams params;
	GlobalSetting globalSetting;
	OscModulatorSetting oscModulatorSetting;
	FilterModulatorSetting filterModulatorSetting;
	OscSetting osc1Setting;
	OscSetting osc2Setting;
	FilterSetting filter1Setting;
	FilterSetting filter2Setting;
	EnvelopeSetting envelopeSetting;

	void RefreshFromParams()
	{
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
		globalSetting(params, instance, [this]() { RefreshFromParams(); }),
		oscModulatorSetting(params, instance),
		filterModulatorSetting(params, instance),
		osc1Setting(params, instance, 1),
		osc2Setting(params, instance, 2),
		filter1Setting(params, instance, 1),
		filter2Setting(params, instance, 2),
		envelopeSetting(params, instance)
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
