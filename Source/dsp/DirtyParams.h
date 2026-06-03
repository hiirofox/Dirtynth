#pragma once

namespace Dirtynth
{
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
		enum KnobFeelType
		{
			Linear = 0,//线性手感
			Integer = 1,//整数手感，带卡鞘的那种
			NyquistFreqExp = 2,//滤波器频率旋钮手感，类似指数手感但是特调
			FilterQ = 3,//Q手感，特调
			TimeMsExp = 4//毫秒手感，特调
		};
		struct ParamDesc
		{
			std::string indexName = "index name";
			std::string name = "display name";
			bool isModulated = 0;
			KnobFeelType paramType = Linear;
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
			std::vector<ParamDesc> descs;
			auto Push = [&](const std::string& indexName, const std::string& name,
				bool isModulated, KnobFeelType paramType,
				float minValue, float maxValue, float defValue)
				{
					descs.push_back({ indexName, name, isModulated, paramType, minValue, maxValue, defValue });
				};
			auto ClampType = [](float value, int typeCount)
				{
					int type = static_cast<int>(value);
					if (type < 0) type = 0;
					if (type >= typeCount) type = typeCount - 1;
					return type;
				};
			auto PushMutantDesc = [&](int osc, int mut)
				{
					const DirtynthParams::OscParams& oscParams =
						(osc == 1) ? paramsRef.osc1Params : paramsRef.osc2Params;
					const DirtynthParams::OscParams::MutantParams& mutParams =
						(mut == 1) ? oscParams.mutantA : oscParams.mutantB;
					const int mutType = ClampType(mutParams.mutantType, RegMutant::NumRegMutant);
					const std::string prefix = DirtyTools::OldFormat("osc%dMut%s", osc, mut == 1 ? "A" : "B");
					switch (mutType)
					{
					case 0: // HardSync: depth, phase, smooth
						Push(prefix + "P1", "Depth", true, Linear, 0.0f, 1.0f, 0.0f);
						Push(prefix + "P2", "Phase", true, Linear, 0.0f, 1.0f, 0.0f);
						Push(prefix + "P3", "Smooth", true, Linear, 0.0f, 1.0f, 0.0f);
						break;
					case 1: // SelfPM: depth, prelp, stages
						Push(prefix + "P1", "Depth", true, Linear, 0.0f, 1.0f, 0.0f);
						Push(prefix + "P2", "PreLP", true, Linear, 0.0f, 1.0f, 0.0f);
						Push(prefix + "P3", "Stages", true, Linear, 0.0f, 1.0f, 0.0f);
						break;
					case 2: // Kickizer: depth, tmix, rate
						Push(prefix + "P1", "Depth", true, Linear, 0.0f, 1.0f, 0.0f);
						Push(prefix + "P2", "Tmix", true, Linear, 0.0f, 1.0f, 0.0f);
						Push(prefix + "P3", "Rate", true, Linear, 0.0f, 1.0f, 0.0f);
						break;
					case 3: // Disperser: disperse, harmonic, comb
						Push(prefix + "P1", "Disperse", true, Linear, 0.0f, 1.0f, 0.0f);
						Push(prefix + "P2", "Harmonic", true, Linear, 0.0f, 1.0f, 0.0f);
						Push(prefix + "P3", "Comb", true, Linear, 0.0f, 1.0f, 0.0f);
						break;
					}
				};
			auto GetFilterMorphName = [&](int filterType)
				{
					switch (filterType)
					{
					case 0: // SVF12dB
					case 1: // SVF24dB
					case 2: // Ellip6order
						return "LP-BP-HP";
					case 3: // Comb
						return "AddSub";
					case 4: // Comb4Stage
						return "Spread";
					case 5: // Phaser
						return "Stages";
					default:
						return "Morph";
					}
				};
			auto PushFilterDesc = [&](int filt, const DirtynthParams::FiltParams& filtParams)
				{
					const std::string prefix = DirtyTools::OldFormat("filt%d", filt);
					const int filterType = ClampType(filtParams.type, RegFilter::NumRegFilter);
					Push(prefix + "Type", "Type", false, Integer, 0.0f, static_cast<float>(RegFilter::NumRegFilter - 1), 0.0f);
					Push(prefix + "Cutoff", "Cutoff", true, NyquistFreqExp, 20.0f, 22000.0f, 22000.0f);
					Push(prefix + "KeyTrack", "KeyTrack", true, Linear, 0.0f, 1.0f, 0.0f);
					Push(prefix + "Reso", "Reso", true, FilterQ, 0.707f, 40.0f, 0.707f);
					Push(prefix + "Morph", GetFilterMorphName(filterType), true, Linear, 0.0f, 1.0f, 0.0f);
				};
			auto PushEnvelopeDesc = [&](int enve)
				{
					const std::string prefix = DirtyTools::OldFormat("enve%d", enve + 1);
					const DirtynthParams::EnveParams& enveParams = paramsRef.enveParams[enve];
					const int enveType = ClampType(enveParams.type, RegEnvelope::NumRegEnvelope);
					const float maxTargetID = static_cast<float>(sizeof(DirtynthParams) / sizeof(float) - NumEffects * 6 - 1);
					Push(prefix + "Type", "Type", false, Integer, 0.0f, static_cast<float>(RegEnvelope::NumRegEnvelope - 1), 0.0f);
					Push(prefix + "Mode", "Mode", false, Integer, 0.0f, 4.0f, 0.0f);
					Push(prefix + "TargetID", "Target", false, Integer, -1.0f, maxTargetID, -1.0f);
					Push(prefix + "Amount", "Amount", false, Linear, -1.0f, 1.0f, 0.0f);
					if (enveType == 0) // ADSR: attMs, attShape, decMs, decShape, susV, relMs
					{
						Push(prefix + "P1", "Attack", false, TimeMsExp, 0.0f, 60000.0f, 0.0f);
						Push(prefix + "P2", "AttShape", false, Linear, -10.0f, 10.0f, 0.0f);
						Push(prefix + "P3", "Decay", false, TimeMsExp, 0.0f, 60000.0f, 0.0f);
						Push(prefix + "P4", "DecShape", false, Linear, -10.0f, 10.0f, 0.0f);
						Push(prefix + "P5", "Sustain", false, Linear, 0.0f, 1.0f, 1.0f);
						Push(prefix + "P6", "Release", false, TimeMsExp, 0.0f, 60000.0f, 0.0f);
					}
					else
					{
						Push(prefix + "P1", "P1", false, Linear, 0.0f, 1.0f, 0.0f);
						Push(prefix + "P2", "P2", false, Linear, 0.0f, 1.0f, 0.0f);
						Push(prefix + "P3", "P3", false, Linear, 0.0f, 1.0f, 0.0f);
						Push(prefix + "P4", "P4", false, Linear, 0.0f, 1.0f, 0.0f);
						Push(prefix + "P5", "P5", false, Linear, 0.0f, 1.0f, 0.0f);
						Push(prefix + "P6", "P6", false, Linear, 0.0f, 1.0f, 0.0f);
					}
				};
			Push("masterVol", "Master", true, Linear, -60.0f, 12.0f, 0.0f);
			Push("octave", "Octave", true, Linear, -4.0f, 4.0f, 0.0f);
			const float maxWtPreset = static_cast<float>(WavetableGenerator::NumWavetablePresets - 1);
			for (int osc = 1; osc <= 2; ++osc)
			{
				const DirtynthParams::OscParams& oscParams =
					(osc == 1) ? paramsRef.osc1Params : paramsRef.osc2Params;
				const std::string prefix = DirtyTools::OldFormat("osc%d", osc);
				Push(prefix + "WtPreset", "WTPreset", false, Integer, 0.0f, maxWtPreset, 0.0f);
				Push(prefix + "WtPos", "WTPos", true, Linear, 0.0f, 1.0f, 0.0f);
				Push(prefix + "MutAType", "MutAType", false, Integer, 0.0f, static_cast<float>(RegMutant::NumRegMutant - 1), 0.0f);
				PushMutantDesc(osc, 1);
				Push(prefix + "MutBType", "MutBType", false, Integer, 0.0f, static_cast<float>(RegMutant::NumRegMutant - 1), 0.0f);
				PushMutantDesc(osc, 2);
				Push(prefix + "Pitch", "Pitch", true, Integer, -48.0f, 48.0f, 0.0f);
				Push(prefix + "Detune", "Detune", true, Linear, -100.0f, 100.0f, 0.0f);
			}
			Push("pmDepth", "PMDepth", true, Linear, 0.0f, 100.0f, 0.0f);
			Push("osc1gain", "Osc1Gain", true, Linear, 0.0f, 1.0f, 1.0f);
			Push("osc2gain", "Osc2Gain", true, Linear, 0.0f, 1.0f, 1.0f);
			PushFilterDesc(1, paramsRef.filt1Params);
			PushFilterDesc(2, paramsRef.filt2Params);
			Push("filt1gain", "Filt1Gain", true, Linear, 0.0f, 1.0f, 1.0f);
			Push("filt2gain", "Filt2Gain", true, Linear, 0.0f, 1.0f, 1.0f);
			Push("outputAmp", "OutputAmp", true, Linear, 0.0f, 1.0f, 1.0f);
			Push("sysTopo", "Topology", false, Integer, 0.0f, 3.0f, 0.0f);

			for (int i = 0; i < NumEnvelopes; ++i)
				PushEnvelopeDesc(i);

			return descs;
		}
		DirtynthParams GetModulatorAmountMultiplier(DirtynthParams param)
		{
			//根据当前的参数生成调制倍率。包络出来的范围是0-1，amount的范围是-1到1，需要映射成参数具体单位。
			//实际上就是最大值减去最小值，即Amount的倍数了。
			//返回的结构体装着各参数的调制倍数。若是不能被调制的参数，直接等于0。
			DirtynthParams multiplier;
			float* multiplierArray = reinterpret_cast<float*>(&multiplier);
			const int numParams = static_cast<int>(sizeof(DirtynthParams) / sizeof(float));
			for (int i = 0; i < numParams; ++i)
				multiplierArray[i] = 0.0f;
			std::vector<ParamDesc> descs = GetParamList(param);
			for (const ParamDesc& desc : descs)
			{
				if (!desc.isModulated)
					continue;
				const int id = SearchID(desc.indexName);
				if (id <= 0 || id >= static_cast<int>(idOffsetMap.size()))
					continue;
				GetParamRefByID(multiplier, id) = desc.maxValue - desc.minValue;
			}
			return multiplier;
		}
	};
}