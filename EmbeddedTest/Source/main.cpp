#include <fstream>
#include <cstdio>

#include "dsp/DirtySystem.h"

#include "WaveIO.h"
#include "MidiIO.h"

#include "Kbhit.h"

WaveIO_I2S waveIO(48000);
MidiIO_Universal midiIO;
Dirtynth::DirtynthSystem dtsys;
Dirtynth::DirtynthParamSystem dtps;

#define NumSamples 512
float datl[NumSamples];
float datr[NumSamples];

#define NumMidiEvents 1024
Dirtynth::DirtynthMidiEvent dtmidi[NumMidiEvents];
int numBlockMidiEvents = 0;

void ProcessMIDI()
{
	numBlockMidiEvents = 0;

	MidiIO_Universal::Event ev;
	while (midiIO.PopEvent(ev))
	{
		const int statusType = ev.status & 0xf0;
		const int channel = ev.status & 0x0f;

		if (statusType == 0x90 && ev.data2 > 0)
		{
			int note = ev.data1;
			float vel = ev.data2;

			dtmidi[numBlockMidiEvents].type = Dirtynth::DirtynthMidiEvent::NoteOn;
			dtmidi[numBlockMidiEvents].note = note;
			dtmidi[numBlockMidiEvents].velocity = vel;
			numBlockMidiEvents++;
			printf("Note On: %d Vel: %.2f\n", note, vel);
		}
		else if (statusType == 0x80 || (statusType == 0x90 && ev.data2 == 0))
		{
			int note = ev.data1;
			dtmidi[numBlockMidiEvents].type = Dirtynth::DirtynthMidiEvent::NoteOff;
			dtmidi[numBlockMidiEvents].note = note;
			dtmidi[numBlockMidiEvents].velocity = 0;
			numBlockMidiEvents++;
			printf("Note Off: %d\n", note);
		}
		else if (statusType == 0xb0)
		{
			int cc_num = ev.data1;
			int cc_val = ev.data2;

			dtmidi[numBlockMidiEvents].type = Dirtynth::DirtynthMidiEvent::ControlChange;
			dtmidi[numBlockMidiEvents].controlNumber = cc_num;
			dtmidi[numBlockMidiEvents].controlValue = cc_val;
			numBlockMidiEvents++;
			if (cc_num == 64)
			{
				printf("Sustain Pedal: %s\n", (cc_val >= 64 ? "ON" : "OFF"));
			}
		}
	}
}

std::string ReadFileToString(const std::string& path)
{
	std::ifstream file(path, std::ios::binary);
	if (!file) {

		printf("cant open file!:%s\n", path.c_str());
		return "";
	}

	printf("open file!:%s\n", path.c_str());
	return std::string(
		std::istreambuf_iterator<char>(file),
		std::istreambuf_iterator<char>()
	);
}

int SelectPreset(int idx)
{
	std::string paramstr = ReadFileToString("Presets/" + std::to_string(idx) + ".txt");
	if (paramstr != "")
	{
		Dirtynth::DirtynthParams dtp = dtps.LoadParamsFromString(paramstr);
		dtsys.SetParams(dtp);
		return 1;
	}
	return 0;
}
int selectedPreset = 1;


int main()
{
#ifndef _WIN32
	kb_init();
#endif

	midiIO.Start();

	SelectPreset(selectedPreset);
	for (;;)
	{
		while (kb_kbhit())
		{
			KeyEvent key = kb_getkey();
			if (key.type == KEY_DOWN)
			{
				selectedPreset++;
				if (!SelectPreset(selectedPreset))selectedPreset--;
			}
			else if (key.type == KEY_UP)
			{
				selectedPreset--;
				if (!SelectPreset(selectedPreset))selectedPreset++;
			}
		}
		ProcessMIDI();
		dtsys.ProcessBlock(dtmidi, numBlockMidiEvents, datl, datr, NumSamples);
		waveIO.PlayAudio(datl, datr, NumSamples);
	}
}
