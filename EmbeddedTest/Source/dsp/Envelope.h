#pragma once
#include <math.h>

namespace MinusMKI
{
	enum class EnvelopeMode {
		PolyphonicResetOnNoteOn = 0,	//复音，按下时重置
		PolyphonicNoReset = 1,			//复音，无重置
		GlobalNoReset = 2,				//全局，无重置
		GlobalResetOnFirstNoteOn = 3,	//全局，按下第一个键重置
		GlobalResetOnAnyNoteOn = 4		//全局，按下任意键重置
	};
	enum class ControlSourceType
	{
		NoteGate = -3,//note on触发1，note off触发0(二值)
		NotePitch = -2,//归一化key num，即key/128
		Velocity = -1,//力度
		Aftertouch = 0,//触后

		CC1 = 1,//CC1调制轮
		CC2 = 2,//CC2呼吸控制器
		CV1 = 3,//CV1电压
		CV2 = 4,//CV2电压
	};
	class Envelope
	{
	private:
	public:
		virtual void Reset() {};
		virtual void SetSampleRate(float sr) {};//设置采样率
		virtual void SetParams(float p1, float p2, float p3, float p4, float p5, float p6) {};//设置包络参数
		virtual void Step() {};//步进
		virtual float GetValue() { return 0.0f; };//获取值

		virtual void SetControlValue(float cv) = 0;//设置控制值，可以是velocity,cc,mpe,cv等等，仅用于调制源
		virtual void SetControlSourceType(ControlSourceType cst) = 0;
		virtual ControlSourceType GetControlSourceType() = 0;
	};
	class ADSR :public Envelope
	{
	private:
		ControlSourceType cst = ControlSourceType::NoteGate;
		float sampleRate = 48000;
		//0:release 1:attack 2:decay 3:sustain
		float s[4] = { 0 }, k[4] = { 0 }, r[4] = { 0 };
		float pos = 0;
		float y = 0;

		float attl = 100.0, attShape = -1;
	public:
		//ADSR() { SetControlSourceType(ControlSourceType::NoteGate); }
		ADSR(ControlSourceType cst) { SetControlSourceType(cst); }
		void SetControlSourceType(ControlSourceType cst) override { this->cst = cst; }
		ControlSourceType GetControlSourceType() override { return cst; }

		void Step() override
		{
			int idx = pos;
			if (idx < 0)idx = 0;
			if (idx > 3)idx = 3;
			y = s[idx] * y + k[idx];
			pos += r[idx];
			if (pos > 3.0f)pos = 3.0f;
		}
		void Reset() override
		{
			pos = 0.0;
			y = 0.0;
		}
		void SetControlValue(float cv) override //一切皆cv！
		{
			if (cv > 0.5)
			{
				pos = 1.0;
				//y = 0.0;//attack强制从0开始
				SetSegment(y, 1.0, attl, attShape, 1);//attack继续release的值开始
			}
			else
			{
				pos = 0.0;
			}
		}
		float GetValue() override { return y; }
		void SetSampleRate(float sr) override { sampleRate = sr; }
		void SetSegment(float y0, float y1, float len, float shape, int i)
		{
			if (len < 4.0)len = 4.0;
			float sv = 0, kv = 0, rv = 0;
			if (fabsf(shape) > 0.01)
			{
				sv = expf(shape / len);
				float expshape = expf(shape);
				kv = (y1 - y0 * expshape) * (sv - 1.0) / (expshape - 1.0);
				rv = 1.0 / len;
			}
			else
			{
				sv = 1.0;
				kv = (y1 - y0) / len;
				rv = 1.0 / len;
			}
			s[i] = sv;
			k[i] = kv;
			r[i] = rv;
		}
		float EvalSegment(float y0, float y1, float shape, float phase)
		{
			if (fabsf(shape) > 0.01f)
			{
				float expshape = expf(shape);
				float expphase = expf(shape * phase);
				return y0 * expphase + (y1 - y0 * expshape) * (expphase - 1.0f) / (expshape - 1.0f);
			}
			return y0 + (y1 - y0) * phase;
		}
		void SetParams(float attMs, float attShape, float decMs, float decShape, float susV, float relMs) override
		{
			float attl = sampleRate / 1000.0 * attMs;
			float decl = sampleRate / 1000.0 * decMs;
			float rell = sampleRate / 1000.0 * relMs;
			//release
			if (rell < 4.0)rell = 4.0;
			s[0] = expf(-6.90775527898f / rell);//rt60
			k[0] = 0.0;
			r[0] = 0.0;
			//sustain
			s[3] = 1.0;
			k[3] = 0.0;
			r[3] = 0.0;
			//attack
			this->attl = attl;
			this->attShape = attShape;
			SetSegment(0.0, 1.0, attl, attShape, 1);
			//decay
			SetSegment(1.0, susV, decl, decShape, 2);
			//update y
			int idx = pos;
			float posFrac = pos - idx;
			if (idx == 1) y = EvalSegment(0.0f, 1.0f, attShape, posFrac);
			else if (idx == 2) y = EvalSegment(1.0f, susV, decShape, posFrac);
			else if (idx == 3) y = susV;
		}

	};

	class ModSource :public Envelope
	{
	private:
		ControlSourceType cst;
		float sampleRate = 48000.0;
		float cv = 0.0;
		float z1 = 0.0, z2 = 0.0;
		float y = 0.0;

		float curve = 0.0;
		float downbit = 0.0;
		float smooth = 0.0;
		float overshoot = 0.0;
		float hp = 0.0;
		float trajitter = 0.0;

		float Curve(float x, float curve)
		{
			curve *= 12.0;
			if (fabsf(curve) < 0.001)return x;
			float sign = x;
			x = fabsf(x);
			float y = (expf((x - 1.0) * curve) - expf(-curve)) / (1.0 - expf(-curve));
			return std::copysignf(y, sign);
		}
		float Downbit(float x, float perceptBit)
		{
			perceptBit = Curve(perceptBit, 1.0);
			const float quanti = 65536.0 * 4.0;
			int ix = (1.0 - x) * perceptBit * quanti;
			return ix / perceptBit / quanti;
		}
	public:
		//ModSource() { SetControlSourceType(ControlSourceType::NoteGate); };
		ModSource(ControlSourceType cst) { SetControlSourceType(cst); }

		void Reset() override
		{
			y = cv;
			z1 = cv, z2 = cv;
		};
		void SetSampleRate(float sr) override { sampleRate = sr; }
		void SetNoteState(bool off0on1) {}
		void SetParams(float curve, float downbit, float smooth, float overshoot, float hp, float trajitter) override
		{
			this->curve = curve;
			this->downbit = downbit;
			this->smooth = Curve(1.0 - smooth, 1.0);//1
			this->overshoot = overshoot;//2
			this->hp = Curve(hp, 1.0);//3
			this->trajitter = trajitter;//4
		}
		void Step() override
		{
			//z1 += smooth * (cv - z1);
			//z2 += hp * (z1 - z2);
			//y = z1 - z2;
			y = cv;
		}
		float GetValue() override
		{
			if (y < 0)y = 0;
			if (y > 1.0)y = 1.0;
			if (isnan(y))y = 0;
			return y;
		};

		void SetControlValue(float cv) override
		{
			//this->cv = Downbit(Curve(cv, curve), downbit);
			//this->cv = Curve(cv, curve);
			this->cv = cv;
		}
		void SetControlSourceType(ControlSourceType cst) override { this->cst = cst; }
		ControlSourceType GetControlSourceType() override { return cst; }
	};
}