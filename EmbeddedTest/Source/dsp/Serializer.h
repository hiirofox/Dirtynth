#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <sstream>

class SerializerRead
{
private:
	std::string data;
	size_t cursor = 0; // 当前读取到的字符串位置索引
	std::vector<std::string> blockStack; // 记录进入的块，用于验证结构

	// 内部辅助函数：跳过空白字符查找目标
	size_t FindNext(const std::string& target);

public:
	void ReadSerialDataFromString(std::string& data); // 从字符串开始反序列化

	void IntoBlock(std::string blockTag); // 进入块
	bool TryIntoBlock(std::string blockTag); // 尝试进入块，找不到时不移动读取位置
	void Break(); // 退出块

	float GetTagFloat(std::string tag); // 获取标签值（浮点）
	int GetTagInt(std::string tag);
	std::string GetTagString(std::string tag); // 获取标签值（字符串）
};

class SerializerWrite
{
private:
	std::string data;
	std::vector<std::string> blockStack; // 栈，用于存储当前打开的块名

	// 辅助获取缩进字符串
	std::string GetIndent();

public:
	std::string& GetSerialData() { return data; }

	void MakeBlock(std::string blockTag);
	void Break();

	void WriteTagFloat(std::string tag, float value);
	void WriteTagInt(std::string tag, int value);
	void WriteTagString(std::string tag, std::string str);
};

/*
持久化数据示例：
[MidiChannel]
	numInstrument = 2
	[Instrument1]
		type = "SF2"
		file = "../sf2/a.sf2"
		midiChannel = 0
		sendTrackID = 2
	[/Instrument1]
	[Instrument2]
		type = "SF2"
		file = "../sf2/b.sf2"
		midiChannel = 1
		sendTrackID = 1
	[/Instrument2]
[/MidiChannel]
[Tracks]
	numTracks = 3
	[Chain1]
		numEffects = 2
		outputGain = 1.0
		[Effect1]
			type = "EQ"
			[EQ]
				numNodes = 1
				[Node1]
					type = "Peaking"
					cutoff = 2046.1272246869823
					Q = 2.32058427723469
					gain = 1.0000000000000
				[/Node1]
			[/EQ]
		[/Effect1]
		[Effect2]
			type = "Delay"
			delayTime = 618.2263712656
			feedback = 0.5000000000
			mix = 1.0
		[/Effect2]
	[/Chain1]
	[Chain2]
		numEffects = 0
		outputGain = 0.5
	[/Chain2]
[/Tracks]



*/