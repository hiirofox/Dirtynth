#pragma once

#include "LM_slider.h"

class LMKnobDirect : public juce::Component, public juce::Timer
{
public:
	LMKnobDirect();
	~LMKnobDirect();

	void paint(juce::Graphics& g) override;
	juce::Slider& getSlider()
	{
		return slider;
	}

	void ParamLink(float minv, float maxv, float defaultValue,
		float& currentValue, std::function<void(float)> onValueChange);

	void setValue(float newValue);

	void setText(const juce::String& KnobText);
	void resized() override;
	void setPos(int x, int y);

	void timerCallback() override;

	float* currentValue = nullptr;

private:
	std::unique_ptr<L_MODEL_STYLE> L_MODEL_STYLE_LOOKANDFEEL;
	std::function<void(float)> onValueChangeCallback;
	Custom1_Slider slider;
	juce::Label label;
	juce::String text;
};