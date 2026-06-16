/*
  ==============================================================================

	This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
	constexpr int DirtynthStateMagic = 0x44727468; // Drth
	constexpr int DirtynthStateVersion = 1;

	Dirtynth::DirtynthParams makeDefaultProcessorParams()
	{
		Dirtynth::DirtynthParams params;
		return params;
	}
}

//==============================================================================
DirtynthAudioProcessor::DirtynthAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
	: AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
		.withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
		.withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
	)
#endif
{
	applyParamsToEngine(makeDefaultProcessorParams());
}

DirtynthAudioProcessor::~DirtynthAudioProcessor()
{
}

//==============================================================================
const juce::String DirtynthAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

bool DirtynthAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool DirtynthAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

bool DirtynthAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
	return true;
#else
	return false;
#endif
}

double DirtynthAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int DirtynthAudioProcessor::getNumPrograms()
{
	return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
	// so this should be at least 1, even if you're not really implementing programs.
}

int DirtynthAudioProcessor::getCurrentProgram()
{
	return 0;
}

void DirtynthAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String DirtynthAudioProcessor::getProgramName(int index)
{
	return {};
}

void DirtynthAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void DirtynthAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	// Use this method as the place to do any pre-playback
	// initialisation that you need..
	dirtynth.SetSampleRate(static_cast<float>(sampleRate));
}

void DirtynthAudioProcessor::applyParamsToEngine(const Dirtynth::DirtynthParams& params)
{
	dirtynth.SetParams(params);

	float dummyLeft = 0.0f;
	float dummyRight = 0.0f;
	dirtynth.ProcessBlock(nullptr, 0, &dummyLeft, &dummyRight, 0);
}

void DirtynthAudioProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DirtynthAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
	juce::ignoreUnused(layouts);
	return true;
#else
	// This is the place where you check if the layout is supported.
	// In this template code we only support mono or stereo.
	// Some plugin hosts, such as certain GarageBand versions, will only
	// load plugins that support stereo bus layouts.
	if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
		&& layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
		return false;

	// This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;
#endif

	return true;
#endif
}
#endif

void DirtynthAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	int midiQueueLen = 0;
	juce::MidiMessage midiMessage;
	int samplePosition = 0;
	juce::MidiBuffer::Iterator midiIterator(midiMessages);
	while (midiIterator.getNextEvent(midiMessage, samplePosition))
	{
		DirtynthMidiEvent event;

		if (midiMessage.isNoteOn())
		{
			event.type = DirtynthMidiEvent::NoteOn;
			event.note = midiMessage.getNoteNumber();
			event.velocity = static_cast<int>(midiMessage.getVelocity());
			midiQueue[midiQueueLen++] = event;
		}
		else if (midiMessage.isNoteOff())
		{
			event.type = DirtynthMidiEvent::NoteOff;
			event.note = midiMessage.getNoteNumber();
			midiQueue[midiQueueLen++] = event;
		}
		else if (midiMessage.isController())
		{
			event.type = DirtynthMidiEvent::ControlChange;
			event.controlNumber = midiMessage.getControllerNumber();
			event.controlValue = midiMessage.getControllerValue();
			midiQueue[midiQueueLen++] = event;
		}
		else if (midiMessage.isPitchWheel())
		{
			event.type = DirtynthMidiEvent::PitchBend;
			event.pitchBendValue = midiMessage.getPitchWheelValue();
			midiQueue[midiQueueLen++] = event;
		}
	}

	int numSamples = buffer.getNumSamples();
	float* wavbufl = buffer.getWritePointer(0);
	float* wavbufr = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : nullptr;

	dirtynth.ProcessBlock(midiQueue, midiQueueLen, wavbufl, wavbufr, numSamples);
}

//==============================================================================
bool DirtynthAudioProcessor::hasEditor() const
{
	return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DirtynthAudioProcessor::createEditor()
{
	return new DirtynthAudioProcessorEditor(*this);
}

//==============================================================================
void DirtynthAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	destData.reset();

	const auto params = dirtynth.GetParams();
	juce::MemoryOutputStream stream(destData, false);
	stream.writeInt(DirtynthStateMagic);
	stream.writeInt(DirtynthStateVersion);
	stream.writeInt(static_cast<int>(sizeof(Dirtynth::DirtynthParams)));
	stream.write(&params, sizeof(params));
}

void DirtynthAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	juce::MemoryInputStream stream(data, static_cast<size_t>(sizeInBytes), false);

	if (stream.readInt() != DirtynthStateMagic)
		return;

	if (stream.readInt() != DirtynthStateVersion)
		return;

	const int paramsSize = stream.readInt();
	if (paramsSize != static_cast<int>(sizeof(Dirtynth::DirtynthParams)))
		return;

	Dirtynth::DirtynthParams params;
	if (stream.read(&params, sizeof(params)) != sizeof(params))
		return;

	applyParamsToEngine(params);
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new DirtynthAudioProcessor();
}
