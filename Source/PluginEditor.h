/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "ui/DirtynthUI.h"
#include "ui/PresetComponent.h"
//==============================================================================
/**
*/
class DirtynthAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
	DirtynthAudioProcessorEditor(DirtynthAudioProcessor&);
	~DirtynthAudioProcessorEditor() override;

	//==============================================================================
	void paint(juce::Graphics&) override;
	void resized() override;

private:
	// This reference is provided as a quick way for your editor to
	// access the processor object that created it.
	DirtynthAudioProcessor& audioProcessor;
	DirtynthUI dirtynthUI;
	PresetComponent presetComponent;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DirtynthAudioProcessorEditor)
};
