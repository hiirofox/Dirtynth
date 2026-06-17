#include "Serializer.h"
#include <iostream>

// ==========================================
// SerializerRead 实现
// ==========================================

void SerializerRead::ReadSerialDataFromString(std::string& data)
{
	this->data = data;
	this->cursor = 0;
	this->blockStack.clear();
}

size_t SerializerRead::FindNext(const std::string& target)
{
	size_t pos = data.find(target, cursor);
	if (pos == std::string::npos)
	{
		//throw std::runtime_error("Parse Error: Unable to find '" + target + "' starting from position " + std::to_string(cursor));
		return std::string::npos;
	}
	return pos;
}

void SerializerRead::IntoBlock(std::string blockTag)
{
	if (!TryIntoBlock(blockTag))
	{
		blockStack.push_back(blockTag);
	}
}

bool SerializerRead::TryIntoBlock(std::string blockTag)
{
	// 构造块标签 [Tag]
	std::string target = "[" + blockTag + "]";

	// 查找并移动光标
	const size_t pos = FindNext(target);
	if (pos == std::string::npos)
	{
		return false;
	}

	cursor = pos + target.length();

	// 入栈
	blockStack.push_back(blockTag);
	return true;
}

void SerializerRead::Break()
{
	try
	{
		if (blockStack.empty())
		{
			return;
		}

		std::string currentBlock = blockStack.back();

		// 构造闭合标签 [/Tag]
		std::string target = "[/" + currentBlock + "]";

		size_t pos = FindNext(target);
		if (pos != std::string::npos)
		{
			cursor = pos + target.length();
		}

		blockStack.pop_back();
	}
	catch (const std::exception& e)
	{
		//throw std::runtime_error(std::string("Break Error: ") + e.what());
	}
}

float SerializerRead::GetTagFloat(std::string tag)
{
	try
	{
		// 1. 查找 "tag ="
		std::string key = tag + " =";
		size_t keyPos = FindNext(key);
		if (keyPos == std::string::npos) {
			//throw std::runtime_error("Missing tag");
			return 0;
		}

		// 2. 找到值开始的位置
		size_t valStart = keyPos + key.length();

		// 3. 读取数值
		// std::stof 会自动跳过前面的空白符并读取数字
		size_t endOfNum = 0;
		// 截取从 valStart 开始的子串以进行解析
		std::string sub = data.substr(valStart);
		float val = std::stof(sub, &endOfNum);

		// 4. 更新光标到数值之后
		cursor = valStart + endOfNum;

		return val;
	}
	catch (const std::exception& e)
	{
		////throw std::runtime_error("GetTagFloat Error for tag '" + tag + "': " + e.what());
	}
}

int SerializerRead::GetTagInt(std::string tag)
{
	try
	{
		// 1. 查找 "tag ="
		std::string key = tag + " =";
		size_t keyPos = FindNext(key);
		if (keyPos == std::string::npos) {
			//throw std::runtime_error("Missing tag");
			return 0;
		}
		// 2. 找到值开始的位置
		size_t valStart = keyPos + key.length();

		// 3. 读取数值
		// std::stof 会自动跳过前面的空白符并读取数字
		size_t endOfNum = 0;
		// 截取从 valStart 开始的子串以进行解析
		std::string sub = data.substr(valStart);
		int val = std::stoi(sub, &endOfNum);

		// 4. 更新光标到数值之后
		cursor = valStart + endOfNum;

		return val;
	}
	catch (const std::exception& e)
	{
		//throw std::runtime_error("GetTagFloat Error for tag '" + tag + "': " + e.what());
	}
}

std::string SerializerRead::GetTagString(std::string tag)
{
	try
	{
		// 1. 查找 "tag ="
		std::string key = tag + " =";
		size_t keyPos = FindNext(key);
		if (keyPos == std::string::npos) {
			//throw std::runtime_error("Missing tag");
			return "";
		}
		// 2. 查找第一个引号
		size_t quoteStart = data.find("\"", keyPos + key.length());
		if (quoteStart == std::string::npos) {
			//throw std::runtime_error("Missing opening quote");
			return "";
		}
		// 3. 查找第二个引号
		size_t quoteEnd = data.find("\"", quoteStart + 1);
		if (quoteEnd == std::string::npos) {
			//throw std::runtime_error("Missing closing quote");
			return "";
		}
		// 4. 提取内容
		std::string val = data.substr(quoteStart + 1, quoteEnd - quoteStart - 1);

		// 5. 更新光标
		cursor = quoteEnd + 1;

		return val;
	}
	catch (const std::exception& e)
	{
		//throw std::runtime_error("GetTagString Error for tag '" + tag + "': " + e.what());
	}
}

// ==========================================
// SerializerWrite 实现
// ==========================================

std::string SerializerWrite::GetIndent()
{
	// 根据栈的深度生成 tab 缩进
	std::string indent = "";
	for (size_t i = 0; i < blockStack.size(); ++i)
	{
		indent += "\t";
	}
	return indent;
}

void SerializerWrite::MakeBlock(std::string blockTag)
{
	// 写入：缩进 + [Tag] + 换行
	data += GetIndent() + "[" + blockTag + "]\n";
	blockStack.push_back(blockTag);
}

void SerializerWrite::Break()
{
	try
	{
		if (blockStack.empty())
		{
			//throw std::runtime_error("SerializerWrite Break Error: Stack is empty.");
		}

		std::string currentBlock = blockStack.back();
		blockStack.pop_back();

		// 写入：缩进 + [/Tag] + 换行
		data += GetIndent() + "[/" + currentBlock + "]\n";
	}
	catch (const std::exception& e)
	{
		// 在写入类中通常不应该抛出异常，但在调试阶段很有用
		//throw;
	}
}

void SerializerWrite::WriteTagFloat(std::string tag, float value)
{
	// 写入：缩进 + tag = value + 换行
	// std::to_string 默认精度可能不够，如果需要高精度可以使用 stringstream
	data += GetIndent() + tag + " = " + std::to_string(value) + "\n";
}

void  SerializerWrite::WriteTagInt(std::string tag, int value)
{
	data += GetIndent() + tag + " = " + std::to_string(value) + "\n";
}

void SerializerWrite::WriteTagString(std::string tag, std::string str)
{
	// 写入：缩进 + tag = "str" + 换行
	data += GetIndent() + tag + " = \"" + str + "\"\n";
}