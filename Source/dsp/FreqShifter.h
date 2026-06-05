#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

namespace Dirtynth
{
	struct ApfZ2 //(k+z^-2)/(1+kz^-2)
	{
		float x1 = 0.0f, x2 = 0.0f;
		float y1 = 0.0f, y2 = 0.0f;
		inline float ProcessSample(float x, float k)
		{
			float y = x2 + k * (x - y2);
			x2 = x1;
			x1 = x;
			y2 = y1;
			y1 = y;
			return y;
		}
		void Reset() { x1 = x2 = y1 = y2 = 0; }
	};
	struct QuadOsc //vicanek
	{
		float u = 1.0f; // cos
		float v = 0.0f; // sin
		float k1 = 0.0f;
		float k2 = 0.0f;
		inline float CheapTan(float x)
		{
			if (x > M_PI_4) x = M_PI_4; //nyquist/2£¨“≤πª”√
			else if (x < -M_PI_4) x = -M_PI_4;
			float z = x * x;
			float p = (0.00107447718f * z - 0.111351598f) * z + 1.0f;
			float q = (0.0159694364f * z - 0.444684930f) * z + 1.0f;
			return x * p / q;
		}
		inline float cheapCosPi(float x)
		{
			int n = floorf(x);
			float f = x - n;
			float y = 1.0f + f * f * (4.0f * f - 6.0f);
			return (n & 1) ? -y : y;
		}
		inline float cheapSinPi(float x)
		{
			int n = floorf(x);
			float f = x - n;
			float t = f * (1.0f - f);
			float y = t * (3.14159265f + 3.40185714f * t);
			return (n & 1) ? -y : y;
		}
		inline float cheapTanPi(float x)
		{
			return cheapSinPi(x) / cheapCosPi(x);
		}
		void setFreq(float freqHz, float sampleRate = 48000.0)
		{
			k1 = tanf(M_PI * freqHz / sampleRate);
			k2 = 2.0f * k1 / (1.0f + k1 * k1);
		}
		void setFreqNear(float freqHz, float sampleRate = 48000.0)
		{
			//k1 = tanf(M_PI * freqHz / sampleRate);
			k1 = cheapTanPi(freqHz / sampleRate);
			k2 = 2.0f * k1 / (1.0f + k1 * k1);
		}
		inline void ProcessSample(float& cosOut, float& sinOut)
		{
			float w = u - k1 * v;
			v = v + k2 * w;
			u = w - k1 * v;
			cosOut = u;
			sinOut = v;
		}
		void Normalized()
		{
			float r = sqrtf(u * u + v * v);
			u /= r;
			v /= r;
		}
		void Reset()
		{
			u = 1.0;
			v = 0.0;
			k1 = 0;
			k2 = 0;
		}
	};
	class FreqShifter
	{
	private:
		constexpr static int NumStages = 4;
		const float rek[NumStages] = { -0.986879579f,-0.142409132f,-0.693461043f,-0.929835008f };
		const float imk[NumStages] = { -0.437876235f,-0.968507388f,-0.996439527f,-0.849331917f };
		float sampleRate = 48000;
		ApfZ2 are[NumStages], aim[NumStages];
		QuadOsc quadosc;

		float hpv = 0, hpctof = 120.0 / sampleRate;

		template<int i>
		inline float ApfChainRe(float x)
		{
			if constexpr (i >= NumStages)return x;
			else return ApfChainRe<i + 1>(are[i].ProcessSample(x, rek[i]));
		}
		float imz = 0;
		template<int i>
		inline float ApfChainIm(float x)
		{
			if constexpr (i >= NumStages) { float v = imz; imz = x; return v; }
			else return ApfChainIm<i + 1>(aim[i].ProcessSample(x, imk[i]));
		}
	public:
		void Reset()
		{
			for (auto& n : are)n.Reset();
			for (auto& n : aim)n.Reset();
			quadosc.Reset();
			imz = 0;
		}
		inline float ProcessSample(float x, float freq)
		{
			float cosv, sinv;
			quadosc.setFreq(freq);
			quadosc.ProcessSample(cosv, sinv);
			float rev = ApfChainRe<0>(x);
			float imv = ApfChainIm<0>(x);
			return cosv * rev - sinv * imv;
		}
		inline float ProcessSampleFM(float x, float freq)
		{
			hpv += hpctof * (freq - hpv);
			freq = freq - hpv;
			float cosv, sinv;
			quadosc.setFreqNear(freq);
			quadosc.ProcessSample(cosv, sinv);
			float rev = ApfChainRe<0>(x);
			float imv = ApfChainIm<0>(x);
			return cosv * rev - sinv * imv;
		}
		void Normalized()
		{
			quadosc.Normalized();
		}
	};
}