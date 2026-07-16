#pragma once

#include <array>
#include <memory>
#include "Wavetable.h"
#include "FreqShifter.h"
#include "Filter.h"
#include "Envelope.h"
#include "DirtyParams.h"
#include "Platform.h"
#include "DirtyDef.h"

namespace Dirtynth
{
	using namespace MinusMKI;

	struct RegMutant
	{
		constexpr static int NumRegMutant = 5;
		constexpr static const char* MutantNames[RegMutant::NumRegMutant] = { "HardSync","SelfPM","Kickizer","Disperser","VirusGrain" };
		std::vector<std::shared_ptr<TableMutant>> regMutant{
			std::make_shared<TableMutantSync<TableWidth>>(),
			std::make_shared<TableMutantSelfPM<TableWidth>>(),
			std::make_shared<TableMutantKickizer<TableWidth>>(),
			std::make_shared<TableMutantDisperser<TableWidth>>(),
			std::make_shared<TableMutantVirusGrain<TableWidth>>() };
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
		constexpr static int NumRegFilter = 7;
		constexpr static const char* FilterNames[RegFilter::NumRegFilter] = { "SVF12dB","SVF24dB","Ellip6order","Comb","Comb4Stage","RSModal","Phaser" };
		std::vector<std::shared_ptr<Filter>> regFilter{
			std::make_shared<SVFilter12dB>(),
			std::make_shared<SVFilter24dB>(),
			std::make_shared<Elliptic6order>(),
			std::make_shared<CombFilter>(),
			std::make_shared<CombFilter4Stage>(),
			std::make_shared<RigidStringModal>(),
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
		constexpr static int NumRegEnvelope = 8;
		constexpr static const char* EnvelopeNames[RegEnvelope::NumRegEnvelope] = { "ADSR" ,"Pitch","Velocity","Aftertouch","CC1","CC2","CV1","CV2" };
		std::vector<std::shared_ptr<Envelope>> regEnvelope{
			std::make_shared<ADSR>(ControlSourceType::NoteGate),
			std::make_shared<ModSource>(ControlSourceType::NotePitch),
			std::make_shared<ModSource>(ControlSourceType::Velocity),
			std::make_shared<ModSource>(ControlSourceType::Aftertouch),
			std::make_shared<ModSource>(ControlSourceType::CC1),
			std::make_shared<ModSource>(ControlSourceType::CC2),
			std::make_shared<ModSource>(ControlSourceType::CV1),
			std::make_shared<ModSource>(ControlSourceType::CV2)
		};

		std::shared_ptr<Envelope> operator[](std::size_t index)
		{
			return regEnvelope[index];
		}
		std::shared_ptr<const Envelope> operator[](std::size_t index) const
		{
			return regEnvelope[index];
		}
	};



	class MutantThreadPool//异步计算mutant线程池
	{
	private:
		std::thread threads[NumMutantThreads];
		WavetableGenerator wtgenOsc1[NumMutantThreads];//这个计算很重，准备两套给osc1和osc2
		WavetableGenerator wtgenOsc2[NumMutantThreads];
		RegMutant regMutant[NumMutantThreads];
		struct MutantTask
		{
			int oscIndex;

			int wtgenPreset;
			float wtgenPos; //WavetableGenerator 0-1
			int typeA;
			float pA1, pA2, pA3;//mutantA
			int typeB;
			float pB1, pB2, pB3;//mutantB

			float* intMagtable;
			int tableWidth;

			double ts;
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
			int oscIndex,
			int wtgenPreset, float wtgenPos,//WavetableGenerator参数
			int typeA, float pA1, float pA2, float pA3,//mutantA参数
			int typeB, float pB1, float pB2, float pB3,//mutantB参数
			float* intMagtable/*tableWidth*2*/, int tableWidth,//输出表参数
			double ts)//时钟
		{
			MutantTask pack = { oscIndex, wtgenPreset, wtgenPos, typeA, pA1, pA2, pA3, typeB, pB1, pB2, pB3, intMagtable, tableWidth, ts };
			MutantTask* nextTask = nullptr;
			int taskID = -1;
			for (int i = 0; i < MaxQueueLen; ++i)
			{
				int idx = (submitIdx + i) % MaxQueueLen;
				if (taskFlags[idx].load() == 0)//找到空任务槽
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
				return -1;//找不到空任务槽就不添加这个任务
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
			Platform::BindToCPU(threadId + 1);//从1核开始
			float* localTable = new float[TableWidth * 2];
			WavetableGenerator& wtGeneratorOsc1 = wtgenOsc1[threadId];
			WavetableGenerator& wtGeneratorOsc2 = wtgenOsc2[threadId];
			int wtgenPresetOsc1 = 0;
			int wtgenPresetOsc2 = 0;
			wtGeneratorOsc1.Generate(wtgenPresetOsc1);
			wtGeneratorOsc2.Generate(wtgenPresetOsc2);
			while (isRunning)
			{
				for (int i = threadId; i < MaxQueueLen; i += NumMutantThreads)
				{
					MutantTask& task = taskQueue[i];
					if (taskFlags[i].load() == 1)
					{
						TableMutant* mutantA = regMutant[threadId][task.typeA].get();
						TableMutant* mutantB = regMutant[threadId][task.typeB].get();

						WavetableGenerator& wtGenerator = (task.oscIndex == 1) ? wtGeneratorOsc1 : wtGeneratorOsc2;
						if (task.oscIndex == 1 && task.wtgenPreset != wtgenPresetOsc1)//只在preset有变化时更新
						{
							wtGenerator.Generate(task.wtgenPreset);
							wtgenPresetOsc1 = task.wtgenPreset;
						}
						if (task.oscIndex == 2 && task.wtgenPreset != wtgenPresetOsc2)
						{
							wtGenerator.Generate(task.wtgenPreset);
							wtgenPresetOsc2 = task.wtgenPreset;
						}

						float* wtgenTable = wtGenerator.GetTable(task.wtgenPos * 63.0);//0-1!
						for (int j = 0; j < task.tableWidth; ++j)localTable[j] = wtgenTable[j];
						mutantA->SetTime(task.ts);
						mutantA->SetMutantParams(task.pA1, task.pA2, task.pA3);
						mutantA->Apply(localTable, task.tableWidth);
						mutantB->SetTime(task.ts);
						mutantB->SetMutantParams(task.pB1, task.pB2, task.pB3);
						mutantB->Apply(localTable, task.tableWidth);
						WTOscillator::CalcIntMagtable(task.intMagtable, localTable, task.tableWidth);
						taskFlags[i].store(2);
					}
				}
				std::this_thread::sleep_for(std::chrono::nanoseconds(20000));//cpu你幸苦了！
			}
		}
	};

	class DirtynthVoice
	{
	private:
		float sampleRate = 48000.0;
		WTOscillator osc1, osc2;
		FreqShifter freqShifter;
		RegFilter regFilt1, regFilt2;
		RegEnvelope regEnves[NumEnvelopes];
		int sampleCount = 0;
		/*VOICE RUNTIME STATE*/
		float voicefreq = 0.0;
		float osc1dt = 0.0;
		float osc2dt = 0.0;
		float voiceVel = 1.0;
		float voiceStateVolume = 0.0;//用于检测活动状态
		DirtynthParams::OscParams::MutantParams lastOsc1MutA, lastOsc1MutB;//用于检测mutant参数变化
		DirtynthParams::OscParams::MutantParams lastOsc2MutA, lastOsc2MutB;
		int isNoteOn = 0;

		int osc1MutantTaskID = -1;
		int osc2MutantTaskID = -1;
		/*-------------------*/
		MutantThreadPool* mutantThreadPool;
		DirtynthParamSystem* paramTools;
		DirtynthParams* amountMul;

		DirtynthParams params;
		Filter* filter1 = std::dynamic_pointer_cast<Filter>(regFilt1[0]).get();
		Filter* filter2 = std::dynamic_pointer_cast<Filter>(regFilt2[0]).get();

		double t = 0.0;
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
		void SetParamTools(DirtynthParamSystem* tools)
		{
			paramTools = tools;
		}
		void SetAmountMultiplier(DirtynthParams* amountMult)
		{
			this->amountMul = amountMult;
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

		Envelope* enves[NumEnvelopes];
		float tmpval;
		float* modTarget1[NumEnvelopes * 2];
		float* modTarget2[NumEnvelopes * 2];
		float modOrigin1[NumEnvelopes * 2];
		float modOrigin2[NumEnvelopes * 2];
		float modAmount1[NumEnvelopes * 2];
		float modAmount2[NumEnvelopes * 2];
		float modAmountMul1[NumEnvelopes * 2];
		float modAmountMul2[NumEnvelopes * 2];
		inline void InitModulator(DirtynthParams& target, DirtynthParams& origin, DirtynthParams& amountMul)
		{
			for (int i = 0; i < NumEnvelopes; ++i) enves[i] = regEnves[i][(int)origin.enveParams[i].type].get();
			for (int j = 0; j < NumEnvelopes; ++j)
			{
				float p1 = origin.enveParams[j].p1;
				float p2 = origin.enveParams[j].p2;
				float p3 = origin.enveParams[j].p3;
				float p4 = origin.enveParams[j].p4;
				float p5 = origin.enveParams[j].p5;
				float p6 = origin.enveParams[j].p6;
				enves[j]->SetParams(p1, p2, p3, p4, p5, p6);
			}
			for (int i = 0; i < NumEnvelopes; ++i)
			{
				if ((int)origin.enveParams[i].targetID1 < 1.0) modTarget1[i] = &tmpval;
				else modTarget1[i] = &paramTools->GetParamRefByID(target, origin.enveParams[i].targetID1);
				if ((int)origin.enveParams[i].targetID2 < 1.0) modTarget2[i] = &tmpval;
				else modTarget2[i] = &paramTools->GetParamRefByID(target, origin.enveParams[i].targetID2);
				modOrigin1[i] = paramTools->GetParamRefByID(origin, origin.enveParams[i].targetID1);
				modOrigin2[i] = paramTools->GetParamRefByID(origin, origin.enveParams[i].targetID2);
				modAmountMul1[i] = paramTools->GetParamRefByID(amountMul, origin.enveParams[i].targetID1);
				modAmountMul2[i] = paramTools->GetParamRefByID(amountMul, origin.enveParams[i].targetID2);
				modAmount1[i] = origin.enveParams[i].amount1;
				modAmount2[i] = origin.enveParams[i].amount2;
			}
		}
		inline void ApplyModulator()
		{
			for (int j = 0; j < NumEnvelopes; ++j)
			{
				float enveval = enves[j]->GetValue();
				*modTarget1[j] = modOrigin1[j] + enveval * modAmount1[j] * modAmountMul1[j];
				*modTarget2[j] = modOrigin2[j] + enveval * modAmount2[j] * modAmountMul2[j];
			}
		}
		inline void UpdateParamsToDSP()
		{
			//检查oscillator的intMagtable是否需要更新，并更新
			if (osc1MutantTaskID == -1 && osc1.IsSwapTablePrepared())
			{
				osc1MutantTaskID = mutantThreadPool->SubmitMutantTask(1,
					params.osc1Params.oscWtPreset, params.osc1Params.oscWtPos,
					params.osc1Params.mutantA.mutantType, params.osc1Params.mutantA.p1, params.osc1Params.mutantA.p2, params.osc1Params.mutantA.p3,
					params.osc1Params.mutantB.mutantType, params.osc1Params.mutantB.p1, params.osc1Params.mutantB.p2, params.osc1Params.mutantB.p3,
					osc1.GetNextIntMagtable(), TableWidth, t);
			}
			if (osc2MutantTaskID == -1 && osc2.IsSwapTablePrepared())
			{
				osc2MutantTaskID = mutantThreadPool->SubmitMutantTask(2,
					params.osc2Params.oscWtPreset, params.osc2Params.oscWtPos,
					params.osc2Params.mutantA.mutantType, params.osc2Params.mutantA.p1, params.osc2Params.mutantA.p2, params.osc2Params.mutantA.p3,
					params.osc2Params.mutantB.mutantType, params.osc2Params.mutantB.p1, params.osc2Params.mutantB.p2, params.osc2Params.mutantB.p3,
					osc2.GetNextIntMagtable(), TableWidth, t);
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
			float cutoffTrackValue1 = powf(voicefreq / 55.0, params.filt1Params.keyTrack);
			float cutoffTrackValue2 = powf(voicefreq / 55.0, params.filt2Params.keyTrack);
			filter1->SetFilterParams(
				params.filt1Params.cutoff * cutoffTrackValue1,
				params.filt1Params.reso, params.filt1Params.morph);
			filter2->SetFilterParams(
				params.filt2Params.cutoff * cutoffTrackValue2,
				params.filt2Params.reso, params.filt2Params.morph);
			//更新oscillator频率
			float voiceDtBase = voicefreq / sampleRate;
			osc1dt = voiceDtBase * powf(2.0, (params.osc1Params.oscPitch + params.osc1Params.oscDetune / 100.0f) / 12.0);
			osc2dt = voiceDtBase * powf(2.0, (params.osc2Params.oscPitch + params.osc2Params.oscDetune / 100.0f) / 12.0);
		}
		void ProcessBlockAccumulating(DirtynthParams& paramsInput, float* outl, float* outr, int numSamples)//输出直接叠加在原块上
		{
			params = paramsInput;
			filter1 = std::dynamic_pointer_cast<Filter>(regFilt1[params.filt1Params.type]).get();
			filter2 = std::dynamic_pointer_cast<Filter>(regFilt2[params.filt2Params.type]).get();

			/*将被调制的参数指针，以及原始的参数指针打包*/
			InitModulator(params, paramsInput, *amountMul);
			ApplyModulator();
			UpdateParamsToDSP();

			/*PROCESSOR*/
			float voiceDtBase = voicefreq / sampleRate;
			float fmBase = voiceDtBase * 10.0;
			voiceStateVolume = 0.0;
			int systopo = params.sysTopo;
			freqShifter.Normalized();//这个偶尔归一化一下
			for (int i = 0; i < numSamples; ++i)
			{
				t += 1.0 / sampleRate;

				sampleCount++;
				if (sampleCount >= EnvelopeUpdateInterval)
				{
					sampleCount = 0;
					for (int j = 0; j < NumEnvelopes; ++j) enves[j]->Step();//步进一次包络
					ApplyModulator();//更新包络并应用到参数
					UpdateParamsToDSP();//将参数更新到dsp
				}
				/*OSC PROCESS*/
				float osc1out = osc1.ProcessSample(osc1dt);
				float osc1fmv = osc1out * params.pmDepth * fmBase * 1000.0;//乘voiceDtBase让pm输入幅度与频率去相关
				float osc2out = osc2.ProcessSample(osc2dt);
				osc2out = freqShifter.ProcessSampleFM(osc2out, osc1fmv);
				osc1out *= params.osc1gain;
				osc2out *= params.osc2gain;

				/*FILTER PROCESS*/
				float filt1in = (systopo == 0 || systopo == 1) ? osc1out : osc1out + osc2out;
				float filt1out = filter1->ProcessSample(filt1in) * params.filt1gain;
				float filt2in = 0;
				if (systopo == 0)filt2in = osc2out;
				else if (systopo == 1)filt2in = osc2out + filt1out;
				else if (systopo == 2)filt2in = filt1out;
				else if (systopo == 3)filt2in = osc1out + osc2out;
				float filt2out = filter2->ProcessSample(filt2in) * params.filt2gain;
				float filtout = (systopo == 0 || systopo == 3) ? filt1out + filt2out : filt2out;
				filtout *= params.outputAmp;

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
				voiceVel = velocity * velocity;
				sampleCount = EnvelopeUpdateInterval;//立即更新包络
				filter1->Reset();
				filter2->Reset();
			}
		}
		std::array<Envelope*, NumEnvelopes> GetSelectedEnvelops(const DirtynthParams& params)//根据params返回当前选中的包络指针数组，方便外部函数调用
		{
			std::array<Envelope*, NumEnvelopes> enves{};
			for (int i = 0; i < NumEnvelopes; ++i)
			{
				int envelopeType = static_cast<int>(params.enveParams[i].type);
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
		DirtynthParams amountMul;
		DirtynthParamSystem paramTools;

		DirtynthVoice voices[MaxPolyphony];
		MutantThreadPool mutantThreadPool;
		int voiceBelongNote[MaxPolyphony] = { -1 };//记录每个voice当前属于哪个midi note，-1表示不属于任何note了
		int isVoiceActive[MaxPolyphony] = { 0 };
		int isNoteActive[128] = { 0 };
		int nextVoiceIdx = 0;//最坏情况下使用循环分配voice。一般情况优先寻找不活动的voice
		int midiNumNoteOn = 0;//用于检测是否有按键按下，进而决定是否更新包络状态
	public:
		DirtynthSystem()
		{
			Platform::BindToCPU(0);
			for (int i = 0; i < MaxPolyphony; ++i)
			{
				voices[i].SetMutantThreadPool(&mutantThreadPool);
				voices[i].SetParamTools(&paramTools);
				voices[i].SetAmountMultiplier(&amountMul);
			}
		}
		void SetSampleRate(float sr)
		{
			for (int i = 0; i < MaxPolyphony; ++i)
				voices[i].SetSampleRate(sr);
		}
		void SetParams(const DirtynthParams& newParams)
		{
			nextParams = newParams;
			amountMul = paramTools.GetModulatorAmountMultiplier(nextParams);
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
			for (int i = 0; i < MaxPolyphony; ++i)//先解决掉不响的
				if (!isVoiceActive[i])
					return i;
			for (int i = 0; i < MaxPolyphony; ++i)//再解决掉正在响但不按键的
				if (voiceBelongNote[i] == -1)
					return i;
			int idx = nextVoiceIdx;
			nextVoiceIdx = (nextVoiceIdx + 1) % MaxPolyphony;//没有了只能循环分配
			return idx;
		}
		/*
		void UpdateVoiceEnvelope_NoteOn(DirtynthVoice& voice)//在这里面更新包络状态，包络有全局和复音模式
		{
			//先处理全局模式
			for (int i = 0; i < MaxPolyphony; ++i)
			{
				auto enves = voices[i].GetSelectedEnvelops(params);
				for (int j = 0; j < NumEnvelopes; ++j)
				{
					if (enves[j]->GetControlSourceType() != ControlSourceType::NoteGate)continue;
					if (static_cast<EnvelopeMode>((int)params.enveParams[j].mode) == EnvelopeMode::GlobalResetOnFirstNoteOn)//全局，按下第一个键开始
						if (midiNumNoteOn == 0)
							enves[j]->SetControlValue(1);
					if (static_cast<EnvelopeMode>((int)params.enveParams[j].mode) == EnvelopeMode::GlobalResetOnAnyNoteOn)//全局，按下任意键开始
						enves[j]->SetControlValue(1);
				}
			}
			//再处理复音模式
			auto enves = voice.GetSelectedEnvelops(params);
			for (int j = 0; j < NumEnvelopes; ++j)
			{
				if (enves[j]->GetControlSourceType() != ControlSourceType::NoteGate)continue;
				if (static_cast<EnvelopeMode>((int)params.enveParams[j].mode) == EnvelopeMode::PolyphonicResetOnNoteOn)//复音，按下时开始
					enves[j]->SetControlValue(1);
			}
		}
		void UpdateVoiceEnvelope_NoteOff(DirtynthVoice& voice)//在这里面更新包络状态，包络有全局和复音模式
		{
			//先处理全局模式
			for (int i = 0; i < MaxPolyphony; ++i)
			{
				auto enves = voices[i].GetSelectedEnvelops(params);
				for (int j = 0; j < NumEnvelopes; ++j)
				{
					if (enves[j]->GetControlSourceType() != ControlSourceType::NoteGate)continue;
					if (static_cast<EnvelopeMode>((int)params.enveParams[j].mode) == EnvelopeMode::GlobalResetOnFirstNoteOn ||
						static_cast<EnvelopeMode>((int)params.enveParams[j].mode) == EnvelopeMode::GlobalResetOnAnyNoteOn)
						if (midiNumNoteOn == 0)//全局都是松开最后一个键时重置
							enves[j]->SetControlValue(0);
				}
			}
			//再处理复音模式
			auto enves = voice.GetSelectedEnvelops(params);
			for (int j = 0; j < NumEnvelopes; ++j)
			{
				if (enves[j]->GetControlSourceType() != ControlSourceType::NoteGate)continue;
				if (static_cast<EnvelopeMode>((int)params.enveParams[j].mode) == EnvelopeMode::PolyphonicResetOnNoteOn)//复音，按下时开始
					enves[j]->SetControlValue(0);
			}
		}
		void UpdateVoiceMod(DirtynthVoice& voice, float cv, ControlSourceType cst)
		{
			//先处理全局模式
			for (int i = 0; i < MaxPolyphony; ++i)
			{
				auto enves = voices[i].GetSelectedEnvelops(params);
				for (int j = 0; j < NumEnvelopes; ++j)
				{
					if (enves[j]->GetControlSourceType() != cst)continue;

					if (static_cast<EnvelopeMode>((int)params.enveParams[j].mode) == EnvelopeMode::GlobalResetOnFirstNoteOn)//全局，按下第一个键开始
						if (midiNumNoteOn == 0)
							enves[j]->SetControlValue(cv);
					if (static_cast<EnvelopeMode>((int)params.enveParams[j].mode) == EnvelopeMode::GlobalResetOnAnyNoteOn)//全局，按下任意键开始
						enves[j]->SetControlValue(cv);
				}
			}
			//再处理复音模式
			auto enves = voice.GetSelectedEnvelops(params);
			for (int j = 0; j < NumEnvelopes; ++j)
			{
				if (enves[j]->GetControlSourceType() != cst)continue;
				if (static_cast<EnvelopeMode>((int)params.enveParams[j].mode) == EnvelopeMode::PolyphonicResetOnNoteOn)//复音，按下时开始
					enves[j]->SetControlValue(cv);
			}
		}
		*/
		void UpdateVoiceMod(DirtynthVoice& voice, float cv, ControlSourceType cst)
		{
			auto enves = voice.GetSelectedEnvelops(params);
			for (int j = 0; j < NumEnvelopes; ++j)
			{
				if (enves[j]->GetControlSourceType() != cst)continue;
				enves[j]->SetControlValue(cv);
			}
		}
		void ProcessBlock(DirtynthMidiEvent* midiQueue, int numMidiEvents, float* __restrict outl, float* __restrict outr, int numSamples)
		{
			params = nextParams;
			/*PROCESS MIDI*/
			for (int i = 0; i < numMidiEvents; ++i)
			{
				AutoFlagVoiceState();//每处理一个midi事件就更新一次voice状态
				if (midiQueue[i].type == DirtynthMidiEvent::NoteOn)
				{
					int octave = floorf(params.octave) * 12;//+-4 octave
					float freq = 440.0f * powf(2.0f, (midiQueue[i].note + octave - 69) / 12.0f);
					float velocity = midiQueue[i].velocity / 127.0f;
					int nextIdx = FindNextVoiceIdx();
					DirtynthVoice& nextVoice = voices[nextIdx];
					nextVoice.SetVoiceState(true, freq, velocity);
					//UpdateVoiceEnvelope_NoteOn(nextVoice);
					UpdateVoiceMod(nextVoice, 1.0, ControlSourceType::NoteGate);
					UpdateVoiceMod(nextVoice, velocity, ControlSourceType::Velocity);
					UpdateVoiceMod(nextVoice, (float)midiQueue[i].note / 127.0, ControlSourceType::NotePitch);
					midiNumNoteOn++;//从0开始
					voiceBelongNote[nextIdx] = midiQueue[i].note;
					isNoteActive[midiQueue[i].note] = 1;
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
							//UpdateVoiceEnvelope_NoteOff(voices[j]);
							UpdateVoiceMod(voices[j], 0.0, ControlSourceType::NoteGate);
							voiceBelongNote[j] = -1;
							isNoteActive[midiQueue[i].note] = 0;
						}
					}
				}
				else if (midiQueue[i].type == DirtynthMidiEvent::ControlChange)
				{
					if (midiQueue[i].controlNumber == 1)
					{
						for (auto& voice : voices)
						{
							UpdateVoiceMod(voice, midiQueue[i].controlValue / 127.0, ControlSourceType::CC1);
						}
					}
					else if (midiQueue[i].controlNumber == 2)
					{
						for (auto& voice : voices)
						{
							UpdateVoiceMod(voice, midiQueue[i].controlValue / 127.0, ControlSourceType::CC2);
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
			float masterVol = powf(10.0f, params.masterVol / 20.0f);
			for (int i = 0; i < numSamples; ++i)
			{
				outl[i] *= masterVol;
				outr[i] *= masterVol;
			}
			/*--------------*/
		}
	};
}