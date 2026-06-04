#pragma once

#include <functional>

#include "LM_slider.h"

class LMKnobDirect : public juce::Component, public juce::Timer
{
public:
	struct KnobFeelRule
	{
		float sliderMin = 0.0f;
		float sliderMax = 1.0f;
		float sliderDefault = 0.0f;
		double interval = 0.0;
		std::function<float(float)> SliderToValue;
		std::function<float(float)> ValueToSlider;
	};

	LMKnobDirect();
	~LMKnobDirect();

	void paint(juce::Graphics& g) override;
	juce::Slider& getSlider()
	{
		return slider;
	}

	void ParamLink(float minv, float maxv, float defaultValue,
		float& currentValue, std::function<void(float)> onValueChange);

	void ClearKnobFeelRule();
	void SetKnobFeelRule(const KnobFeelRule& rule);
	void setValue(float newValue);

	void setText(const juce::String& KnobText);
	void resized() override;
	void setPos(int x, int y);

	void timerCallback() override;

	float* currentValue = nullptr;

private:
	float SliderToParam(float sliderValue) const;
	float ParamToSlider(float paramValue) const;

	std::unique_ptr<L_MODEL_STYLE> L_MODEL_STYLE_LOOKANDFEEL;
	std::function<void(float)> onValueChangeCallback;
	KnobFeelRule knobFeelRule;
	bool hasKnobFeelRule = false;
	Custom1_Slider slider;
	juce::Label label;
	juce::String text;
};
