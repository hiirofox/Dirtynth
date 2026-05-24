#include "LMKnobDirect.h"
//
// ==============================================================================
// LMKnobDirect 实现
// ==============================================================================
//

LMKnobDirect::LMKnobDirect() :slider(), label()
{
	slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
	slider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
	L_MODEL_STYLE_LOOKANDFEEL = std::make_unique<L_MODEL_STYLE>();
	slider.setLookAndFeel(L_MODEL_STYLE_LOOKANDFEEL.get());//Ó¦ÓÃl-model·ç¸ñ
	label.setJustificationType(juce::Justification::centredTop);
	label.setFont(juce::Font("FIXEDSYS", 15.0, 0));
	label.setMinimumHorizontalScale(1.0);//²»Ëõ·Å×ÖÌå
	label.setColour(juce::Label::textColourId, juce::Colour(0x77, 0xff, 0x77));
	label.setFont(label.getFont().withStyle(juce::Font::bold));//ÉèÖÃ´ÖÌå

	// *** 关键改动 ***
	// 设置 slider 的 onValueChange 回调
	// 当 slider 的值改变时，调用我们存储的回调函数
	slider.onValueChange = [this]
		{
			if (onValueChangeCallback)
			{
				onValueChangeCallback(slider.getValue());
			}
		};

	setPaintingIsUnclipped(true);//×é¼þÎÞ±ß½ç
	setOpaque(false);//×é¼þºÚÉ«²¿·ÖÍ¸Ã÷

	addAndMakeVisible(slider);
	addAndMakeVisible(label);

	startTimerHz(5);
}

LMKnobDirect::~LMKnobDirect()
{
	slider.setLookAndFeel(nullptr);
	// onValueChangeCallback 会自动销毁，无需处理
}

void LMKnobDirect::paint(juce::Graphics& g)
{
	// (与 LMKnob 相同，为空)
}
void LMKnobDirect::ParamLink(float minv, float maxv, float defaultValue, float& currentValue, std::function<void(float)> onValueChange)
{
	slider.setRange(minv, maxv);
	slider.setDoubleClickReturnValue(true, defaultValue);

	// 存储回调函数
	onValueChangeCallback = std::move(onValueChange);

	// *** 修改点 ***
	// 存储外部变量的地址，而不是试图拷贝值
	this->currentValue = &currentValue;

	// currentValue 仍然是一个引用，所以用 &currentValue 来获取它的地址
	slider.setValue(currentValue, juce::dontSendNotification);
}

void LMKnobDirect::timerCallback()
{
	//检测是否在调节
	if (slider.isMouseButtonDown()) return;

	// *** 修改点 ***
	// 检查指针是否有效（非空）
	if (currentValue != nullptr)
	{
		// 使用 *currentValue 来解引用指针，获取它所指向的外部变量的 *值*
		slider.setValue(*currentValue, juce::dontSendNotification);
	}
}

void LMKnobDirect::setValue(float newValue)
{
	// 外部调用此函数来更新UI，例如加载预设时
	// 我们不希望这个设置触发 onValueChange 回调（避免循环调用）
	slider.setValue(newValue, juce::dontSendNotification);
}

void LMKnobDirect::setText(const juce::String& KnobText)
{
	label.setText(KnobText, juce::dontSendNotification);
	text = KnobText;
}

void LMKnobDirect::resized()
{
	slider.setBounds(32 - 56 / 2, 32 - 56 / 2, 56, 56);
	label.setBounds(-32, 48 - 4, 64 + 64, 16);
}

void LMKnobDirect::setPos(int x, int y)
{
	setBounds(x - 32, y - 32, 64, 64);
}
