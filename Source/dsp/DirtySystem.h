#pragma once

#include <array>

#include "Wavetable.h"
#include "Filter.h"
#include "Envelope.h"

namespace Dirtynth
{
	using namespace MinusMKI;

	constexpr static int NumEnvelopes = 6;
	constexpr static int NumEffects = 2;

	constexpr static int EnvelopeUpdateInterval = 6;//这个不是越大越好
	constexpr static int MaxPolyphony = 8;
	constexpr static int NumMutantThreads = 2;//根据平台cpu填

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

	/*参数系统*/
	struct DirtynthParams //这次用自然单位，不用归一化单位
	{
		float masterVol = 0;//-60dB->12dB def:0
		float octave = 0;//-4->4 def:0

		struct OscParams
		{
			float/*int*/ oscWtPreset = 0;//0->WavetableGenerator::NumWavetablePresets-1 def:0
			float oscWtPos = 0.0;//0->1 def:0
			struct MutantParams
			{
				float/*int*/ mutantType = 0;//0->RegMutant::NumRegMutant-1 def:0
				float p1 = 0, p2 = 0, p3 = 0;//0->1 def:0
			}mutantA, mutantB;
			float oscPitch = 0.0; //-48->48 def:0
			float oscDetune = 0.0; //-100->100 def:0
		}osc1Params, osc2Params;
		float pmDepth = 0.0; //0->100 def:0
		float osc1gain = 1.0; //0->1 def:0
		float osc2gain = 1.0; //0->1 def:0

		struct FiltParams
		{
			float/*int*/ type = 0;//0->RegFilter::NumRegFilter-1 def:0
			float cutoff = 0.0; //0->22000 def:22000
			float keyTrack = 0.0; //0->1 def:0
			float reso = 0.0; //0.707->40 def:0.707 
			float morph = 0.0;//0->1 def:0
		}filt1Params, filt2Params;
		float filt1gain = 1.0;//0->1 def:0
		float filt2gain = 1.0;//0->1 def:0
		float outputAmp = 1.0;//0->1 def:0
		float/*int*/ sysTopo = 0.0;//0->3 def:0

		struct EnveParams
		{
			float/*int*/ type = 0.0;//0->RegEnvelope::NumRegEnvelope-1 def:0
			float/*int*/ mode = 0.0;//0->4 def:0
			float/*int*/ targetID = 0.0;//def:0
			float amount = 0.0;//0->1 def:0
			float p1 = 0.0;//这个根据具体type来定义语义
			float p2 = 0.0;
			float p3 = 0.0;
			float p4 = 0.0;
			float p5 = 0.0;
			float p6 = 0.0;
		}enveParams[NumEnvelopes];

		struct EffectParams
		{
			float/*int*/ type = 0;//0->3 def:0
			float p1 = 0;//这个根据具体type来定义语义
			float p2 = 0;
			float p3 = 0;
			float p4 = 0;
			float mix = 0.0;//0->1 def:0
		}effectParams[NumEffects];
	};
	struct DirtynthParamSystem
	{
		enum ParamType
		{
			Linear = 0,
			Exp01 = 1,
			NyquistFreqExp = 2,
			FilterQ = 3,
			TimeMsExp = 4
		};
		struct ParamDesc
		{
			std::string indexName = "index name";
			std::string name = "display name";
			int id = 0;
			int isModulated = 0;
			ParamType paramType = Linear;
			float minValue = 0.0;
			float maxValue = 1.0;
			float defValue = 0.0;
		};
		DirtynthParams ref;
		static std::map<std::string, int> indexNameIdMap;//索引名字查询id
		static std::vector<size_t> idOffsetMap;//id查询偏移量，这个用来获取引用
		int maxIndex = 0;
		void RegParam(float& paramRef, const std::string& indexName)
		{
			size_t offset = reinterpret_cast<char*>(&paramRef) -
				reinterpret_cast<char*>(&ref);
			idOffsetMap.push_back(offset);
			indexNameIdMap[indexName] = maxIndex;
			maxIndex++;
		}
		void BuildParamTable()
		{
			indexNameIdMap.clear();
			idOffsetMap.clear();
			idOffsetMap.push_back(0);
			maxIndex = 1;
			RegParam(ref.masterVol, "masterVol");
			RegParam(ref.octave, "octave");
			//osc1
			RegParam(ref.osc1Params.oscWtPreset, "osc1WtPreset");
			RegParam(ref.osc1Params.oscWtPos, "osc1WtPos");
			RegParam(ref.osc1Params.mutantA.mutantType, "osc1MutAType");
			RegParam(ref.osc1Params.mutantA.p1, "osc1MutAP1");
			RegParam(ref.osc1Params.mutantA.p2, "osc1MutAP2");
			RegParam(ref.osc1Params.mutantA.p3, "osc1MutAP3");
			RegParam(ref.osc1Params.mutantB.mutantType, "osc1MutBType");
			RegParam(ref.osc1Params.mutantB.p1, "osc1MutBP1");
			RegParam(ref.osc1Params.mutantB.p2, "osc1MutBP2");
			RegParam(ref.osc1Params.mutantB.p3, "osc1MutBP3");
			RegParam(ref.osc1Params.oscPitch, "osc1Pitch");
			RegParam(ref.osc1Params.oscDetune, "osc1Detune");
			//osc2
			RegParam(ref.osc2Params.oscWtPreset, "osc2WtPreset");
			RegParam(ref.osc2Params.oscWtPos, "osc2WtPos");
			RegParam(ref.osc2Params.mutantA.mutantType, "osc2MutAType");
			RegParam(ref.osc2Params.mutantA.p1, "osc2MutAP1");
			RegParam(ref.osc2Params.mutantA.p2, "osc2MutAP2");
			RegParam(ref.osc2Params.mutantA.p3, "osc2MutAP3");
			RegParam(ref.osc2Params.mutantB.mutantType, "osc2MutBType");
			RegParam(ref.osc2Params.mutantB.p1, "osc2MutBP1");
			RegParam(ref.osc2Params.mutantB.p2, "osc2MutBP2");
			RegParam(ref.osc2Params.mutantB.p3, "osc2MutBP3");
			RegParam(ref.osc2Params.oscPitch, "osc2Pitch");
			RegParam(ref.osc2Params.oscDetune, "osc2Detune");
			//osc setting
			RegParam(ref.pmDepth, "pmDepth");
			RegParam(ref.osc1gain, "osc1gain");
			RegParam(ref.osc2gain, "osc2gain");
			//filt1
			RegParam(ref.filt1Params.type, "filt1Type");
			RegParam(ref.filt1Params.cutoff, "filt1Cutoff");
			RegParam(ref.filt1Params.keyTrack, "filt1KeyTrack");
			RegParam(ref.filt1Params.reso, "filt1Reso");
			RegParam(ref.filt1Params.morph, "filt1Morph");
			//filt2
			RegParam(ref.filt2Params.type, "filt2Type");
			RegParam(ref.filt2Params.cutoff, "filt2Cutoff");
			RegParam(ref.filt2Params.keyTrack, "filt2KeyTrack");
			RegParam(ref.filt2Params.reso, "filt2Reso");
			RegParam(ref.filt2Params.morph, "filt2Morph");
			//filt setting
			RegParam(ref.filt1gain, "filt1gain");
			RegParam(ref.filt2gain, "filt2gain");
			RegParam(ref.outputAmp, "outputAmp");
			RegParam(ref.sysTopo, "sysTopo");
			//envelopes
			for (int i = 0; i < NumEnvelopes; i++)
			{
				std::string prefix = "enve" + std::to_string(i + 1);
				RegParam(ref.enveParams[i].type, prefix + "Type");
				RegParam(ref.enveParams[i].mode, prefix + "Mode");
				RegParam(ref.enveParams[i].targetID, prefix + "TargetID");
				RegParam(ref.enveParams[i].amount, prefix + "Amount");
				RegParam(ref.enveParams[i].p1, prefix + "P1");
				RegParam(ref.enveParams[i].p2, prefix + "P2");
				RegParam(ref.enveParams[i].p3, prefix + "P3");
				RegParam(ref.enveParams[i].p4, prefix + "P4");
				RegParam(ref.enveParams[i].p5, prefix + "P5");
				RegParam(ref.enveParams[i].p6, prefix + "P6");
			}
			//effects
			for (int i = 0; i < NumEffects; i++)
			{
				std::string prefix = "effect" + std::to_string(i + 1);
				RegParam(ref.effectParams[i].type, prefix + "Type");
				RegParam(ref.effectParams[i].p1, prefix + "P1");
				RegParam(ref.effectParams[i].p2, prefix + "P2");
				RegParam(ref.effectParams[i].p3, prefix + "P3");
				RegParam(ref.effectParams[i].p4, prefix + "P4");
				RegParam(ref.effectParams[i].mix, prefix + "Mix");
			}
		}
		DirtynthParamSystem()
		{
			BuildParamTable();
		}
		int SearchID(std::string name)
		{
			return indexNameIdMap[name];
		}
		float& GetParamRefByID(DirtynthParams& paramsRef, int ID)
		{
			return *reinterpret_cast<float*>(
				reinterpret_cast<char*>(&paramsRef) + idOffsetMap[ID]);
		}
		std::vector<ParamDesc> GetParamList(DirtynthParams& paramsRef)
		{
			//根据可复用参数的type去生成参数列表，其中indexName不会变，但name可能变，并且min,max,defValue可能变
			//这个函数是给外部(ui或序列化系统)使用，dsp只使用id索引系统
			//ui拿到一个参数包，要通过这个函数得到描述
		}
		DirtynthParams GetModulatorAmountMultiplier(DirtynthParams param)
		{
			//根据当前的参数生成调制倍率。包络出来的范围是0-1，amount的范围是-1到1，需要映射成参数具体单位。
		}
	};

	/*TOOLS FUNCTION*/
	constexpr static float CutoffMin = 20.0;
	constexpr static float CutoffMax = 22000.0;
	constexpr static float ResoMin = 0.707;
	constexpr static float ResoMax = 40.0;
	constexpr static float EnveTimeMaxMs = 60000;
	constexpr static float EnveExpShape = 4.0;
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
		float x = expf((Clamp01(param) - 1.0) * EnveExpShape);
		const static float expShape = expf(-EnveExpShape);
		float y = (x - expShape) / (1.0f - expShape);
		return y * EnveTimeMaxMs;
	}
	inline float EnveTimeToParam(float time)
	{
		float y = Clamp01(time / EnveTimeMaxMs);
		const static float expShape = expf(-EnveExpShape);
		float x = y * (1.0f - expShape) + expShape;
		float param = 1.0f + logf(x) / EnveExpShape;
		return Clamp01(param);
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
			float* intMagtable/*tableWidth*2*/, int tableWidth)//输出表参数
		{
			MutantTask pack = { oscIndex, wtgenPreset, wtgenPos, typeA, pA1, pA2, pA3, typeB, pB1, pB2, pB3, intMagtable, tableWidth };
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
						mutantA->SetMutantParams(task.pA1, task.pA2, task.pA3);
						mutantA->Apply(localTable, task.tableWidth);
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
			float pmBase = voiceDtBase * 10.0;
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
						osc1MutantTaskID = mutantThreadPool->SubmitMutantTask(1,
							params.osc1Params.oscWtPreset, params.osc1Params.oscWtPos,
							selectedOsc1MutantAType, params.osc1Params.mutantA.p1, params.osc1Params.mutantA.p2, params.osc1Params.mutantA.p3,
							selectedOsc1MutantBType, params.osc1Params.mutantB.p1, params.osc1Params.mutantB.p2, params.osc1Params.mutantB.p3,
							osc1.GetNextIntMagtable(), WTOscillator::TableWidth);
					}
					if (osc2MutantTaskID == -1 && osc2.IsSwapTablePrepared())
					{
						osc2MutantTaskID = mutantThreadPool->SubmitMutantTask(2,
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
					float cutoffTrackValue1 = powf(voicefreq / 55.0, params.filt1Params.keyTrack);
					float cutoffTrackValue2 = powf(voicefreq / 55.0, params.filt1Params.keyTrack);
					filter1.SetFilterParams(
						ParamToCutoff(params.filt1Params.cutoff) * cutoffTrackValue1,
						ParamToReso(params.filt1Params.reso), params.filt1Params.morph);
					filter2.SetFilterParams(
						ParamToCutoff(params.filt2Params.cutoff) * cutoffTrackValue2,
						ParamToReso(params.filt2Params.reso), params.filt2Params.morph);
					//更新oscillator频率
					osc1dt = voiceDtBase * powf(2.0, (params.osc1Params.oscPitch + params.osc1Params.oscDetune) / 12.0);
					osc2dt = voiceDtBase * powf(2.0, (params.osc2Params.oscPitch + params.osc2Params.oscDetune) / 12.0);
				}
				/*OSC PROCESS*/
				float osc1out = osc1.ProcessSample(osc1dt);
				float osc1pmv = params.pmDepth /* *pmBase* 100.0*/ * osc1out;//乘voiceDtBase让pm输入幅度与频率去相关
				if (osc1pmv > 60.0)osc1pmv = 60.0;
				if (osc1pmv < -60.0)osc1pmv = -60.0;
				float osc2out = osc2.ProcessSample(osc2dt, osc1pmv);
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
		int isNoteActive[128] = { 0 };
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
			for (int i = 0; i < MaxPolyphony; ++i)//先解决掉不响的
				if (!isVoiceActive[i])
					return i;
			for (int i = 0; i < MaxPolyphony; ++i)//再解决掉正在响但不按键的
				if (!isNoteActive[voiceBelongNote[i]])
					return i;
			int idx = nextVoiceIdx;
			nextVoiceIdx = (nextVoiceIdx + 1) % MaxPolyphony;//没有了只能循环分配
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
							UpdateAllVoiceEnvelope_NoteOff(voices[j]);
							voiceBelongNote[j] = -1;
							isNoteActive[midiQueue[i].note] = 0;
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
			float masterVol = params.masterVol * 0.005;
			for (int i = 0; i < numSamples; ++i)
			{
				outl[i] *= masterVol;
				outr[i] *= masterVol;
			}
			/*--------------*/
		}
	};
}