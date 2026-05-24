#pragma once

#include <JuceHeader.h>
#include "LM_slider.h"
#include "LMKnobDirect.h"
#include "../dsp/DirtySystem.h"

using namespace Dirtynth;

class GlobalSetting :public juce::Component
{
private:
	DirtynthParams& params;
	LMKnobDirect masterVol;
	LMKnobDirect octave;
	LMCombox preset;//暂时没有实现预设功能
public:
	GlobalSetting(DirtynthParams& params) :params(params)
	{
		masterVol.setText("Master");
		masterVol.ParamLink(0.0, 1.0, 0.5, params.masterVol, [&](float x) {params.masterVol = x; });
		octave.setText("Octave");
		octave.ParamLink(0.0, 1.0, 0.5, params.octave, [&](float x) {params.octave = x; });
		preset.addItem("--Init--", 1);
		addAndMakeVisible(masterVol);
		addAndMakeVisible(octave);
		addAndMakeVisible(preset);
	}
	void resized() override
	{
		masterVol.setBounds(64 * 0, 0, 64, 64);
		octave.setBounds(64 * 1, 0, 64, 64);
		preset.setBounds(64 * 2 + 16, (64 - 32) / 2, 128, 24);
	}
	int GetTotalWidth() { return 64 * 2 + 16 + 128 + 32; }
};
class OscSetting :public juce::Component
{
private:
	DirtynthParams& params;
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
public:
	OscSetting(DirtynthParams& params, int osc) :params(params)
	{
		wtpreset.addItem("InitSaw", 1);
		wtpos.setText("TablePos");
		pitch.setText("Pitch");
		detune.setText("Detune");
		mutantAtype.addItem("Kicknizer", 1);
		pA1.setText("PA1");
		pA2.setText("PA2");
		pA3.setText("PA3");
		mutantBtype.addItem("Kicknizer", 1);
		pB1.setText("PB1");
		pB2.setText("PB2");
		pB3.setText("PB3");
		DirtynthParams::OscParams& oscParams = (osc == 1) ? params.osc1Params : params.osc2Params;
		wtpos.ParamLink(0.0, 1.0, 0.5, oscParams.oscWtPos, [&](float x) {oscParams.oscWtPos = x; });
		pitch.ParamLink(-48.0, 48.0, 0.0, oscParams.oscPitch, [&](float x) {oscParams.oscPitch = x; });
		detune.ParamLink(-1200.0, 1200.0, 0.0, oscParams.oscDetune, [&](float x) {oscParams.oscDetune = x; });
		pA1.ParamLink(0.0, 1.0, 0.0, oscParams.mutantA.p1, [&](float x) {oscParams.mutantA.p1 = x; });
		pA2.ParamLink(0.0, 1.0, 0.0, oscParams.mutantA.p2, [&](float x) {oscParams.mutantA.p2 = x; });
		pA3.ParamLink(0.0, 1.0, 0.0, oscParams.mutantA.p3, [&](float x) {oscParams.mutantA.p3 = x; });
		pB1.ParamLink(0.0, 1.0, 0.0, oscParams.mutantB.p1, [&](float x) {oscParams.mutantB.p1 = x; });
		pB2.ParamLink(0.0, 1.0, 0.0, oscParams.mutantB.p2, [&](float x) {oscParams.mutantB.p2 = x; });
		pB3.ParamLink(0.0, 1.0, 0.0, oscParams.mutantB.p3, [&](float x) {oscParams.mutantB.p3 = x; });
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
class FilterSetting :public juce::Component
{
private:
	DirtynthParams& params;
	LMCombox filterType;
	LMKnobDirect cutoff;
	LMKnobDirect keyTrack;
	LMKnobDirect reso;
	LMKnobDirect morph;
public:
	FilterSetting(DirtynthParams& params, int filt) :params(params)
	{
		filterType.addItem("SVF12", 1);
		cutoff.setText("Cutoff");
		keyTrack.setText("KeyTrack");
		reso.setText("Reso");
		morph.setText("Morph");
		DirtynthParams::FiltParams& filtParams = (filt == 1) ? params.filt1Params : params.filt2Params;
		cutoff.ParamLink(0.0, 1.0, 1.0, filtParams.cutoff, [&](float x) {filtParams.cutoff = x; });
		keyTrack.ParamLink(0.0, 1.0, 0.0, filtParams.keyTrack, [&](float x) {filtParams.keyTrack = x; });
		reso.ParamLink(0.0, 1.0, 0.0, filtParams.reso, [&](float x) {filtParams.reso = x; });
		morph.ParamLink(0.0, 1.0, 0.0, filtParams.morph, [&](float x) {filtParams.morph = x; });
		addAndMakeVisible(filterType);
		addAndMakeVisible(cutoff);
		addAndMakeVisible(keyTrack);
		addAndMakeVisible(reso);
		addAndMakeVisible(morph);
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
class OscModulatorSetting :public juce::Component
{
private:
	DirtynthParams& params;
	LMKnobDirect pmDepth;
	LMKnobDirect oscMix;
	LMKnobDirect oscAmp;
public:
	OscModulatorSetting(DirtynthParams& params) :params(params)
	{
		pmDepth.setText("1PM2");
		oscMix.setText("OscMix");
		oscAmp.setText("OscAmp");
		pmDepth.ParamLink(0.0, 1.0, 0.0, params.pmDepth, [&](float x) {params.pmDepth = x; });
		oscMix.ParamLink(0.0, 1.0, 0.0, params.oscMix, [&](float x) {params.oscMix = x; });
		oscAmp.ParamLink(0.0, 1.0, 0.0, params.oscAmp, [&](float x) {params.oscAmp = x; });;
		addAndMakeVisible(pmDepth);
		addAndMakeVisible(oscMix);
		addAndMakeVisible(oscAmp);
	}
	void resized() override
	{
		pmDepth.setBounds(64 * 0, 0, 64, 64);
		oscMix.setBounds(64 * 1, 0, 64, 64);
		oscAmp.setBounds(64 * 2, 0, 64, 64);
	}
	int GetTotalWidth() { return 64 * 3; }
};
class FilterModulatorSetting :public juce::Component
{
private:
	DirtynthParams& params;
	LMKnobDirect filt2SwitchIn;
	LMKnobDirect filtMix;
public:
	FilterModulatorSetting(DirtynthParams& params) :params(params)
	{
		filt2SwitchIn.setText("F2Input");
		filtMix.setText("FiltMix");
		filt2SwitchIn.ParamLink(0.0, 1.0, 0.0, params.filt2SwitchIn, [&](float x) {params.filt2SwitchIn = x; });
		filtMix.ParamLink(0.0, 1.0, 0.0, params.filtMix, [&](float x) {params.filtMix = x; });
		addAndMakeVisible(filt2SwitchIn);
		addAndMakeVisible(filtMix);
	}
	void resized() override
	{
		filt2SwitchIn.setBounds(64 * 0, 0, 64, 64);
		filtMix.setBounds(64 * 1, 0, 64, 64);
	}
	int GetTotalWidth() { return 64 * 2; }
};
class EnvelopeSetting :public juce::Component
{
private:
	DirtynthParams& params;
	LMCombox envView;
	LMCombox envType;
	LMCombox envMode;
	LMCombox envTarget;
	LMKnobDirect p1;
	LMKnobDirect p2;
	LMKnobDirect p3;
	LMKnobDirect p4;
	LMKnobDirect p5;
	LMKnobDirect p6;
public:
	EnvelopeSetting(DirtynthParams& params) :params(params)
	{
		envView.addItem("Env1", 1);
		envView.addItem("Env2", 2);
		envView.addItem("Env3", 3);
		envView.addItem("Env4", 4);
		envView.addItem("Env5", 5);
		envView.addItem("Env6", 6);
		envType.addItem("ADSR", 1);
		envMode.addItem("PolyReset", 1);
		envTarget.addItem("--none--", 1);
		p1.setText("P1");
		p2.setText("P2");
		p3.setText("P3");
		p4.setText("P4");
		p5.setText("P5");
		p6.setText("P6");

		addAndMakeVisible(envView);
		addAndMakeVisible(envType);
		addAndMakeVisible(envMode);
		addAndMakeVisible(envTarget);
		addAndMakeVisible(p1);
		addAndMakeVisible(p2);
		addAndMakeVisible(p3);
		addAndMakeVisible(p4);
		addAndMakeVisible(p5);
		addAndMakeVisible(p6);
	}
	void SelectEnve(int index)
	{
		DirtynthParams::EnveParams& enveParams = params.enveParams[index];
		if (enveParams.enveType == 0)//ADSR
		{
			p1.setText("Attack");
			p2.setText("AttShape");
			p3.setText("Decay");
			p4.setText("DecShape");
			p5.setText("Sustain");
			p6.setText("Release");
			p1.ParamLink(0.0, 1.0, 0.0, enveParams.enveP1, [&](float x) {enveParams.enveP1 = x; });
			p2.ParamLink(0.0, 1.0, 0.0, enveParams.enveP2, [&](float x) {enveParams.enveP2 = x; });
			p3.ParamLink(0.0, 1.0, 0.0, enveParams.enveP3, [&](float x) {enveParams.enveP3 = x; });
			p4.ParamLink(0.0, 1.0, 0.0, enveParams.enveP4, [&](float x) {enveParams.enveP4 = x; });
			p5.ParamLink(0.0, 1.0, 0.0, enveParams.enveP5, [&](float x) {enveParams.enveP5 = x; });
			p6.ParamLink(0.0, 1.0, 0.0, enveParams.enveP6, [&](float x) {enveParams.enveP6 = x; });
		}
		else
		{
			p1.setText("P1");
			p2.setText("P2");
			p3.setText("P3");
			p4.setText("P4");
			p5.setText("P5");
			p6.setText("P6");
			p1.ParamLink(0.0, 1.0, 0.0, enveParams.enveP1, [&](float x) {enveParams.enveP1 = x; });
			p2.ParamLink(0.0, 1.0, 0.0, enveParams.enveP2, [&](float x) {enveParams.enveP2 = x; });
			p3.ParamLink(0.0, 1.0, 0.0, enveParams.enveP3, [&](float x) {enveParams.enveP3 = x; });
			p4.ParamLink(0.0, 1.0, 0.0, enveParams.enveP4, [&](float x) {enveParams.enveP4 = x; });
			p5.ParamLink(0.0, 1.0, 0.0, enveParams.enveP5, [&](float x) {enveParams.enveP5 = x; });
			p6.ParamLink(0.0, 1.0, 0.0, enveParams.enveP6, [&](float x) {enveParams.enveP6 = x; });
		}
	}
	void resized() override
	{
		envView.setBounds(0, (64 - 32) / 2, 96, 24);
		envType.setBounds(96 + 16, (64 - 32) / 2, 96, 24);
		envMode.setBounds(192 + 32, (64 - 32) / 2, 96, 24);
		envTarget.setBounds(288 + 48, (64 - 32) / 2, 96, 24);
		p1.setBounds(384 + 64, 0, 64, 64);
		p2.setBounds(448 + 64, 0, 64, 64);
		p3.setBounds(512 + 64, 0, 64, 64);
		p4.setBounds(576 + 64, 0, 64, 64);
		p5.setBounds(640 + 64, 0, 64, 64);
		p6.setBounds(704 + 64, 0, 64, 64);
	}
	int GetTotalWidth() { return 384 + 64 + 64 * 6; }
};
class DirtynthUI : public juce::Component
{
private:
	DirtynthParams params;
	GlobalSetting globalSetting{ params };
	OscModulatorSetting oscModulatorSetting{ params };
	FilterModulatorSetting filterModulatorSetting{ params };
	OscSetting osc1Setting{ params, 1 };
	OscSetting osc2Setting{ params, 2 };
	FilterSetting filter1Setting{ params, 1 };
	FilterSetting filter2Setting{ params, 2 };
	EnvelopeSetting envelopeSetting{ params };
public:
	DirtynthUI()
	{
		addAndMakeVisible(globalSetting);
		addAndMakeVisible(oscModulatorSetting);
		addAndMakeVisible(filterModulatorSetting);
		addAndMakeVisible(osc1Setting);
		addAndMakeVisible(osc2Setting);
		addAndMakeVisible(filter1Setting);
		addAndMakeVisible(filter2Setting);
		addAndMakeVisible(envelopeSetting);
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