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
		constexpr static int NumRegFilter = 6;
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
	struct RegEnvelope
	{
		constexpr static int NumRegEnvelope = 1;
		std::vector<std::shared_ptr<Envelope>> regEnvelope{
			std::make_shared<ADSR>() };
		std::shared_ptr<Envelope> operator[](std::size_t index)
		{
			return regEnvelope[index];
		}
		std::shared_ptr<const Envelope> operator[](std::size_t index) const
		{
			return regEnvelope[index];
		}
	};

	constexpr static int NumEnvelopes = 6;
	constexpr static int EnvelopeUpdateInterval = 4;
	constexpr static int MaxPolyphony = 8;
	struct DirtynthParams
	{
		struct OscParams
		{
			float oscWtPreset = 0;//int
			float oscWtPos = 0.0;
			struct MutantParams
			{
				float mutantType = 0;//int
				float p1 = 0, p2 = 0, p3 = 0;
			}mutantA, mutantB;
			float oscPitch = 0.0;
			float oscDetune = 0.0;
		}osc1Params, osc2Params;
		float pmDepth = 0.0;
		float oscMix = 0.0;
		float oscAmp = 0.0;
		float octave = 0.0;//int
		struct FiltParams
		{
			float type = 0;//int
			float cutoff = 0.0;
			float keyTrack = 0.0;
			float reso = 0.0;
			float morph = 0.0;
		}filt1Params, filt2Params;
		float filt2SwitchIn = 0.0;
		float filtMix = 0.0;
		struct EnveParams
		{
			float enveType = 0.0;//int
			float enveMode = 0.0;//int
			float enveTarget = 0.0;//int
			float enveAmount = 0.0;
			float enveP1 = 0.0;
			float enveP2 = 0.0;
			float enveP3 = 0.0;
			float enveP4 = 0.0;
			float enveP5 = 0.0;
			float enveP6 = 0.0;
		}enveParams[NumEnvelopes];
	};

	/*TOOLS FUNCTION*/
	inline void EnvelopeStep(Envelope* enves[NumEnvelopes])
	{
		for (int j = 0; j < NumEnvelopes; ++j)
			enves[j]->Step();
	}
	inline void ParamsApplyEnvelope(DirtynthParams& params, Envelope* enves[NumEnvelopes])
	{
		//根据enveTarget和enveAmount修改params
		float* paramsArray = reinterpret_cast<float*>(&params);
		for (int j = 0; j < NumEnvelopes; ++j)
		{
			int targetParamIdx = params.enveParams[j].enveTarget;
			paramsArray[targetParamIdx] += params.enveParams[j].enveAmount * enves[j]->GetValue();
			//之后应该补个各种参数的amount范围系数，有些参数大有些参数小
		}
	}
	/*--------------*/

	class DirtynthVoice
	{
	private:
		float sampleRate = 48000;
		WTOscillator osc1, osc2;
		RegFilter regFilt1, regFilt2;
		RegEnvelope regEnves[NumEnvelopes];
		int sampleCount = 0;
		/*VOICE RUNTIME STATE*/
		float voicefreq = 0.0;
		float voiceVel = 1.0;
		float voiceStateVolume = 0.0;//用于检测活动状态
		/*-------------------*/
	public:
		void SetSampleRate(float sr)
		{
			sampleRate = sr;
			//osc1.SetSampleRate(sr);
			//osc2.SetSampleRate(sr);
			for (int i = 0; i < RegFilter::NumRegFilter; ++i)
			{
				regFilt1[i]->SetSampleRate(sr);
				regFilt2[i]->SetSampleRate(sr);
			}
			for (int i = 0; i < RegEnvelope::NumRegEnvelope; ++i)
				for (int j = 0; j < NumEnvelopes; ++j)
					regEnves[j][i]->SetSampleRate(sr / EnvelopeUpdateInterval);
		}
		void ProcessBlockAccumulating(DirtynthParams& paramsInput, float* outl, float* outr, int numSamples)//输出直接叠加在原块上
		{
			DirtynthParams params = paramsInput;
			int selectedOsc1MutantAType = params.osc1Params.mutantA.mutantType;
			int selectedOsc1MutantBType = params.osc1Params.mutantB.mutantType;
			int selectedOsc2MutantAType = params.osc2Params.mutantA.mutantType;
			int selectedOsc2MutantBType = params.osc2Params.mutantB.mutantType;
			int selectedFilt1Type = params.filt1Params.type;
			int selectedFilt2Type = params.filt2Params.type;
			int selectedEnveTarget[NumEnvelopes];
			for (int i = 0; i < NumEnvelopes; ++i) selectedEnveTarget[i] = params.enveParams[i].enveTarget;
			Filter& filter1 = *std::dynamic_pointer_cast<Filter>(regFilt1[selectedFilt1Type]);
			Filter& filter2 = *std::dynamic_pointer_cast<Filter>(regFilt2[selectedFilt2Type]);
			Envelope* enves[NumEnvelopes];
			for (int i = 0; i < NumEnvelopes; ++i) enves[i] = regEnves[i][params.enveParams[i].enveType].get();

			float voiceDtBase = voicefreq / sampleRate;
			voiceStateVolume = 0.0;
			for (int i = 0; i < numSamples; ++i)
			{
				sampleCount++;
				if (sampleCount >= EnvelopeUpdateInterval)
				{
					sampleCount = 0;
					EnvelopeStep(enves);
					params = paramsInput;
					ParamsApplyEnvelope(params, enves);
				}
				/*OSC PROCESS*/
				float osc1dt = voiceDtBase * powf(2.0, (params.osc1Params.oscPitch + params.osc1Params.oscDetune) / 12.0);
				float osc2dt = voiceDtBase * powf(2.0, (params.osc2Params.oscPitch + params.osc2Params.oscDetune) / 12.0);
				float osc1out = osc1.ProcessSample(osc1dt);
				float osc2out = osc2.ProcessSample(osc2dt + params.pmDepth * voiceDtBase * osc1out);//乘voiceDtBase让pm输入幅度与频率去相关
				float oscout = (osc1out + (osc2out - osc1out) * params.oscMix) * params.oscAmp;
				/*FILTER PROCESS*/
				float filt1out = filter1.ProcessSample(oscout);
				float filt2out = filter2.ProcessSample(oscout + (filt1out - oscout) * params.filt2SwitchIn);
				float filtout = filt1out + (filt2out - filt1out) * params.filtMix;
				/*OUTPUT*/
				outl[i] += filtout * voiceVel;
				outr[i] += filtout * voiceVel;
				voiceStateVolume += fabsf(outl[i]);
			}
		}
		int IsVoiceActive() const
		{
			return voiceStateVolume > 0.00001f ? 1 : 0;//简单的门限检测是否活动
		}
		void SetVoiceEvent(float freq, float velocity)
		{
			voicefreq = freq;
			voiceVel = velocity;
			sampleCount = EnvelopeUpdateInterval;//立即更新包络
		}
		Envelope** GetSelectedEnvelops(DirtynthParams& params)//根据params返回当前选中的包络指针数组，方便外部函数调用
		{
			Envelope* enves[NumEnvelopes];
			for (int i = 0; i < NumEnvelopes; ++i) enves[i] = regEnves[i][params.enveParams[i].enveType].get();
			return enves;
		}
	};

	struct DirtynthMidiEvent
	{
		enum Type { NoteOn, NoteOff, ControlChange, PitchBend }type;
		int note = 0;//for note on/off
		int velocity = 0;//for note on
		int controlNumber = 0;//for control change
		int controlValue = 0;//for control change
		int pitchBendValue = 0;//for pitch bend
	};

	class Dirtynth
	{
	public:
	private:
		DirtynthParams params;
		WavetableGenerator wtgen;
		RegMutant regMutant;
		DirtynthVoice voices[MaxPolyphony];
		int isVoiceActive[MaxPolyphony] = { 0 };
		int nextVoiceIdx = 0;//最坏情况下使用循环分配voice。一般情况优先寻找不活动的voice
		int midiNumNoteOn = 0;//用于检测是否有按键按下，进而决定是否更新包络状态
	public:
		void SetSampleRate(float sr)
		{
			for (int i = 0; i < MaxPolyphony; ++i)
				voices[i].SetSampleRate(sr);
		}
		void SetParams(const DirtynthParams& newParams)
		{
			params = newParams;
		}
		void AutoFlagVoiceState()
		{
			for (int i = 0; i < MaxPolyphony; ++i)
				isVoiceActive[i] = voices[i].IsVoiceActive();
		}
		DirtynthVoice& FindNextVoice()
		{
			for (int i = 0; i < MaxPolyphony; ++i)
				if (!isVoiceActive[i])
					return voices[i];
			DirtynthVoice& voice = voices[nextVoiceIdx];
			nextVoiceIdx = (nextVoiceIdx + 1) % MaxPolyphony;
			return voice;
		}
		void UpdateAllVoiceEnvelope_NoteOn(DirtynthVoice& voice)//在这里面更新包络状态，包络有全局和复音模式
		{
			//先处理全局模式
			for (int i = 0; i < MaxPolyphony; ++i)
			{
				Envelope** enves = voices[i].GetSelectedEnvelops(params);
				for (int j = 0; j < NumEnvelopes; ++j)
				{
					if (static_cast<EnvelopeMode>((int)params.enveParams[j].enveMode) == EnvelopeMode::GlobalResetOnFirstNoteOn)//全局，按下第一个键重置
						if (midiNumNoteOn == 0)
							enves[j]->SetNoteState(1);
					if (static_cast<EnvelopeMode>((int)params.enveParams[j].enveMode) == EnvelopeMode::GlobalResetOnAnyNoteOn)//全局，按下任意键重置
						enves[j]->SetNoteState(1);
				}
			}
			//再处理复音模式
			Envelope** enves = voice.GetSelectedEnvelops(params);
			for (int j = 0; j < NumEnvelopes; ++j)
			{
				if (static_cast<EnvelopeMode>((int)params.enveParams[j].enveMode) == EnvelopeMode::PolyphonicResetOnNoteOn)//复音，按下时重置
					enves[j]->SetNoteState(1);
			}
		}
		void UpdateAllVoiceEnvelope_NoteOff(DirtynthVoice& voice)//在这里面更新包络状态，包络有全局和复音模式
		{
			//先处理全局模式
			for (int i = 0; i < MaxPolyphony; ++i)
			{
				Envelope** enves = voices[i].GetSelectedEnvelops(params);
				for (int j = 0; j < NumEnvelopes; ++j)
				{
					if (static_cast<EnvelopeMode>((int)params.enveParams[j].enveMode) == EnvelopeMode::GlobalResetOnAnyNoteOn)//全局，按下任意键重置
						if (midiNumNoteOn == 0)
							enves[j]->SetNoteState(0);
				}
			}
			//再处理复音模式
			Envelope** enves = voice.GetSelectedEnvelops(params);
			for (int j = 0; j < NumEnvelopes; ++j)
			{
				if (static_cast<EnvelopeMode>((int)params.enveParams[j].enveMode) == EnvelopeMode::PolyphonicResetOnNoteOn)//复音，按下时重置
					enves[j]->SetNoteState(0);
			}
		}
		void ProcessBlock(DirtynthMidiEvent* midiQueue, int numMidiEvents, float* outl, float* outr, int numSamples)
		{
			/*PROCESS MIDI*/
			for (int i = 0; i < numMidiEvents; ++i)
			{
				if (midiQueue[i].type == DirtynthMidiEvent::NoteOn)
				{
					float freq = 440.0f * powf(2.0f, (midiQueue[i].note - 69) / 12.0f);
					float velocity = midiQueue[i].velocity / 127.0f;
					DirtynthVoice& nextVoice = FindNextVoice();
					nextVoice.SetVoiceEvent(freq, velocity);
					UpdateAllVoiceEnvelope_NoteOn(nextVoice);
					midiNumNoteOn++;
				}
			}
			/*------------*/

			/*PROCESS VOICES*/
			/*--------------*/
		}
	};
}