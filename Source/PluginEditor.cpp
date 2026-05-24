/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DirtynthAudioProcessorEditor::DirtynthAudioProcessorEditor(DirtynthAudioProcessor& p)
	: AudioProcessorEditor(&p), audioProcessor(p)
{
	// Make sure that before the constructor has finished, you've set the
	// editor's size to whatever you need it to be.
	setSize(1024, 480);
	addAndMakeVisible(dirtynthUI);
}

DirtynthAudioProcessorEditor::~DirtynthAudioProcessorEditor()
{
}

//==============================================================================
void DirtynthAudioProcessorEditor::paint(juce::Graphics& g)
{
	// (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll(juce::Colour(0xff000000));
}

void DirtynthAudioProcessorEditor::resized()
{
	// This is generally where you'll want to lay out the positions of any
	// subcomponents in your editor..
	dirtynthUI.setBounds(0, 0, getWidth(), getHeight());
}
