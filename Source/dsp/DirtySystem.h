#pragma once

#include <array>

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
	constexpr static int NumMutantThreads = 2;//根据平台cpu填

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
	constexpr static float CutoffMin = 20.0;
	constexpr static float CutoffMax = 22000.0;
	constexpr static float ResoMin = 0.707;
	constexpr static float ResoMax = 40.0;
	inline float Clamp01(float x)
	{
		if (x < 0.0f) return 0.0f;
		if (x > 1.0f) return 1.0f;
		return x;
	}
	inline float ParamToCutoff(float param)
	{
		param = Clamp01(param);
		return CutoffMin * powf(CutoffMax / CutoffMin, param);
	}
	inline float ParamToReso(float param)
	{
		param = Clamp01(param);
		return ResoMin * powf(ResoMax / ResoMin, param);
	}
	inline float CutoffToParam(float cutoff)
	{
		if (cutoff < CutoffMin) cutoff = CutoffMin;
		if (cutoff > CutoffMax) cutoff = CutoffMax;
		return logf(cutoff / CutoffMin) / logf(CutoffMax / CutoffMin);
	}
	inline float ResoToParam(float reso)
	{
		if (reso < ResoMin) reso = ResoMin;
		if (reso > ResoMax) reso = ResoMax;
		return logf(reso / ResoMin) / logf(ResoMax / ResoMin);
	}
	/*--------------*/

	class MutantThreadPool//异步计算mutant线程池
	{
	private:
		std::thread threads[NumMutantThreads];
		WavetableGenerator wtgen[NumMutantThreads];
		RegMutant regMutant[NumMutantThreads];
		struct MutantTask
		{
			int wtgenPreset;
			float wtgenPos; //WavetableGenerator 0-1
			int typeA;
			float pA1, pA2, pA3;//mutantA
			int typeB;
			float pB1, pB2, pB3;//mutantB

			float* intMagtable;
			int tableWidth;
		};
		constexpr static int MaxQueueLen = MaxPolyphony * 2 + 10;
		MutantTask taskQueue[MaxQueueLen];//每个osc有2个mutant，外加一些冗余
		std::atomic_bool taskFlags[MaxQueueLen] = { false };
		int submitIdx = 0, processIdx = 0;
		std::atomic<bool> isRunning = true;
	public:
		MutantThreadPool()
		{
			for (int i = 0; i < NumMutantThreads; ++i)
			{
				threads[i] = std::thread(&MutantThreadPool::MutantThreadFunc, this, i);
			}
		}
		~MutantThreadPool()
		{
			isRunning = false;
			for (int i = 0; i < NumMutantThreads; ++i)
			{
				if (threads[i].joinable())threads[i].join();
			}
		}
		int SubmitMutantTask(
			int wtgenPreset, float wtgenPos,//WavetableGenerator参数
			int typeA, float pA1, float pA2, float pA3,//mutantA参数
			int typeB, float pB1, float pB2, float pB3,//mutantB参数
			float* intMagtable/*tableWidth*2*/, int tableWidth)//输出表参数
		{
			MutantTask pack = { wtgenPreset, wtgenPos, typeA, pA1, pA2, pA3, typeB, pB1, pB2, pB3, intMagtable, tableWidth };
			MutantTask* nextTask = nullptr;
			int taskID = -1;
			for (int i = 0; i < MaxQueueLen; ++i)
			{
				int idx = (submitIdx + i) % MaxQueueLen;
				if (taskFlags[idx].load() == 0)
				{
					taskID = idx;
					nextTask = &taskQueue[idx];
					break;
				}
			}
			if (nextTask)
			{
				*nextTask = pack;
				taskFlags[taskID].store(1);
				return taskID;
			}
			else
			{
				return -1;
			}
		}
		int GetTaskState(int taskID)
		{
			if (taskID < 0 || taskID >= MaxQueueLen)return -1;
			return taskFlags[taskID].load();
		}

		void MutantThreadFunc(int threadId)
		{
			float* localTable = new float[WTOscillator::TableWidth * 2];
			WavetableGenerator& wtGenerator = wtgen[threadId];
			while (isRunning)
			{
				for (int i = threadId; i < MaxQueueLen; i += NumMutantThreads)
				{
					MutantTask& task = taskQueue[i];
					if (taskFlags[i].load() == 1)
					{
						TableMutant* mutantA = regMutant[threadId][task.typeA].get();
						TableMutant* mutantB = regMutant[threadId][task.typeB].get();
						wtGenerator.Generate(task.wtgenPreset);
						float* wtgenTable = wtGenerator.GetTable(task.wtgenPos * 63.0);//0-1!
						for (int j = 0; j < task.tableWidth; ++j)localTable[j] = wtgenTable[j];
						mutantA->SetMutantParams(task.pA1, task.pA2, task.pA3);
						mutantB->SetMutantParams(task.pB1, task.pB2, task.pB3);
						mutantA->Apply(localTable, task.tableWidth);
						mutantB->Apply(localTable, task.tableWidth);
						WTOscillator::CalcIntMagtable(task.intMagtable, localTable, task.tableWidth);
						taskFlags[i].store(0);
					}
				}
				std::this_thread::sleep_for(std::chrono::nanoseconds(2000));
			}
		}
	};

	class DirtynthVoice
	{
	private:
		float sampleRate = 48000.0;
		WTOscillator osc1, osc2;
		RegFilter regFilt1, regFilt2;
		RegEnvelope regEnves[NumEnvelopes];
		int sampleCount = 0;
		/*VOICE RUNTIME STATE*/
		float voicefreq = 0.0;
		float voiceVel = 1.0;
		float voiceStateVolume = 0.0;//用于检测活动状态
		int isNoteOn = 0;

		int osc1MutantTaskID = -1;
		int osc2MutantTaskID = -1;
		/*-------------------*/
		MutantThreadPool* mutantThreadPool;
	public:
		DirtynthVoice()
		{
			SetSampleRate(48000.0);
		}
		DirtynthVoice(MutantThreadPool* pool) :mutantThreadPool(pool)
		{
			SetSampleRate(48000.0);
		}
		void SetMutantThreadPool(MutantThreadPool* pool)
		{
			mutantThreadPool = pool;
		}

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
					regEnves[j][i]->SetSampleRate(sr / EnvelopeUpdateInterval);//根据间隔设置采样率
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

			/*将被调制的参数指针，以及原始的参数指针打包*/
			float* paramsArray = reinterpret_cast<float*>(&params);
			float* origParamsArray = reinterpret_cast<float*>(&paramsInput);
			float* paramTargetPtr[NumEnvelopes] = { nullptr };
			float* paramOriginalValue[NumEnvelopes] = { nullptr };
			for (int j = 0; j < NumEnvelopes; ++j)
			{
				int targetParamIdx = params.enveParams[j].enveTarget;
				if (targetParamIdx >= 0 && targetParamIdx < sizeof(DirtynthParams) / sizeof(float))
				{
					paramTargetPtr[j] = &paramsArray[targetParamIdx];
					paramOriginalValue[j] = &origParamsArray[targetParamIdx];
				}
				else
				{
					paramTargetPtr[j] = nullptr;
					paramOriginalValue[j] = nullptr;
				}
			}

			for (int i = 0; i < numSamples; ++i)
			{
				sampleCount++;
				if (sampleCount >= EnvelopeUpdateInterval)
				{
					sampleCount = 0;
					//设置包络参数
					for (int j = 0; j < NumEnvelopes; ++j)
					{
						enves[j]->SetParams(params.enveParams[j].enveP1, params.enveParams[j].enveP2, params.enveParams[j].enveP3,
							params.enveParams[j].enveP4, params.enveParams[j].enveP5, params.enveParams[j].enveP6);
					}
					//更新包络
					for (int j = 0; j < NumEnvelopes; ++j) enves[j]->Step();
					//根据enveTarget和enveAmount修改params
					for (int j = 0; j < NumEnvelopes; ++j)
					{
						if (paramTargetPtr[j] != nullptr)
							*paramTargetPtr[j] = *paramOriginalValue[j] + enves[j]->GetValue() * params.enveParams[j].enveAmount;
					}
					//检查oscillator的intMagtable是否需要更新，并更新
					if (osc1MutantTaskID == -1 && osc1.IsSwapTablePrepared())
					{
						if (IsVoiceActive())
							osc1MutantTaskID = mutantThreadPool->SubmitMutantTask(
								params.osc1Params.oscWtPreset, params.osc1Params.oscWtPos,
								selectedOsc1MutantAType, params.osc1Params.mutantA.p1, params.osc1Params.mutantA.p2, params.osc1Params.mutantA.p3,
								selectedOsc1MutantBType, params.osc1Params.mutantB.p1, params.osc1Params.mutantB.p2, params.osc1Params.mutantB.p3,
								osc1.GetNextIntMagtable(), WTOscillator::TableWidth);
					}
					if (osc2MutantTaskID == -1 && osc2.IsSwapTablePrepared())
					{
						if (IsVoiceActive())
							osc2MutantTaskID = mutantThreadPool->SubmitMutantTask(
								params.osc2Params.oscWtPreset, params.osc2Params.oscWtPos,
								selectedOsc2MutantAType, params.osc2Params.mutantA.p1, params.osc2Params.mutantA.p2, params.osc2Params.mutantA.p3,
								selectedOsc2MutantBType, params.osc2Params.mutantB.p1, params.osc2Params.mutantB.p2, params.osc2Params.mutantB.p3,
								osc2.GetNextIntMagtable(), WTOscillator::TableWidth);
					}
					if (osc1MutantTaskID != -1 && mutantThreadPool->GetTaskState(osc1MutantTaskID) == 0)
					{
						osc1.SetFillCompleteFlag();
						osc1MutantTaskID = -1;
					}
					if (osc2MutantTaskID != -1 && mutantThreadPool->GetTaskState(osc2MutantTaskID) == 0)
					{
						osc2.SetFillCompleteFlag();
						osc2MutantTaskID = -1;
					}
					//根据params更新滤波器参数
					float cutoffTrackValue = powf(voicefreq / 440.0, params.filt1Params.keyTrack);
					filter1.SetFilterParams(
						ParamToCutoff(params.filt1Params.cutoff) * cutoffTrackValue,
						ParamToReso(params.filt1Params.reso), params.filt1Params.morph);
					filter2.SetFilterParams(
						ParamToCutoff(params.filt2Params.cutoff) * cutoffTrackValue,
						ParamToReso(params.filt2Params.reso), params.filt2Params.morph);
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
				float output = filtout * voiceVel;
				outl[i] += output;
				outr[i] += output;
				voiceStateVolume += fabsf(output);
			}
		}
		int IsVoiceActive() const
		{
			if (voiceStateVolume < 0.00001f && !isNoteOn) return 0;
			return 1;
		}
		void SetVoiceState(bool off0on1, float freq, float velocity)
		{
			isNoteOn = off0on1;
			if (isNoteOn)
			{
				voicefreq = freq;
				voiceVel = velocity;
				sampleCount = EnvelopeUpdateInterval;//立即更新包络
			}
		}
		std::array<Envelope*, NumEnvelopes> GetSelectedEnvelops(const DirtynthParams& params)//根据params返回当前选中的包络指针数组，方便外部函数调用
		{
			std::array<Envelope*, NumEnvelopes> enves{};
			for (int i = 0; i < NumEnvelopes; ++i)
			{
				int envelopeType = static_cast<int>(params.enveParams[i].enveType);
				if (envelopeType < 0)envelopeType = 0;
				if (envelopeType >= RegEnvelope::NumRegEnvelope)envelopeType = RegEnvelope::NumRegEnvelope - 1;
				enves[i] = regEnves[i][envelopeType].get();
			}
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
		DirtynthVoice voices[MaxPolyphony];
		MutantThreadPool mutantThreadPool;
		int voiceBelongNote[MaxPolyphony] = { -1 };//记录每个voice当前属于哪个midi note，-1表示不属于任何note了
		int isVoiceActive[MaxPolyphony] = { 0 };
		int nextVoiceIdx = 0;//最坏情况下使用循环分配voice。一般情况优先寻找不活动的voice
		int midiNumNoteOn = 0;//用于检测是否有按键按下，进而决定是否更新包络状态
	public:
		Dirtynth()
		{
			for (int i = 0; i < MaxPolyphony; ++i)
				voices[i].SetMutantThreadPool(&mutantThreadPool);
		}
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
		int FindNextVoiceIdx()
		{
			for (int i = 0; i < MaxPolyphony; ++i)
				if (!isVoiceActive[i])
					return i;
			int idx = nextVoiceIdx;
			nextVoiceIdx = (nextVoiceIdx + 1) % MaxPolyphony;
			return idx;
		}
		void UpdateAllVoiceEnvelope_NoteOn(DirtynthVoice& voice)//在这里面更新包络状态，包络有全局和复音模式
		{
			//先处理全局模式
			for (int i = 0; i < MaxPolyphony; ++i)
			{
				auto enves = voices[i].GetSelectedEnvelops(params);
				for (int j = 0; j < NumEnvelopes; ++j)
				{
					if (static_cast<EnvelopeMode>((int)params.enveParams[j].enveMode) == EnvelopeMode::GlobalResetOnFirstNoteOn)//全局，按下第一个键开始
						if (midiNumNoteOn == 0)
							enves[j]->SetNoteState(1);
					if (static_cast<EnvelopeMode>((int)params.enveParams[j].enveMode) == EnvelopeMode::GlobalResetOnAnyNoteOn)//全局，按下任意键开始
						enves[j]->SetNoteState(1);
				}
			}
			//再处理复音模式
			auto enves = voice.GetSelectedEnvelops(params);
			for (int j = 0; j < NumEnvelopes; ++j)
			{
				if (static_cast<EnvelopeMode>((int)params.enveParams[j].enveMode) == EnvelopeMode::PolyphonicResetOnNoteOn)//复音，按下时开始
					enves[j]->SetNoteState(1);
			}
		}
		void UpdateAllVoiceEnvelope_NoteOff(DirtynthVoice& voice)//在这里面更新包络状态，包络有全局和复音模式
		{
			//先处理全局模式
			for (int i = 0; i < MaxPolyphony; ++i)
			{
				auto enves = voices[i].GetSelectedEnvelops(params);
				for (int j = 0; j < NumEnvelopes; ++j)
				{
					if (static_cast<EnvelopeMode>((int)params.enveParams[j].enveMode) == EnvelopeMode::GlobalResetOnFirstNoteOn ||
						static_cast<EnvelopeMode>((int)params.enveParams[j].enveMode) == EnvelopeMode::GlobalResetOnAnyNoteOn)
						if (midiNumNoteOn == 0)//全局都是松开最后一个键时重置
							enves[j]->SetNoteState(0);
				}
			}
			//再处理复音模式
			auto enves = voice.GetSelectedEnvelops(params);
			for (int j = 0; j < NumEnvelopes; ++j)
			{
				if (static_cast<EnvelopeMode>((int)params.enveParams[j].enveMode) == EnvelopeMode::PolyphonicResetOnNoteOn)//复音，按下时开始
					enves[j]->SetNoteState(0);
			}
		}
		void ProcessBlock(DirtynthMidiEvent* midiQueue, int numMidiEvents, float* outl, float* outr, int numSamples)
		{
			/*PROCESS MIDI*/
			for (int i = 0; i < numMidiEvents; ++i)
			{
				AutoFlagVoiceState();//每处理一个midi事件就更新一次voice状态
				if (midiQueue[i].type == DirtynthMidiEvent::NoteOn)
				{
					float freq = 440.0f * powf(2.0f, (midiQueue[i].note - 69) / 12.0f);
					float velocity = midiQueue[i].velocity / 127.0f;
					int nextIdx = FindNextVoiceIdx();
					DirtynthVoice& nextVoice = voices[nextIdx];
					nextVoice.SetVoiceState(true, freq, velocity);
					UpdateAllVoiceEnvelope_NoteOn(nextVoice);
					midiNumNoteOn++;//从0开始
					voiceBelongNote[nextIdx] = midiQueue[i].note;
				}
				else if (midiQueue[i].type == DirtynthMidiEvent::NoteOff)
				{
					for (int j = 0; j < MaxPolyphony; ++j)
					{
						if (voiceBelongNote[j] == midiQueue[i].note)
						{
							midiNumNoteOn--;//从0结束
							if (midiNumNoteOn < 0)midiNumNoteOn = 0;
							DirtynthVoice& thisVoice = voices[j];
							thisVoice.SetVoiceState(false, 0.0, 0.0);//!voice在note off时不会改变内置freq和velo!
							UpdateAllVoiceEnvelope_NoteOff(voices[j]);
							voiceBelongNote[j] = -1;
						}
					}
				}
			}
			/*------------*/

			/*PROCESS VOICES*/
			for (int i = 0; i < numSamples; ++i)
			{
				outl[i] = 0.0f;
				outr[i] = 0.0f;
			}
			for (int i = 0; i < MaxPolyphony; ++i)
			{
				voices[i].ProcessBlockAccumulating(params, outl, outr, numSamples);
			}
			/*--------------*/
		}
	};
}