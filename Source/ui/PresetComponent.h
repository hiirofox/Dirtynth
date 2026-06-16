#pragma once

#include <JuceHeader.h>

#include "../dsp/DirtySystem.h"

class PresetComponent : public juce::Component,
	public juce::FileDragAndDropTarget
{
public:
	explicit PresetComponent(Dirtynth::DirtynthSystem& instance)
		: instance(instance)
	{
		setSize(64, 32);
		setMouseCursor(juce::MouseCursor::DraggingHandCursor);
	}

	void paint(juce::Graphics& g) override
	{
		const auto bounds = getLocalBounds().toFloat().reduced(1.0f);
		const auto baseColour = isFileDragOver ? juce::Colour(0xff46d37f) : juce::Colour(0xffb6f35c);

		g.setColour(juce::Colour(0xff101010).withAlpha(0.92f));
		g.fillRoundedRectangle(bounds, 4.0f);

		g.setColour(baseColour);
		g.drawRoundedRectangle(bounds, 4.0f, 1.5f);

		g.setColour(baseColour.withAlpha(0.22f));
		g.fillRect(getLocalBounds().removeFromBottom(3));

		g.setColour(baseColour);
		g.setFont(juce::Font(12.0f, juce::Font::bold));
		g.drawText("PRESET", getLocalBounds(), juce::Justification::centred, false);
	}

	void mouseDown(const juce::MouseEvent&) override
	{
		hasStartedExternalDrag = false;
	}

	void mouseDrag(const juce::MouseEvent& event) override
	{
		if (hasStartedExternalDrag || event.getDistanceFromDragStart() < 4)
			return;

		hasStartedExternalDrag = true;
		dragCurrentPresetToFile();
	}

	void mouseUp(const juce::MouseEvent&) override
	{
		hasStartedExternalDrag = false;
	}

	bool isInterestedInFileDrag(const juce::StringArray& files) override
	{
		for (const auto& fileName : files)
		{
			const juce::File file(fileName);
			if (file.existsAsFile() && file.hasFileExtension(".txt"))
				return true;
		}

		return false;
	}

	void fileDragEnter(const juce::StringArray&, int, int) override
	{
		isFileDragOver = true;
		repaint();
	}

	void fileDragExit(const juce::StringArray&) override
	{
		isFileDragOver = false;
		repaint();
	}

	void filesDropped(const juce::StringArray& files, int, int) override
	{
		isFileDragOver = false;
		repaint();

		for (const auto& fileName : files)
		{
			const juce::File file(fileName);
			if (file.existsAsFile() && file.hasFileExtension(".txt"))
			{
				loadPresetFromFile(file);
				return;
			}
		}
	}

private:
	static juce::String makePresetFileName()
	{
		const auto now = juce::Time::getCurrentTime();
		return "Preset" +
			juce::String::formatted("%04d%02d%02d_%02d%02d%02d",
				now.getYear(),
				now.getMonth() + 1,
				now.getDayOfMonth(),
				now.getHours(),
				now.getMinutes(),
				now.getSeconds()) +
			".txt";
	}

	static juce::File getPresetTempDirectory()
	{
		return juce::File::getSpecialLocation(juce::File::tempDirectory)
			.getChildFile("DirtynthPresets");
	}

	void dragCurrentPresetToFile()
	{
		const juce::File tempDirectory = getPresetTempDirectory();
		if (!tempDirectory.exists() && !tempDirectory.createDirectory())
			return;

		const juce::File presetFile = tempDirectory.getChildFile(makePresetFileName());
		savePresetToFile(presetFile);

		if (!presetFile.existsAsFile())
			return;

		juce::StringArray files;
		files.add(presetFile.getFullPathName());
		juce::DragAndDropContainer::performExternalDragDropOfFiles(files, false);
	}

	void savePresetToFile(const juce::File& file)
	{
		Dirtynth::DirtynthParamSystem paramSystem;
		const std::string presetData = paramSystem.SaveParamsToString(instance.GetParams());
		file.replaceWithText(juce::String(presetData), false, false, "\n");
	}

	void loadPresetFromFile(const juce::File& file)
	{
		const juce::String presetData = file.loadFileAsString();
		if (presetData.isEmpty())
			return;

		Dirtynth::DirtynthParamSystem paramSystem;
		instance.SetParams(paramSystem.LoadParamsFromString(presetData.toStdString()));
	}

	Dirtynth::DirtynthSystem& instance;
	bool hasStartedExternalDrag = false;
	bool isFileDragOver = false;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PresetComponent)
};
