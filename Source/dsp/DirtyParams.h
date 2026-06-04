#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

namespace Dirtynth
{
	constexpr static int NumEnvelopes = 6;
	constexpr static int NumEffects = 2;

	constexpr static int NumWavetablePresets = 1;
	constexpr static int NumMutantTypes = 4;
	constexpr static int NumFilterTypes = 7;
	constexpr static int NumEnvelopeTypes = 1;
	constexpr static int NumEnvelopeModes = 5;
	constexpr static int NumEffectTypes = 4;


	/*KnobFeel helper function*/
	//这些param应该都是0到1归一化的
	inline float Clamp01(float x)
	{
		if (x < 0.0f) return 0.0f;
		if (x > 1.0f) return 1.0f;
		return x;
	}
	inline float ParamToCutoff(float param, float cutoffMin = 20.0, float cutoffMax = 22000.0)
	{
		param = Clamp01(param);
		return cutoffMin * powf(cutoffMax / cutoffMin, param);
	}
	inline float ParamToReso(float param, float resoMin = 0.707, float resoMax = 40.0)
	{
		param = Clamp01(param);
		return resoMin * powf(resoMax / resoMin, param);
	}
	inline float CutoffToParam(float cutoff, float cutoffMin = 20.0, float cutoffMax = 22000.0)
	{
		if (cutoff < cutoffMin) cutoff = cutoffMin;
		if (cutoff > cutoffMax) cutoff = cutoffMax;
		return logf(cutoff / cutoffMin) / logf(cutoffMax / cutoffMin);
	}
	inline float ResoToParam(float reso, float resoMin = 0.707, float resoMax = 40.0)
	{
		if (reso < resoMin) reso = resoMin;
		if (reso > resoMax) reso = resoMax;
		return logf(reso / resoMin) / logf(resoMax / resoMin);
	}

	inline float ParamToEnveTime(float param, float enveExpShape = 4.0, float enveTimeMaxMs = 60000)
	{
		float x = expf((Clamp01(param) - 1.0) * enveExpShape);
		const static float expShape = expf(-enveExpShape);
		float y = (x - expShape) / (1.0f - expShape);
		return y * enveTimeMaxMs;
	}
	inline float EnveTimeToParam(float time, float enveExpShape = 4.0, float enveTimeMaxMs = 60000)
	{
		float y = Clamp01(time / enveTimeMaxMs);
		const static float expShape = expf(-enveExpShape);
		float x = y * (1.0f - expShape) + expShape;
		float param = 1.0f + logf(x) / enveExpShape;
		return Clamp01(param);
	}
	/*--------------*/

	struct DirtynthParams
	{
		float masterVol = 0.0f;
		float octave = 0.0f;

		struct OscParams
		{
			float/*int*/ oscWtPreset = 0.0f;
			float oscWtPos = 0.0f;
			struct MutantParams
			{
				float/*int*/ mutantType = 0.0f;
				float p1 = 0.0f;
				float p2 = 0.0f;
				float p3 = 0.0f;
			}mutantA, mutantB;
			float oscPitch = 0.0f;
			float oscDetune = 0.0f;
		}osc1Params, osc2Params;
		float pmDepth = 0.0f;
		float osc1gain = 1.0f;
		float osc2gain = 1.0f;

		struct FiltParams
		{
			float/*int*/ type = 0.0f;
			float cutoff = 22000.0f;
			float keyTrack = 0.0f;
			float reso = 0.707f;
			float morph = 0.0f;
		}filt1Params, filt2Params;
		float filt1gain = 1.0f;
		float filt2gain = 1.0f;
		float outputAmp = 1.0f;
		float/*int*/ sysTopo = 0.0f;

		struct EnveParams
		{
			float/*int*/ type = 0.0f;
			float/*int*/ mode = 0.0f;
			float/*int*/ targetID1 = 0.0f;
			float/*int*/ targetID2 = 0.0f;
			float amount1 = 0.0f;
			float amount2 = 0.0f;
			float p1 = 0.0f;
			float p2 = 0.0f;
			float p3 = 0.0f;
			float p4 = 0.0f;
			float p5 = 1.0f;
			float p6 = 0.0f;
		}enveParams[NumEnvelopes];

		struct EffectParams
		{
			float/*int*/ type = 0.0f;
			float p1 = 0.0f;
			float p2 = 0.0f;
			float p3 = 0.0f;
			float p4 = 0.0f;
			float mix = 0.0f;
		}effectParams[NumEffects];
	};

	struct DirtynthParamSystem
	{
		enum KnobFeelType
		{
			Linear = 0,
			Integer = 1,
			NyquistFreqExp = 2,
			FilterQ = 3,
			TimeMsExp = 4
		};

		struct ParamDesc
		{
			std::string indexName = "index name";
			std::string name = "display name";
			bool isModulated = false;
			KnobFeelType paramType = Linear;
			float minValue = 0.0f;
			float maxValue = 1.0f;
			float defValue = 0.0f;
		};

		DirtynthParams ref;
		inline static std::unordered_map<std::string, int> indexNameIdMap;
		inline static std::vector<std::size_t> idOffsetMap;
		inline static std::vector<ParamDesc> baseParamDescs;
		inline static int maxIndex = 0;
		inline static bool tableBuilt = false;

		static ParamDesc MakeDesc(const std::string& indexName, const std::string& name,
			bool isModulated, KnobFeelType paramType,
			float minValue, float maxValue, float defValue)
		{
			return { indexName, name, isModulated, paramType, minValue, maxValue, defValue };
		}

		static int ClampType(float value, int typeCount)
		{
			int type = static_cast<int>(value);
			if (type < 0) type = 0;
			if (type >= typeCount) type = typeCount - 1;
			return type;
		}

		void RegParam(float& paramRef, const ParamDesc& desc)
		{
			const std::size_t offset = reinterpret_cast<char*>(&paramRef) -
				reinterpret_cast<char*>(&ref);
			idOffsetMap.push_back(offset);
			indexNameIdMap[desc.indexName] = maxIndex;
			baseParamDescs.push_back(desc);
			++maxIndex;
		}

		void RebuildParamTable()
		{
			indexNameIdMap.clear();
			idOffsetMap.clear();
			baseParamDescs.clear();
			idOffsetMap.push_back(0);
			maxIndex = 1;

			RegParam(ref.masterVol, MakeDesc("masterVol", "Master", true, Linear, -60.0f, 12.0f, 0.0f));
			RegParam(ref.octave, MakeDesc("octave", "Octave", true, Integer, -4.0f, 4.0f, 0.0f));
			RegOscParams(ref.osc1Params, 1);
			RegOscParams(ref.osc2Params, 2);
			RegParam(ref.pmDepth, MakeDesc("pmDepth", "PMDepth", true, Linear, 0.0f, 100.0f, 0.0f));
			RegParam(ref.osc1gain, MakeDesc("osc1gain", "Osc1Gain", true, Linear, 0.0f, 1.0f, 1.0f));
			RegParam(ref.osc2gain, MakeDesc("osc2gain", "Osc2Gain", true, Linear, 0.0f, 1.0f, 1.0f));
			RegFilterParams(ref.filt1Params, 1);
			RegFilterParams(ref.filt2Params, 2);
			RegParam(ref.filt1gain, MakeDesc("filt1gain", "Filt1Gain", true, Linear, 0.0f, 1.0f, 1.0f));
			RegParam(ref.filt2gain, MakeDesc("filt2gain", "Filt2Gain", true, Linear, 0.0f, 1.0f, 1.0f));
			RegParam(ref.outputAmp, MakeDesc("outputAmp", "OutputAmp", true, Linear, 0.0f, 1.0f, 1.0f));
			RegParam(ref.sysTopo, MakeDesc("sysTopo", "Topology", false, Integer, 0.0f, 3.9f, 0.0f));
			for (int i = 0; i < NumEnvelopes; ++i)
				RegEnvelopeParams(ref.enveParams[i], i + 1);
			for (int i = 0; i < NumEffects; ++i)
				RegEffectParams(ref.effectParams[i], i + 1);
			tableBuilt = true;
		}

		void EnsureParamTableBuilt()
		{
			if (!tableBuilt)
				RebuildParamTable();
		}

		DirtynthParamSystem()
		{
			EnsureParamTableBuilt();
		}

		int SearchID(std::string name)
		{
			EnsureParamTableBuilt();
			const auto it = indexNameIdMap.find(name);
			return it == indexNameIdMap.end() ? 0 : it->second;
		}

		float& GetParamRefByID(DirtynthParams& paramsRef, int ID)
		{
			return *reinterpret_cast<float*>(
				reinterpret_cast<char*>(&paramsRef) + idOffsetMap[ID]);
		}

		std::vector<ParamDesc> GetParamList(const DirtynthParams& paramsRef)
		{
			EnsureParamTableBuilt();
			std::vector<ParamDesc> descs = baseParamDescs;

			PatchOscDescs(descs, paramsRef.osc1Params, 1);
			PatchOscDescs(descs, paramsRef.osc2Params, 2);
			PatchFilterDescs(descs, paramsRef.filt1Params, 1);
			PatchFilterDescs(descs, paramsRef.filt2Params, 2);
			for (int i = 0; i < NumEnvelopes; ++i)
				PatchEnvelopeDescs(descs, paramsRef.enveParams[i], i + 1);
			for (int i = 0; i < NumEffects; ++i)
				PatchEffectDescs(descs, paramsRef.effectParams[i], i + 1);
			return descs;
		}

		DirtynthParams GetModulatorAmountMultiplier(const DirtynthParams param)
		{
			EnsureParamTableBuilt();
			DirtynthParams multiplier;
			float* multiplierArray = reinterpret_cast<float*>(&multiplier);
			const int numParams = static_cast<int>(sizeof(DirtynthParams) / sizeof(float));
			for (int i = 0; i < numParams; ++i)
				multiplierArray[i] = 0.0f;

			const std::vector<ParamDesc> descs = GetParamList(param);
			const int numRegisteredParams = static_cast<int>(baseParamDescs.size());
			for (int id = 1; id <= numRegisteredParams; ++id)
			{
				const ParamDesc& desc = descs[id - 1];
				if (desc.isModulated)
					GetParamRefByID(multiplier, id) = desc.maxValue - desc.minValue;
			}
			return multiplier;
		}

	private:
		static std::string Prefix(const char* name, int index)
		{
			return std::string(name) + std::to_string(index);
		}
		void RegOscParams(DirtynthParams::OscParams& oscParams, int osc)
		{
			const std::string prefix = Prefix("osc", osc);
			const float maxWtPreset = static_cast<float>(NumWavetablePresets - 1);
			const float maxMutantType = static_cast<float>(NumMutantTypes - 1);
			RegParam(oscParams.oscWtPreset, MakeDesc(prefix + "WtPreset", "WTPreset", false, Integer, 0.0f, maxWtPreset, 0.0f));
			RegParam(oscParams.oscWtPos, MakeDesc(prefix + "WtPos", "WTPos", true, Linear, 0.0f, 1.0f, 0.0f));
			RegMutantParams(oscParams.mutantA, prefix + "MutA", maxMutantType);
			RegMutantParams(oscParams.mutantB, prefix + "MutB", maxMutantType);
			RegParam(oscParams.oscPitch, MakeDesc(prefix + "Pitch", "Pitch", true, Integer, -48.0f, 48.0f, 0.0f));
			RegParam(oscParams.oscDetune, MakeDesc(prefix + "Detune", "Detune", true, Linear, -100.0f, 100.0f, 0.0f));
		}
		void RegMutantParams(DirtynthParams::OscParams::MutantParams& mutantParams,
			const std::string& prefix, float maxMutantType)
		{
			RegParam(mutantParams.mutantType, MakeDesc(prefix + "Type", prefix + "Type", false, Integer, 0.0f, maxMutantType, 0.0f));
			RegParam(mutantParams.p1, MakeDesc(prefix + "P1", "P1", true, Linear, 0.0f, 1.0f, 0.0f));
			RegParam(mutantParams.p2, MakeDesc(prefix + "P2", "P2", true, Linear, 0.0f, 1.0f, 0.0f));
			RegParam(mutantParams.p3, MakeDesc(prefix + "P3", "P3", true, Linear, 0.0f, 1.0f, 0.0f));
		}
		void RegFilterParams(DirtynthParams::FiltParams& filtParams, int filt)
		{
			const std::string prefix = Prefix("filt", filt);
			RegParam(filtParams.type, MakeDesc(prefix + "Type", "Type", false, Integer, 0.0f, static_cast<float>(NumFilterTypes - 1), 0.0f));
			RegParam(filtParams.cutoff, MakeDesc(prefix + "Cutoff", "Cutoff", true, NyquistFreqExp, 20.0f, 22000.0f, 22000.0f));
			RegParam(filtParams.keyTrack, MakeDesc(prefix + "KeyTrack", "KeyTrack", true, Linear, 0.0f, 1.0f, 0.0f));
			RegParam(filtParams.reso, MakeDesc(prefix + "Reso", "Reso", true, FilterQ, 0.707f, 40.0f, 0.707f));
			RegParam(filtParams.morph, MakeDesc(prefix + "Morph", "Morph", true, Linear, 0.0f, 1.0f, 0.0f));
		}
		void RegEnvelopeParams(DirtynthParams::EnveParams& enveParams, int enve)
		{
			const std::string prefix = Prefix("enve", enve);
			RegParam(enveParams.type, MakeDesc(prefix + "Type", "Type", false, Integer, 0.0f, static_cast<float>(NumEnvelopeTypes - 1), 0.0f));
			RegParam(enveParams.mode, MakeDesc(prefix + "Mode", "Mode", false, Integer, 0.0f, static_cast<float>(NumEnvelopeModes - 1), 0.0f));
			RegParam(enveParams.targetID1, MakeDesc(prefix + "TargetID1", "Target1", false, Integer, 0.0f, 0.0f, 0.0f));
			RegParam(enveParams.targetID2, MakeDesc(prefix + "TargetID2", "Target2", false, Integer, 0.0f, 0.0f, 0.0f));
			RegParam(enveParams.amount1, MakeDesc(prefix + "Amount1", "Amount1", false, Linear, -1.0f, 1.0f, 0.0f));
			RegParam(enveParams.amount2, MakeDesc(prefix + "Amount2", "Amount2", false, Linear, -1.0f, 1.0f, 0.0f));
			RegParam(enveParams.p1, MakeDesc(prefix + "P1", "P1", false, Linear, 0.0f, 1.0f, 0.0f));
			RegParam(enveParams.p2, MakeDesc(prefix + "P2", "P2", false, Linear, 0.0f, 1.0f, 0.0f));
			RegParam(enveParams.p3, MakeDesc(prefix + "P3", "P3", false, Linear, 0.0f, 1.0f, 0.0f));
			RegParam(enveParams.p4, MakeDesc(prefix + "P4", "P4", false, Linear, 0.0f, 1.0f, 0.0f));
			RegParam(enveParams.p5, MakeDesc(prefix + "P5", "P5", false, Linear, 0.0f, 1.0f, 1.0f));
			RegParam(enveParams.p6, MakeDesc(prefix + "P6", "P6", false, Linear, 0.0f, 1.0f, 0.0f));
		}
		void RegEffectParams(DirtynthParams::EffectParams& effectParams, int effect)
		{
			const std::string prefix = Prefix("effect", effect);
			RegParam(effectParams.type, MakeDesc(prefix + "Type", "Type", false, Integer, 0.0f, static_cast<float>(NumEffectTypes - 1), 0.0f));
			RegParam(effectParams.p1, MakeDesc(prefix + "P1", "P1", false, Linear, 0.0f, 1.0f, 0.0f));
			RegParam(effectParams.p2, MakeDesc(prefix + "P2", "P2", false, Linear, 0.0f, 1.0f, 0.0f));
			RegParam(effectParams.p3, MakeDesc(prefix + "P3", "P3", false, Linear, 0.0f, 1.0f, 0.0f));
			RegParam(effectParams.p4, MakeDesc(prefix + "P4", "P4", false, Linear, 0.0f, 1.0f, 0.0f));
			RegParam(effectParams.mix, MakeDesc(prefix + "Mix", "Mix", false, Linear, 0.0f, 1.0f, 0.0f));
		}

		void PatchDesc(std::vector<ParamDesc>& descs, const std::string& indexName,
			const std::string& name, bool isModulated, KnobFeelType paramType,
			float minValue, float maxValue, float defValue)
		{
			const int id = SearchID(indexName);
			if (id <= 0)
				return;

			ParamDesc& desc = descs[id - 1];
			desc.name = name;
			desc.isModulated = isModulated;
			desc.paramType = paramType;
			desc.minValue = minValue;
			desc.maxValue = maxValue;
			desc.defValue = defValue;
		}

		void PatchOscDescs(std::vector<ParamDesc>& descs, const DirtynthParams::OscParams& oscParams, int osc)
		{
			const std::string prefix = Prefix("osc", osc);
			PatchMutantDescs(descs, prefix + "MutA", oscParams.mutantA);
			PatchMutantDescs(descs, prefix + "MutB", oscParams.mutantB);
		}

		void PatchMutantDescs(std::vector<ParamDesc>& descs,
			const std::string& prefix, const DirtynthParams::OscParams::MutantParams& mutantParams)
		{
			const int mutantType = ClampType(mutantParams.mutantType, NumMutantTypes);
			switch (mutantType)
			{
			case 0:
				PatchDesc(descs, prefix + "P1", "Depth", true, Linear, 0.0f, 1.0f, 0.0f);
				PatchDesc(descs, prefix + "P2", "Phase", true, Linear, 0.0f, 1.0f, 0.0f);
				PatchDesc(descs, prefix + "P3", "Smooth", true, Linear, 0.0f, 1.0f, 0.0f);
				break;
			case 1:
				PatchDesc(descs, prefix + "P1", "Depth", true, Linear, 0.0f, 1.0f, 0.0f);
				PatchDesc(descs, prefix + "P2", "PreLP", true, Linear, 0.0f, 1.0f, 0.0f);
				PatchDesc(descs, prefix + "P3", "Stages", true, Linear, 0.0f, 1.0f, 0.0f);
				break;
			case 2:
				PatchDesc(descs, prefix + "P1", "Depth", true, Linear, 0.0f, 1.0f, 0.0f);
				PatchDesc(descs, prefix + "P2", "Tmix", true, Linear, 0.0f, 1.0f, 0.0f);
				PatchDesc(descs, prefix + "P3", "Rate", true, Linear, 0.0f, 1.0f, 0.0f);
				break;
			case 3:
				PatchDesc(descs, prefix + "P1", "Disperse", true, Linear, 0.0f, 1.0f, 0.0f);
				PatchDesc(descs, prefix + "P2", "Harmonic", true, Linear, 0.0f, 1.0f, 0.0f);
				PatchDesc(descs, prefix + "P3", "Comb", true, Linear, 0.0f, 1.0f, 0.0f);
				break;
			default:
				break;
			}
		}

		static const char* GetFilterMorphName(int filterType)
		{
			switch (filterType)
			{
			case 0:
			case 1:
			case 2:
				return "LP-BP-HP";
			case 3:
				return "AddSub";
			case 4:
				return "Spread";
			case 5:
				return "Disperse";
			case 6:
				return "Stages";
			default:
				return "Morph";
			}
		}

		void PatchFilterDescs(std::vector<ParamDesc>& descs, const DirtynthParams::FiltParams& filtParams, int filt)
		{
			const std::string prefix = Prefix("filt", filt);
			const int filterType = ClampType(filtParams.type, NumFilterTypes);
			PatchDesc(descs, prefix + "Morph", GetFilterMorphName(filterType), true, Linear, 0.0f, 1.0f, 0.0f);
		}

		void PatchEnvelopeDescs(std::vector<ParamDesc>& descs, const DirtynthParams::EnveParams& enveParams, int enve)
		{
			const std::string prefix = Prefix("enve", enve);
			const int envelopeType = ClampType(enveParams.type, NumEnvelopeTypes);
			const float maxTargetID = static_cast<float>(maxIndex - 1);

			PatchDesc(descs, prefix + "TargetID1", "Target1", false, Integer, 0.0f, maxTargetID, 0.0f);
			PatchDesc(descs, prefix + "TargetID2", "Target2", false, Integer, 0.0f, maxTargetID, 0.0f);

			if (envelopeType == 0)
			{
				PatchDesc(descs, prefix + "P1", "Attack", false, TimeMsExp, 0.0f, 60000.0f, 0.0f);
				PatchDesc(descs, prefix + "P2", "AttShape", false, Linear, -16.0f, 16.0f, 0.0f);
				PatchDesc(descs, prefix + "P3", "Decay", false, TimeMsExp, 0.0f, 60000.0f, 0.0f);
				PatchDesc(descs, prefix + "P4", "DecShape", false, Linear, -16.0f, 16.0f, 0.0f);
				PatchDesc(descs, prefix + "P5", "Sustain", false, Linear, 0.0f, 1.0f, 1.0f);
				PatchDesc(descs, prefix + "P6", "Release", false, TimeMsExp, 0.0f, 60000.0f, 0.0f);
			}
			else if (envelopeType == 1)//待补充类型
			{

			}
		}

		void PatchEffectDescs(std::vector<ParamDesc>& descs, const DirtynthParams::EffectParams& effectParams, int effect)
		{
			const std::string prefix = Prefix("effect", effect);
			const int effectType = ClampType(effectParams.type, NumEffectTypes);

			switch (effectType)
			{
			default:
				PatchDesc(descs, prefix + "P1", "P1", true, Linear, 0.0f, 1.0f, 0.0f);
				PatchDesc(descs, prefix + "P2", "P2", true, Linear, 0.0f, 1.0f, 0.0f);
				PatchDesc(descs, prefix + "P3", "P3", true, Linear, 0.0f, 1.0f, 0.0f);
				PatchDesc(descs, prefix + "P4", "P4", true, Linear, 0.0f, 1.0f, 0.0f);
				PatchDesc(descs, prefix + "Mix", "Mix", true, Linear, 0.0f, 1.0f, 0.0f);
				break;
			}
		}
	};
}