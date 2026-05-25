#pragma once

#define _USE_MATH_DEFINES
#include <math.h>

namespace LMDelayNamespace
{
	template<int MaxDelayLen>
	class DelayLine
	{
	private:
		float dat[MaxDelayLen] = { 0 };
		float out = 0;

		// 平滑处理
		float currentDelay = 0;
		float targetDelay = 0;
		float delayVelocity = 0; // 用于线性平滑延迟时间本身

		int pos = 0;

		inline float ReadSampleHermite(float delay)
		{
			float readPos = (float)pos - delay;
			while (readPos < 0) readPos += MaxDelayLen;
			while (readPos >= MaxDelayLen) readPos -= MaxDelayLen;

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
			while (readPos < 0) readPos += MaxDelayLen;
			while (readPos >= MaxDelayLen) readPos -= MaxDelayLen;
			int i1 = (int)readPos;
			int i2 = (i1 + 1) % MaxDelayLen;
			float f = readPos - i1;
			return dat[i1] * (1.0f - f) + dat[i2] * f;
		}
	public:
		constexpr static int GradientSamples = 5000;

		DelayLine()
		{
			std::fill(dat, dat + MaxDelayLen, 0.0f);
		}

		inline void SetDelayTime(float t)
		{
			if (t < 2)t = 2;
			if (t > MaxDelayLen - 4)t = MaxDelayLen - 4;
			targetDelay = t;
			delayVelocity = (targetDelay - currentDelay) / (float)GradientSamples;
		}

		inline float ReadSample()
		{
			return out;
		}

		inline void WriteSample(float val)
		{
			dat[pos] = val;

			if (fabsf(targetDelay - currentDelay) > 0.0001f)
			{
				currentDelay += delayVelocity;
			}
			else
			{
				currentDelay = targetDelay;
			}

			out = ReadSampleHermite(currentDelay);

			if (++pos >= MaxDelayLen) pos = 0;
		}
	};
}

class LMDelay
{
private:
	float sampleRate = 48000;
	LMDelayNamespace::DelayLine<48000> delayL;
	LMDelayNamespace::DelayLine<48000> delayR;
	LMDelayNamespace::DelayLine<48000> preDelay;
	float dampvl = 0, dampvr = 0;
	float bounce = 0, width = 0, delayTime = 200, feedback = 0.5, damp = 0.0, mix = 0.5;
public:
	LMDelay(float sampleRate = 48000) : sampleRate(sampleRate)
	{
		delayL.SetDelayTime(delayTime * sampleRate / 1000.0f);
		delayR.SetDelayTime(delayTime * sampleRate / 1000.0f);
		preDelay.SetDelayTime(delayTime * sampleRate / 1000.0f);
	}

	void SetParams(float bounce, float width, float delayTime, float feedback, float damp, float mix)
	{
		this->bounce = bounce;
		this->width = width;
		this->delayTime = delayTime;
		this->feedback = feedback;
		this->damp = damp;
		this->mix = mix;

		delayL.SetDelayTime(delayTime * sampleRate / 1000.0f);
		delayR.SetDelayTime(delayTime * sampleRate / 1000.0f);
		preDelay.SetDelayTime(delayTime * sampleRate / 1000.0f);
	}
	void ProcessBlock(const float* inL, const float* inR, float* outL, float* outR, int numSamples)
	{
		float dampfix = expf(-damp * 3.0);
		float bounceA = 1.0 - bounce;
		float bounceB = bounce;
		float widthfix = (width + 1.0) * 0.5;
		float widthA = 1.0 - widthfix;
		float widthB = widthfix;
		float feedbackBounceFix = bounceA * feedback + bounceB;
		for (int i = 0; i < numSamples; ++i)
		{
			float inl = inL[i];
			float inr = inR[i];

			//!:要补偿bounce的作用
			float inl2 = bounceA * inl + bounceB * (inl + inr) * 0.5;//!
			float inr2 = bounceA * inr;//!

			float delayoutl = delayL.ReadSample();
			float delayoutr = delayR.ReadSample();

			dampvl += dampfix * (delayoutl - dampvl);
			dampvr += dampfix * (delayoutr - dampvr);
			delayL.WriteSample(inl2 + dampvr * feedback);
			delayR.WriteSample(inr2 + dampvl * feedbackBounceFix);//!

			float tapOutL = dampvl;
			float tapOutR = dampvr;

			float wetl = tapOutL * widthA + tapOutR * widthB;
			float wetr = tapOutR * widthA + tapOutL * widthB;

			outL[i] = inl * (1.0f - mix) + wetl * mix;
			outR[i] = inr * (1.0f - mix) + wetr * mix;
		}
	}
};