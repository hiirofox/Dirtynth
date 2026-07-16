#pragma once

namespace Dirtynth
{

	constexpr static int EnvelopeUpdateInterval = 6;//包络更新间隔
	constexpr static int MaxPolyphony = 64;//最大复音数
	constexpr static int NumMutantThreads = 2;//给mutant工作的线程个数
	constexpr static int MaxBlockSize = 256;//内部处理用的最大块大小


	constexpr static int NumWavetablePresets = 1;//波表预设数量
	constexpr static int NumMutantTypes = 5;//波表mutant种类

	constexpr static int NumEnvelopes = 12;//包络数量
	constexpr static int NumEnvelopeTypes = 7;//包络种类个数
	constexpr static int NumEnvelopeModes = 5;//包络模式个数

	constexpr static int NumFilterTypes = 7;//滤波器种类个数

	constexpr static int NumEffects = 2;//效果器数量
	constexpr static int NumEffectTypes = 4;//效果器种类个数

}