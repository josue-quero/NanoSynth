//------------------------------------------------------------------------
// Copyright(c) 2022 quero.
//------------------------------------------------------------------------

#ifndef __vst_synth_processor__
#define __vst_synth_processor__

#include "public.sdk/source/vst/vstaudioeffect.h"

#include "vstgui/vstgui.h"
#include "public.sdk/source/vst/vstparameters.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/base/ustring.h"

#include "synthfunctions.h"

#define MAX_VOICES 16
#define OUTPUT_CHANNELS 2 //	stereo only


//	synth objects
#include "WTOscillator.h"
#include "QBLimitedOscillator.h"
#include "LFO.h"

namespace Quero {

//------------------------------------------------------------------------
//  NanoSynthProcessor
//------------------------------------------------------------------------
class NanoSynthProcessor : public Steinberg::Vst::AudioEffect
{
public:
	NanoSynthProcessor ();
	~NanoSynthProcessor () SMTG_OVERRIDE;

    // Create function
	static Steinberg::FUnknown* createInstance (void* /*context*/) 
	{ 
		return (Steinberg::Vst::IAudioProcessor*)new NanoSynthProcessor; 
	}

	//--- ---------------------------------------------------------------------
	// AudioEffect overrides:
	//--- ---------------------------------------------------------------------
	/** Called at first after constructor */
	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
	
	/** Called at the end before destructor */
	Steinberg::tresult PLUGIN_API terminate () SMTG_OVERRIDE;
	
	/** Switch the Plug-in on/off */
	Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) SMTG_OVERRIDE;

	/** Will be called before any process call */
	Steinberg::tresult PLUGIN_API setupProcessing (Steinberg::Vst::ProcessSetup& newSetup) SMTG_OVERRIDE;
	
	/** Asks if a given sample size is supported see SymbolicSampleSizes. */
	Steinberg::tresult PLUGIN_API canProcessSampleSize (Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;

	/** Here we go...the process call */
	Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
		
	/** For persistence */
	Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE;

	//	Define the audio I/O we support
	Steinberg::tresult PLUGIN_API setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns, Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts);

//------------------------------------------------------------------------
protected:
	//	NanoSynth Components

	//	the two oscillators
	QBLimitedOscillator m_Osc1;
	QBLimitedOscillator m_Osc2;

	//	one LFO
	LFO m_LFO1;


	//	updates all voices at once
	void update();

	//	5 GUI Controllers for NanoSynth
	UINT m_uOscWaveform;
	UINT m_uLFO1Waveform;
	double m_dLFO1Rate;
	double m_dLFO1Amplitude;
	UINT m_uLFO1Mode;

	//	functions to reduce size of process()
	bool doControlUpdate(Steinberg::Vst::ProcessData& data);

	//	for MIDI note-on/off
	bool doProcessEvent(Steinberg::Vst::Event& vstEvent);

	//	to load up the samples in new voices
	//bool loadSamples();

	//	for portamento
	double m_dLastNoteFrequency;

	//	MIDI variables
	bool m_bSustainPedal;

	//	MIDI receive channel
	UINT m_uMidiRxChannel;

	//	these are VST3 specific variables for non-note MIDI messages!
	double m_dMIDIPitchBend;
	UINT m_uMIDIModWheel;
	UINT m_uMIDIVolumeCC7;
	UINT m_uMIDIPanCC10;
	UINT m_uMIDIExpressionCC11;

};

//------------------------------------------------------------------------
} // namespace Quero
#endif
