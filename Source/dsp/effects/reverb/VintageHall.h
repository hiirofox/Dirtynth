#pragma once

#include <math.h>

namespace LatticeReverb3
{
	/*
	template<int MaxDelayLen>
	class DelayLine
	{
	private:
		float dat[MaxDelayLen] = { 0 };
		float out = 0;

		// 平滑处理
		float currentDelay = 0;
		float targetDelay = 2;
		float delayVelocity = 0; // 用于线性平滑延迟时间本身

		int pos = 0;

		int updateTime = 0;

		inline float ReadSampleHermite(float delay)
		{
			float readPos = (float)pos - delay;
			//while (readPos < 0) readPos += MaxDelayLen;
			//while (readPos >= MaxDelayLen) readPos -= MaxDelayLen;
			if (readPos < 0) readPos += MaxDelayLen;
			if (readPos >= MaxDelayLen) readPos -= MaxDelayLen;

			int i1 = (int)readPos;
			float f = readPos - i1;

			int i0 = (i1 - 1 + MaxDelayLen) % MaxDelayLen;
			int i2 = (i1 + 1) % MaxDelayLen;
			int i3 = (i1 + 2) % MaxDelayLen;

			float y0 = dat[i0];
			float y1 = dat[i1];
			float y2 = dat[i2];
			float y3 = dat[i3];

			float c0 = y1;
			float c1 = 0.5f * (y2 - y0);
			float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
			float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);

			return ((c3 * f + c2) * f + c1) * f + c0;
		}
		inline float ReadSampleLinear(float delay)
		{
			float readPos = (float)pos - delay;
			//while (readPos < 0) readPos += MaxDelayLen;
			//while (readPos >= MaxDelayLen) readPos -= MaxDelayLen;
			if (readPos < 0) readPos += MaxDelayLen;
			if (readPos >= MaxDelayLen) readPos -= MaxDelayLen;
			int i1 = (int)readPos;
			int i2 = (i1 + 1) % MaxDelayLen;
			float f = readPos - i1;
			return dat[i1] * (1.0f - f) + dat[i2] * f;
		}
		inline float ReadSampleDirect(float delay)
		{
			float readPos = (float)pos - delay;
			if (readPos < 0) readPos += MaxDelayLen;
			if (readPos >= MaxDelayLen) readPos -= MaxDelayLen;
			int i1 = (int)readPos;
			return dat[i1];
		}
	public:
		constexpr static int GradientSamples = 500;

		DelayLine()
		{
			std::fill(dat, dat + MaxDelayLen, 0.0f);
		}

		inline void SetDelayTime(float t)
		{
			if (t < 2)t = 2;
			if (t > MaxDelayLen - 4)t = MaxDelayLen - 4;
			targetDelay = t;
		}

		inline float ReadSample()
		{
			return out;
		}

		inline void WriteSample(float val)
		{
			dat[pos] = val;

			updateTime--;
			if (updateTime <= 0)
			{
				delayVelocity = (targetDelay - currentDelay) / (float)GradientSamples;
				updateTime = 512;
			}

			if (fabsf(targetDelay - currentDelay) > 0.0001f)
			{
				currentDelay += delayVelocity;
			}
			else
			{
				currentDelay = targetDelay;
			}

			//out = ReadSampleHermite(currentDelay);
			//out = ReadSampleLinear(currentDelay);
			out = ReadSampleDirect(currentDelay);

			if (++pos >= MaxDelayLen) pos = 0;
		}
		void Reset()
		{
			for (auto& v : dat)v = 0;
			out = 0;
			pos = 0;
			currentDelay = targetDelay;
		}
	};
	*/

	template<int MaxDelayLen>
	class DelayLine
	{
	private:
		float dat[MaxDelayLen] = { 0 };
		float out = 0;
		int time1 = 0, time2 = 0, updateTime = 0;
		float gradient = 0;
		float dt = 1.0 / 25.0;
		int pos = 0;
	public:
		DelayLine()
		{
		}
		void SetGradientRate(float rate)
		{
			dt = rate;
		}
		inline void SetDelayTime(float t)
		{
			updateTime = t;
		}
		int GetMaxDelay()
		{
			return MaxDelayLen;
		}
		inline float ReadSample()
		{
			return out;
		}

		inline void WriteSample(float val)
		{
			dat[pos] = val;

			int index1 = (MaxDelayLen + pos - time1) % MaxDelayLen;
			int index2 = (MaxDelayLen + pos - time2) % MaxDelayLen;
			out = dat[index1] + (dat[index2] - dat[index1]) * gradient;
			gradient += dt;
			if (gradient >= 1.0)
			{
				gradient -= 1.0;
				time1 = time2;
				time2 = updateTime;
				//updateTime = delay;
			}

			pos = (pos + 1) % MaxDelayLen;
		}
		void Reset()
		{
			pos = 0;
			gradient = 0;
			time1 = time2 = updateTime;
			for (int i = 0; i < MaxDelayLen; ++i) dat[i] = 0;
		}
	};

	class LatticeCascade
	{
	public:
		constexpr static int NumLayers = 6;
		constexpr static int MaxDelayLength = 4800 + 100;

	private:
		float roomSize = 512;

		DelayLine<MaxDelayLength> delays[NumLayers];
		float ks[NumLayers];
		float ds[NumLayers];
		float outks[NumLayers];

		float tapmix = 0;

		template<int inLayer>//NumLayers-1
		inline float HProcSamp(float x)
		{
			if constexpr (inLayer >= 0)
			{
				if constexpr (inLayer == NumLayers - 1) tapmix = 0;
				float y = HProcSamp<inLayer - 1>(delays[inLayer].ReadSample());
				float a = (x + ks[inLayer] * y) * ds[inLayer];
				delays[inLayer].WriteSample(a);
				float out = y - a * ks[inLayer];
				tapmix += out * outks[inLayer];
				return out;
			}
			else
			{
				return x;
			}
		}
		/*
		inline float HProcSamp(float x)//如果重写成cl版本，使用这个无递归模板的分支
		{
			if constexpr (inLayer >= 0)
			{
				if constexpr (inLayer == NumLayers - 1) tapmix = 0.0f;
				float xs[inLayer + 1];
				float cur = x;
				for (int i = inLayer; i >= 0; --i)
				{
					xs[i] = cur;
					cur = delays[i].ReadSample();
				}
				float y = cur;
				for (int i = 0; i <= inLayer; ++i)
				{
					float a = (xs[i] + ks[i] * y) * ds[i];
					delays[i].WriteSample(a);
					float out = y - a * ks[i];
					tapmix += out * outks[i];
					y = out;
				}
				return y;
			}
			else return x;
		}*/
	public:
		inline float ProcessSample(float x)// direct
		{
			return HProcSamp<NumLayers - 1>(x);
		}
		inline float GetTapMix()
		{
			return tapmix;
		}

		void Reset()
		{
			for (auto& it : delays)it.Reset();
		}
		void SetKs(float* ks)
		{
			for (int i = 0; i < NumLayers; ++i)this->ks[i] = ks[i];
		}
		void SetDs(float* ds)
		{
			for (int i = 0; i < NumLayers; ++i)this->ds[i] = ds[i];
		}
		void SetDelaysLength(float* delaysLength)//归一化最大长度
		{
			float maxv = 0;
			for (int i = 0; i < NumLayers; ++i)
			{
				if (delaysLength[i] > maxv)maxv = delaysLength[i];
			}
			for (int i = 0; i < NumLayers; ++i)
			{
				float delayt = delaysLength[i] / maxv * roomSize;
				delays[i].SetDelayTime(delayt);
			}
		}
		void SetOutKs(float* outKs)//归一化能量
		{
			float energy = 0;
			for (int i = 0; i < NumLayers; ++i)
			{
				energy += outKs[i] * outKs[i];
			}
			energy = sqrtf(energy) * sqrtf(NumLayers);
			for (int i = 0; i < NumLayers; ++i)
			{
				float outk = outKs[i] / energy;
				outks[i] = outk;
			}
		}
		void SetRoomSize(float roomSize)
		{
			this->roomSize = roomSize;
		}
	};

	class LatticeReverb3
	{
	public:
		constexpr static int NumLayers = LatticeCascade::NumLayers;
		constexpr static int NumRoomParams = 8 * LatticeCascade::NumLayers + 4;
		constexpr static int MaxPreDelaySize = 8192;
	private:
		float sampleRate = 48000.0;
		LatticeCascade earlyl, earlyr;
		LatticeCascade latel, later;
		DelayLine<LatticeCascade::MaxDelayLength> earlyCrossDelayL, earlyCrossDelayR;
		DelayLine<LatticeCascade::MaxDelayLength> crossDelayL, crossDelayR;
		DelayLine<MaxPreDelaySize> preDelayL, preDelayR;

		struct RoomParams
		{
			float tsl[LatticeCascade::NumLayers];//延迟线长度比例
			float tsr[LatticeCascade::NumLayers];
			float fbtl;
			float fbtr;
			//反射系数受diffusion约束
			float ksl[LatticeCascade::NumLayers];//各节反射系数
			float ksr[LatticeCascade::NumLayers];
			//衰减系数受decayTime约束
			float dsl[LatticeCascade::NumLayers];//各节衰减系数
			float dsr[LatticeCascade::NumLayers];
			float fbdl;//莫比乌斯环结构的反馈衰减系数
			float fbdr;

			float outksl[LatticeCascade::NumLayers];//各节tap输出混合比例
			float outksr[LatticeCascade::NumLayers];//各节tap输出混合比例
		};
		struct ReverbParams
		{
			float roomSize = 1200;//roomsize在这里改！！！
			float decayTime = 0.9999;//这个需要算RT60补偿
			float diffusion = 1.0;
			float mixAttack = 0.2;
		};
		RoomParams roomParams, applyRoomParams;
		ReverbParams reverbParams;

		float drywetmix = 0;
		float wide = 1.0;
		float mdepth = 0, mrate = 0;
		float rndvl[NumLayers];
		float rndvr[NumLayers];
		float rndtl[NumLayers];
		float rndtr[NumLayers];
		void ApplyRoomSize()
		{
			applyRoomParams.fbtl = roomParams.fbtl * reverbParams.roomSize;
			applyRoomParams.fbtr = roomParams.fbtr * reverbParams.roomSize;
			float earlySize = reverbParams.roomSize * 0.25;
			if (earlySize < 950) earlySize = 950;
			earlyl.SetRoomSize(earlySize);
			earlyr.SetRoomSize(earlySize);
			latel.SetRoomSize(reverbParams.roomSize);
			later.SetRoomSize(reverbParams.roomSize);
			for (int i = 0; i < LatticeCascade::NumLayers; ++i)
			{
				float tml = 1.0 + cosf(rndtl[i] * 2.0 * 3.1415926535) * mdepth;
				float tmr = 1.0 + cosf(rndtr[i] * 2.0 * 3.1415926535) * mdepth;
				rndtl[i] += rndvl[i] * mrate;
				rndtr[i] += rndvr[i] * mrate;
				if (rndtl[i] >= 1.0)rndtl[i] -= 1.0;
				if (rndtr[i] >= 1.0)rndtr[i] -= 1.0;
				applyRoomParams.tsl[i] = roomParams.tsl[i] * tml;
				applyRoomParams.tsr[i] = roomParams.tsr[i] * tmr;
			}
			earlyl.SetDelaysLength(applyRoomParams.tsl);
			earlyr.SetDelaysLength(applyRoomParams.tsr);
			latel.SetDelaysLength(applyRoomParams.tsl);
			later.SetDelaysLength(applyRoomParams.tsr);
			float earlyCrossDelaySize = roomParams.fbtl * earlySize;
			earlyCrossDelayL.SetDelayTime(earlyCrossDelaySize);
			earlyCrossDelayR.SetDelayTime(earlyCrossDelaySize);
			crossDelayL.SetDelayTime(applyRoomParams.fbtl);
			crossDelayR.SetDelayTime(applyRoomParams.fbtr);
		}


		float outlpfv = 1.0, outhpfv = 0.0;
		float outlpl = 0, outlpr = 0;
		float outhpl = 0, outhpr = 0;

		float outlsfv = 0.0, outlsgv = 1.0;
		float outlsl = 0, outlsr = 0;
		float damphsfv = 1.0, damphsgv = 1.0;
		float damphsl = 0, damphsr = 0;
		inline float OnePoleCoeffExact(float fc)
		{
			float w = 2.0f * 3.14159265358979 * fc / sampleRate;
			float q = 1.0f - cosf(w);
			return sqrtf(q * q + 2.0f * q) - q;
		}
		static inline float DBToShelfGain(float gainDb)
		{
			return powf(10.0f, gainDb / 20.0f);
		}
	public:
		LatticeReverb3()
		{
			for (int i = 0; i < NumLayers; ++i)
			{
				rndvl[i] = (float)(rand() % 10000) / 10000 * 0.25 + 0.75;
				rndvr[i] = (float)(rand() % 10000) / 10000 * 0.25 + 0.75;
				rndtl[i] = (float)(rand() % 10000) / 10000 * 0.5 + 0.5;
				rndtr[i] = (float)(rand() % 10000) / 10000 * 0.5 + 0.5;
			}

			std::vector<float> roomParams4 = {
			0.0600735247, 0.18174772, 0.33665067, 0.532276452, 0.56940639, 0.999920249,
			0.0466615595, 0.180975243, 0.324151039, 0.472937137, 0.717006147, 0.998368621,
			-0.989957511, 0.810808301, 0.486398011, 0.629302979, -0.319459885, -0.307317376,
			0.982350707, -0.56488359, 0.669427097, 0.369275957, 0.309160382, -0.271651536,
			//0.999726474, 0.999361992, 0.999999285, 0.999973476, 0.999033868, 0.999996662,
			//0.999987483, 0.999970555, 0.999761283, 0.999839008, 0.999948144, 0.999964952,
			1,1,1,1,1,1,
			1,1,1,1,1,1,

			0.230478734, 0.542093515, 0.979074717, 0.999910295, 0.882425249, 0.918131709,
			0.799034178, 0.0295727383, 0.996820807, 0.942539632, 0.903606117, 0.803280115,
			0.783993363, 0.76824981, 0.999421716, 0.794228554
			};
			SetupRoomCharacteristics(roomParams4);
		}
		void Reset()
		{
			earlyl.Reset();
			earlyr.Reset();
			latel.Reset();
			later.Reset();
			earlyCrossDelayL.Reset();
			earlyCrossDelayR.Reset();
			crossDelayL.Reset();
			crossDelayR.Reset();
			preDelayL.Reset();
			preDelayR.Reset();
		}

		void ProcessBlock(const float* inl, const float* inr, float* outl, float* outr, int numSamples)
		{
			ApplyRoomSize();

			constexpr float kInvSqrt2 = 0.70710678118f;
			constexpr float kHalfPi = 1.57079632679f;
			const float fbdl = applyRoomParams.fbdl;
			const float fbdr = applyRoomParams.fbdr;
			const float attackA = cosf(reverbParams.mixAttack * kHalfPi);
			const float attackB = sinf(reverbParams.mixAttack * kHalfPi);
			const float midGain = cosf(wide * kHalfPi);
			const float wideGain = sinf(wide * kHalfPi);
			const float dryGain = cosf(drywetmix * kHalfPi);
			const float wetGain = sinf(drywetmix * kHalfPi);
			for (int i = 0; i < numSamples; ++i)
			{
				///////////////// EARLY
				const float lastEarlyOutl = earlyCrossDelayL.ReadSample();
				const float lastEarlyOutr = earlyCrossDelayR.ReadSample();
				float earlyOutvl = earlyl.ProcessSample(inl[i] + lastEarlyOutl * fbdl);
				float earlyOutvr = earlyr.ProcessSample(inr[i] + lastEarlyOutr * fbdr);
				earlyCrossDelayL.WriteSample(earlyOutvr); // cross
				earlyCrossDelayR.WriteSample(earlyOutvl);
				float earlybufl = earlyOutvl;
				float earlybufr = earlyOutvr;
				//float earlybufl = earlyl.GetTapMix();
				//float earlybufr = earlyr.GetTapMix();

				///////////////// LATE
				//float inputWithAttackL = inl[i] * attackA + earlybufl * attackB;//attack越大越缓，所以将early直送入late让能量上升更缓
				//float inputWithAttackR = inr[i] * attackA + earlybufr * attackB;
				//float inputWithAttackL = inl[i] * attackB;//method b
				//float inputWithAttackR = inr[i] * attackB;
				//float inputWithAttackL = earlybufl * attackB;//method c
				//float inputWithAttackR = earlybufr * attackB;
				float inputWithAttackL = earlybufl * (attackB * 0.5 + 0.5);//method d
				float inputWithAttackR = earlybufr * (attackB * 0.5 + 0.5);

				const float lastoutl = crossDelayL.ReadSample();
				const float lastoutr = crossDelayR.ReadSample();
				const float xl = inputWithAttackL + lastoutl * fbdl;
				const float xr = inputWithAttackR + lastoutr * fbdr;
				float outvl = latel.ProcessSample(xl);
				float outvr = later.ProcessSample(xr);

				//damping high shelf
				damphsl += damphsfv * (outvl - damphsl);//lp
				damphsr += damphsfv * (outvr - damphsr);
				float dvl = damphsl + (outvl - damphsl) * damphsgv;//hs
				float dvr = damphsr + (outvr - damphsr) * damphsgv;
				//inject crossfb delayline
				crossDelayL.WriteSample(dvr); // cross
				crossDelayR.WriteSample(dvl);

				outvl = latel.GetTapMix();
				outvr = later.GetTapMix();
				const float mid = (outvl + outvr) * kInvSqrt2;
				outvl = mid * midGain + outvl * wideGain;
				outvr = mid * midGain + outvr * wideGain;

				//early attackA
				float attackOutL = earlybufl * (attackA * 0.5);//attack越小越抖，这时候early直接输出补齐音头能量
				float attackOutR = earlybufr * (attackA * 0.5);

				//out eq
				outvl += attackOutL;// mix direct early 
				outvr += attackOutR;
				outlpl += outlpfv * (outvl - outlpl);//lp
				outlpr += outlpfv * (outvr - outlpr);
				outhpl += outhpfv * (outlpl - outhpl);//lp
				outhpr += outhpfv * (outlpr - outhpr);
				float eqoutl1 = outlpl - outhpl;//lp->hp
				float eqoutr1 = outlpr - outhpr;
				outlsl += outlsfv * (eqoutl1 - outlsl);//lp
				outlsr += outlsfv * (eqoutr1 - outlsr);
				float eqoutl = outlsl * outlsgv + (eqoutl1 - outlsl);//lp->hp->ls
				float eqoutr = outlsr * outlsgv + (eqoutr1 - outlsr);

				//drywet mix
				float dryl = inl[i] * dryGain;
				float dryr = inr[i] * dryGain;
				float wetl = eqoutl * wetGain * 2.0;//!注意这里补偿wet音量。不一定对所有听众适用
				float wetr = eqoutr * wetGain * 2.0;
				preDelayL.WriteSample(wetl);
				preDelayR.WriteSample(wetr);
				outl[i] = dryl + preDelayL.ReadSample();
				outr[i] = dryr + preDelayR.ReadSample();
			}
		}
		void SetSampleRate(float sr) { sampleRate = sr; }
		void SetPreDelay(float ms)
		{
			int pds = ms / 1000.0 * sampleRate;
			if (pds > MaxPreDelaySize - 10)pds = MaxPreDelaySize - 10;
			preDelayL.SetDelayTime(pds);
			preDelayR.SetDelayTime(pds);
		}
		void SetDampEQ(float damphighfreq, float damphighshelf, float bassfreq, float bassmulti, float eqhighcut, float eqlowcut)
		{
			damphsfv = OnePoleCoeffExact(damphighfreq);
			damphsgv = DBToShelfGain(damphighshelf);
			outlpfv = OnePoleCoeffExact(eqhighcut);//hz
			outhpfv = OnePoleCoeffExact(eqlowcut);
			outlsfv = OnePoleCoeffExact(bassfreq);
			outlsgv = bassmulti;
		}
		void SetModDepthRate(float mdepth, float mrate)
		{
			this->mdepth = mdepth * 0.01;
			this->mrate = mrate * 200.0 / 48000.0;
		}
		void SetWide(float wide)
		{
			this->wide = wide;
		}
		void SetRoomSize(float roomSize)
		{
			reverbParams.roomSize = roomSize;
		}
		void SetDecayTime(float decayTime)//n代表n秒之后到达-60dB
		{
			//reverbParams.decayTime = decayTime;
			//reverbParams.decayTime = (1.0 - expf(-decayTime * 6.0)) / (1.0 - expf(-6.0));
			reverbParams.decayTime = powf(0.001f, (reverbParams.roomSize / (float)LatticeCascade::MaxDelayLength) / (decayTime * 20.0f * 5.0f));
			if (decayTime > 0.75)  reverbParams.decayTime = grad01(reverbParams.decayTime, 1.0, (decayTime - 0.75) / 0.25);

			for (int i = 0; i < LatticeCascade::NumLayers; ++i)
			{
				applyRoomParams.dsl[i] = roomParams.dsl[i] * reverbParams.decayTime;
				applyRoomParams.dsr[i] = roomParams.dsr[i] * reverbParams.decayTime;
			}
			applyRoomParams.fbdl = roomParams.fbdl * reverbParams.decayTime;
			applyRoomParams.fbdr = roomParams.fbdr * reverbParams.decayTime;
			earlyl.SetDs(applyRoomParams.dsl);
			earlyr.SetDs(applyRoomParams.dsr);
			latel.SetDs(applyRoomParams.dsl);
			later.SetDs(applyRoomParams.dsr);
		}
		float grad01(float l, float r, float x)
		{
			return (r - l) * x + l;
		}
		float satuk(float origk, float diffusion, float maxk = 0.75)
		{
			if (diffusion <= 0.5) return origk * diffusion * 2.0;
			return grad01(origk, maxk, diffusion * 2.0 - 1.0);
		}
		void SetDiffusion(float diffusion)//0-1
		{
			reverbParams.diffusion = diffusion;
			for (int i = 0; i < LatticeCascade::NumLayers; ++i)
			{
				//applyRoomParams.ksl[i] = roomParams.ksl[i] * diffusion;
				//applyRoomParams.ksr[i] = roomParams.ksr[i] * diffusion;
				applyRoomParams.ksl[i] = satuk(roomParams.ksl[i], diffusion);
				applyRoomParams.ksr[i] = satuk(roomParams.ksr[i], diffusion);
			}
			earlyl.SetKs(applyRoomParams.ksl);
			earlyr.SetKs(applyRoomParams.ksr);
			latel.SetKs(applyRoomParams.ksl);
			later.SetKs(applyRoomParams.ksr);
		}
		void SetDiffusionEarlyLate(float diffusionEarly, float diffusionLate)
		{
			reverbParams.diffusion = 1.0f; // 这里不再用统一diffusion

			int idx[LatticeCascade::NumLayers];

			// ===== Left channel =====
			for (int i = 0; i < LatticeCascade::NumLayers; ++i)
				idx[i] = i;

			// 按 tsl 从小到大排序索引
			std::sort(idx, idx + LatticeCascade::NumLayers,
				[&](int a, int b)
				{
					return roomParams.tsl[a] < roomParams.tsl[b];
				});

			// 前3小 = early，后3大 = late
			for (int i = 0; i < LatticeCascade::NumLayers; ++i)
			{
				int id = idx[i];
				float factor = (i < 3) ? diffusionEarly : diffusionLate;
				//applyRoomParams.ksl[id] = roomParams.ksl[id] * factor;
				applyRoomParams.ksl[id] = satuk(roomParams.ksl[id], factor);
			}

			// ===== Right channel =====
			for (int i = 0; i < LatticeCascade::NumLayers; ++i)
				idx[i] = i;

			std::sort(idx, idx + LatticeCascade::NumLayers,
				[&](int a, int b)
				{
					return roomParams.tsr[a] < roomParams.tsr[b];
				});

			for (int i = 0; i < LatticeCascade::NumLayers; ++i)
			{
				int id = idx[i];
				float factor = (i < 3) ? diffusionEarly : diffusionLate;
				//applyRoomParams.ksr[id] = roomParams.ksr[id] * factor;
				applyRoomParams.ksr[id] = satuk(roomParams.ksr[id], factor);
			}
			earlyl.SetKs(applyRoomParams.ksl);
			earlyr.SetKs(applyRoomParams.ksr);
			latel.SetKs(applyRoomParams.ksl);
			later.SetKs(applyRoomParams.ksr);
		}
		void SetAttack(float attack)//0-1
		{
			reverbParams.mixAttack = attack;
		}
		void SetMixDryWet(float mix)
		{
			drywetmix = mix;
		}
		void SetupRoomCharacteristics(std::vector<float>& roomParamsPack)
		{
			for (int i = 0; i < LatticeCascade::NumLayers; ++i)
			{
				roomParams.tsl[i] = roomParamsPack[i + 0 * LatticeCascade::NumLayers];
				roomParams.tsr[i] = roomParamsPack[i + 1 * LatticeCascade::NumLayers];
				roomParams.ksl[i] = roomParamsPack[i + 2 * LatticeCascade::NumLayers];
				roomParams.ksr[i] = roomParamsPack[i + 3 * LatticeCascade::NumLayers];
				roomParams.dsl[i] = roomParamsPack[i + 4 * LatticeCascade::NumLayers];
				roomParams.dsr[i] = roomParamsPack[i + 5 * LatticeCascade::NumLayers];
				roomParams.outksl[i] = roomParamsPack[i + 6 * LatticeCascade::NumLayers];
				roomParams.outksr[i] = roomParamsPack[i + 7 * LatticeCascade::NumLayers];
			}
			roomParams.fbdl = roomParamsPack[8 * LatticeCascade::NumLayers + 0];
			roomParams.fbdr = roomParamsPack[8 * LatticeCascade::NumLayers + 1];
			roomParams.fbtl = roomParamsPack[8 * LatticeCascade::NumLayers + 2];
			roomParams.fbtr = roomParamsPack[8 * LatticeCascade::NumLayers + 3];

			SetRoomSize(reverbParams.roomSize);
			SetDecayTime(reverbParams.decayTime);
			SetDiffusion(reverbParams.diffusion);
			SetAttack(reverbParams.mixAttack);

			//std::vector<float> earlyOutKs;
			//earlyOutKs.resize(LatticeCascade::NumLayers);
			//for (auto& v : earlyOutKs)v = 1.0 / sqrtf((float)LatticeCascade::NumLayers);
			//earlyl.SetOutKs(earlyOutKs.data());
			//earlyr.SetOutKs(earlyOutKs.data());

			earlyl.SetOutKs(roomParams.outksl);
			earlyr.SetOutKs(roomParams.outksr);
			latel.SetOutKs(roomParams.outksl);
			later.SetOutKs(roomParams.outksr);

			Reset();
		}
		float randf()
		{
			return (float)(rand() % 1000) / 1000.0f * (rand() % 2 ? 1 : -1);
		}
		void InitRoomParams(std::vector<float>& roomParamsPack)
		{
			for (int i = 0; i < LatticeCascade::NumLayers; ++i)
			{
				float tsl = fabsf(randf()) * 0.8 + 0.2;//tsl
				float tsr = fabsf(randf()) * 0.8 + 0.2;//tsr
				float ksl = randf();//ksl
				float ksr = randf();//ksr
				float dsl = randf() * 0.1 + 0.90;//dsl
				float dsr = randf() * 0.1 + 0.90;//dsr
				float outksl = randf();//outksl
				float outksr = randf();//outksr

				roomParamsPack[i + 0 * LatticeCascade::NumLayers] = tsl;//tsl
				roomParamsPack[i + 1 * LatticeCascade::NumLayers] = tsr;//tsr
				roomParamsPack[i + 2 * LatticeCascade::NumLayers] = ksl;//ksl
				roomParamsPack[i + 3 * LatticeCascade::NumLayers] = ksr;//ksr
				roomParamsPack[i + 4 * LatticeCascade::NumLayers] = dsl;//dsl
				roomParamsPack[i + 5 * LatticeCascade::NumLayers] = dsr;//dsr
				roomParamsPack[i + 6 * LatticeCascade::NumLayers] = outksl;//outksl
				roomParamsPack[i + 7 * LatticeCascade::NumLayers] = outksr;//outksr
			}
			roomParamsPack[8 * LatticeCascade::NumLayers + 0] = -0.75;//fbdl
			roomParamsPack[8 * LatticeCascade::NumLayers + 1] = -0.75;//fbdr
			roomParamsPack[8 * LatticeCascade::NumLayers + 2] = fabsf(randf());//fbtl
			roomParamsPack[8 * LatticeCascade::NumLayers + 3] = fabsf(randf());//fbtr
		}
	};
}