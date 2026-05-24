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
		constexpr static const char* MutantNames[RegMutant::NumRegMutant] = { "HardSync","SelfPM","Kickizer","Disperser" };
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
		constexpr static const char* FilterNames[RegFilter::NumRegFilter] = { "SVF12dB","SVF24dB","Ellip6order","Comb","Comb4Stage","Phaser" };
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
		constexpr static const char* EnvelopeNames[RegEnvelope::NumRegEnvelope] = { "ADSR" };
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
		float masterVol = 0.5;
		float octave = 0.5;

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
	struct ParamsNamePack
	{
		constexpr static const char* paramsName[] = {
			"MasterVol",
			"Octave",
			"Osc1 Wt Preset",
			"Osc1 Wt Pos",
			"Osc1 MutA Type",
			"Osc1 MutA P1",
			"Osc1 MutA P2",
			"Osc1 MutA P3",
			"Osc1 MutB Type",
			"Osc1 MutB P1",
			"Osc1 MutB P2",
			"Osc1 MutB P3",
			"Osc1 Pitch",
			"Osc1 Detune",
			"Osc2 Wt Preset",
			"Osc2 Wt Pos",
			"Osc2 MutA Type",
			"Osc2 MutA P1",
			"Osc2 MutA P2",
			"Osc2 MutA P3",
			"Osc2 MutB Type",
			"Osc2 MutB P1",
			"Osc2 MutB P2",
			"Osc2 MutB P3",
			"Osc2 Pitch",
			"Osc2 Detune",
			"PMDepth",
			"OscMix",
			"OscAmp",
			"Filt1 Type",
			"Filt1 Cutoff",
			"Filt1 KeyTrack",
			"Filt1 Reso",
			"Filt1 Morph",
			"Filt2 Type",
			"Filt2 Cutoff",
			"Filt2 KeyTrack",
			"Filt2 Reso",
			"Filt2 Morph",
			"Filt2 SwitchIn",
			"FiltMix",
			"Enve1 Type",
			"Enve1 Mode",
			"Enve1 Target",
			"Enve1 Amount",
			"Enve1 P1","Enve1 P2","Enve1 P3","Enve1 P4","Enve1 P5","Enve1 P6",
			"Enve2 Type",
			"Enve2 Mode",
			"Enve2 Target",
			"Enve2 Amount",
			"Enve2 P1","Enve2 P2","Enve2 P3","Enve2 P4","Enve2 P5","Enve2 P6",
			"Enve3 Type",
			"Enve3 Mode",
			"Enve3 Target",
			"Enve3 Amount",
			"Enve3 P1","Enve3 P2","Enve3 P3","Enve3 P4","Enve3 P5","Enve3 P6",
			"Enve4 Type",
			"Enve4 Mode",
			"Enve4 Target",
			"Enve4 Amount",
			"Enve4 P1","Enve4 P2","Enve4 P3","Enve4 P4","Enve4 P5","Enve4 P6",
			"Enve5 Type",
			"Enve5 Mode",
			"Enve5 Target",
			"Enve5 Amount",
			"Enve5 P1","Enve5 P2","Enve5 P3","Enve5 P4","Enve5 P5","Enve5 P6",
			"Enve6 Type",
			"Enve6 Mode",
			"Enve6 Target",
			"Enve6 Amount",
			"Enve6 P1","Enve6 P2","Enve6 P3","Enve6 P4","Enve6 P5","Enve6 P6"
		};
	};
	/*TOOLS FUNCTION*/
	constexpr static float CutoffMin = 20.0;
	constexpr static float CutoffMax = 22000.0;
	constexpr static float ResoMin = 0.707;
	constexpr static float ResoMax = 40.0;
	constexpr static float EnveTimeMaxMs = 60000;
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

	inline float ParamToEnveTime(float param)
	{
		return expf((Clamp01(param) - 1.0) * 12.0) * EnveTimeMaxMs;
	}
	inline float EnveTimeToParam(float time)
	{
		if (time < 0.001) time = 0.001;
		if (time > EnveTimeMaxMs) time = EnveTimeMaxMs;
		return 1.0 + logf(time / EnveTimeMaxMs) / 12.0;
	}
	inline float ParamToEnveShape(float param)
	{
		return (param - 0.5) * 2.0 * 10.0;
	}
	inline float EnveShapeToParam(float shape)
	{
		return Clamp01(shape / 20.0 + 0.5);
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
		std::atomic<int> taskFlags[MaxQueueLen] = { false };
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
		void FreeTask(int taskID)
		{
			if (taskID < 0 || taskID >= MaxQueueLen)return;
			taskFlags[taskID].store(0);
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
						mutantA->Apply(localTable, task.tableWidth);
						mutantB->SetMutantParams(task.pB1, task.pB2, task.pB3);
						mutantB->Apply(localTable, task.tableWidth);
						WTOscillator::CalcIntMagtable(task.intMagtable, localTable, task.tableWidth);
						taskFlags[i].store(2);
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
		float osc1dt = 0.0;
		float osc2dt = 0.0;
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
			//设置包络参数(包络参数不需要被调制)
			for (int j = 0; j < NumEnvelopes; ++j)
			{
				float p1, p2, p3, p4, p5, p6;
				if (params.enveParams[j].enveType == 0)//ADSR
				{
					p1 = ParamToEnveTime(params.enveParams[j].enveP1);//attMs
					p2 = ParamToEnveShape(params.enveParams[j].enveP2);//attShape
					p3 = ParamToEnveTime(params.enveParams[j].enveP3);//decMs
					p4 = ParamToEnveShape(params.enveParams[j].enveP4);//decShape
					p5 = params.enveParams[j].enveP5;//susLevel
					p6 = ParamToEnveTime(params.enveParams[j].enveP6);//relMs
				}
				else
				{
					p1 = params.enveParams[j].enveP1;
					p2 = params.enveParams[j].enveP2;
					p3 = params.enveParams[j].enveP3;
					p4 = params.enveParams[j].enveP4;
					p5 = params.enveParams[j].enveP5;
					p6 = params.enveParams[j].enveP6;
				}
				enves[j]->SetParams(p1, p2, p3, p4, p5, p6);
			}
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
			//根据enveTarget和enveAmount修改params
			for (int j = 0; j < NumEnvelopes; ++j)
			{
				if (paramTargetPtr[j] != nullptr)
					*paramTargetPtr[j] = *paramOriginalValue[j] + enves[j]->GetValue() * params.enveParams[j].enveAmount;
			}

			/*PROCESSOR*/
			float voiceDtBase = voicefreq / sampleRate;
			voiceStateVolume = 0.0;
			for (int i = 0; i < numSamples; ++i)
			{
				sampleCount++;
				if (sampleCount >= EnvelopeUpdateInterval)
				{
					sampleCount = 0;
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
						osc1MutantTaskID = mutantThreadPool->SubmitMutantTask(
							params.osc1Params.oscWtPreset, params.osc1Params.oscWtPos,
							selectedOsc1MutantAType, params.osc1Params.mutantA.p1, params.osc1Params.mutantA.p2, params.osc1Params.mutantA.p3,
							selectedOsc1MutantBType, params.osc1Params.mutantB.p1, params.osc1Params.mutantB.p2, params.osc1Params.mutantB.p3,
							osc1.GetNextIntMagtable(), WTOscillator::TableWidth);
					}
					if (osc2MutantTaskID == -1 && osc2.IsSwapTablePrepared())
					{
						osc2MutantTaskID = mutantThreadPool->SubmitMutantTask(
							params.osc2Params.oscWtPreset, params.osc2Params.oscWtPos,
							selectedOsc2MutantAType, params.osc2Params.mutantA.p1, params.osc2Params.mutantA.p2, params.osc2Params.mutantA.p3,
							selectedOsc2MutantBType, params.osc2Params.mutantB.p1, params.osc2Params.mutantB.p2, params.osc2Params.mutantB.p3,
							osc2.GetNextIntMagtable(), WTOscillator::TableWidth);
					}
					if (osc1MutantTaskID != -1 && mutantThreadPool->GetTaskState(osc1MutantTaskID) == 2)
					{
						osc1.SetFillCompleteFlag();
						mutantThreadPool->FreeTask(osc1MutantTaskID);
						osc1MutantTaskID = -1;
					}
					if (osc2MutantTaskID != -1 && mutantThreadPool->GetTaskState(osc2MutantTaskID) == 2)
					{
						osc2.SetFillCompleteFlag();
						mutantThreadPool->FreeTask(osc2MutantTaskID);
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
					//更新oscillator频率
					osc1dt = voiceDtBase * powf(2.0, (params.osc1Params.oscPitch + params.osc1Params.oscDetune / 100.0) / 12.0);
					osc2dt = voiceDtBase * powf(2.0, (params.osc2Params.oscPitch + params.osc2Params.oscDetune / 100.0) / 12.0);
				}
				/*OSC PROCESS*/
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

	class DirtynthSystem
	{
	public:
	private:
		DirtynthParams params, nextParams;
		DirtynthVoice voices[MaxPolyphony];
		MutantThreadPool mutantThreadPool;
		int voiceBelongNote[MaxPolyphony] = { -1 };//记录每个voice当前属于哪个midi note，-1表示不属于任何note了
		int isVoiceActive[MaxPolyphony] = { 0 };
		int nextVoiceIdx = 0;//最坏情况下使用循环分配voice。一般情况优先寻找不活动的voice
		int midiNumNoteOn = 0;//用于检测是否有按键按下，进而决定是否更新包络状态
	public:
		DirtynthSystem()
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
			nextParams = newParams;
		}
		DirtynthParams GetParams() const
		{
			return nextParams;
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
			params = nextParams;
			/*PROCESS MIDI*/
			for (int i = 0; i < numMidiEvents; ++i)
			{
				AutoFlagVoiceState();//每处理一个midi事件就更新一次voice状态
				if (midiQueue[i].type == DirtynthMidiEvent::NoteOn)
				{
					int octave = (int)((params.octave - 0.5) * 2.0 * 4.0) * 12;//+-4 octave
					float freq = 440.0f * powf(2.0f, (midiQueue[i].note + octave - 69) / 12.0f);
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
			float masterVol = params.masterVol * 0.01;
			for (int i = 0; i < numSamples; ++i)
			{
				outl[i] *= masterVol;
				outr[i] *= masterVol;
			}
			/*--------------*/
		}
	};
}