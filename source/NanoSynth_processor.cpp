//------------------------------------------------------------------------
// Copyright(c) 2022 quero.
//------------------------------------------------------------------------

#include "NanoSynth_processor.h"
#include "NanoSynth_controller.h"
#include "NanoSynth_cids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ftypes.h"
#include "pluginterfaces/base/futils.h"
#include "logscale.h"
#include "SynthParamLimits.h"

//	Synth Stuff
#define SYNTH_PROC_BLOCKSIZE 32 // 32 samples per processing block = 0.7 mSec = OK for tactile response WP

//	MIDI Logging -- comment out to disable
#define LOG_MIDI 1

using namespace Steinberg;

namespace Quero {
//	for versioning in serialization
static uint64 NanoSynthVersion = 0;

//	this defines a logarithmig scaling for the filter Fc control
Vst::LogScale<Vst::ParamValue> filterLogScale2(0.0,		/* VST GUI Variable MIN */
	1.0,		/* VST GUI Variable MAX */
	80.0,		/* filter fc LOW */
	18000.0,	/* filter fc HIGH */
	0.5,		/* the value at position 0.5 will be: */
	1800.0);	/* 1800 Hz */


//------------------------------------------------------------------------
// NanoSynthProcessor
//------------------------------------------------------------------------
NanoSynthProcessor::NanoSynthProcessor ()
{
	//	set the wanted controller for our processor
	setControllerClass (kNanoSynthControllerUID);
	
	//	Finish initializations here

	//	GUI Controls
	m_uOscWaveform = DEFAULT_PITCHED_OSC_WAVEFORM;
	m_uLFO1Waveform = DEFAULT_LFO_WAVEFORM;
	m_dLFO1Rate = DEFAULT_LFO_RATE;
	m_dLFO1Amplitude = DEFAULT_UNIPOLAR;
	m_uLFO1Mode = DEFAULT_LFO_MODE;

	m_dLastNoteFrequency = 0.0;

	//	sus pedal support
	m_bSustainPedal = false;

	//	receive on all channels
	m_uMidiRxChannel = MIDI_CH_ALL;

	// VST3 specific
	m_dMIDIPitchBend = DEFAULT_MIDI_PITCHBEND; // -1 to +1
	m_uMIDIModWheel = DEFAULT_MIDI_MODWHEEL;
	m_uMIDIVolumeCC7 = DEFAULT_MIDI_VOLUME;  // note defaults to 127
	m_uMIDIPanCC10 = DEFAULT_MIDI_PAN;     // 64 = center pan
	m_uMIDIExpressionCC11 = DEFAULT_MIDI_EXPRESSION;
}

//------------------------------------------------------------------------
NanoSynthProcessor::~NanoSynthProcessor ()
{}

//------------------------------------------------------------------------
/*
	Processor::initialize()
	Call the base class
	Add a Stereo Audio Output
	Add one Event Bus with 16 Channels
*/
tresult PLUGIN_API NanoSynthProcessor::initialize (FUnknown* context)
{
	// Here the Plug-in will be instanciated
	
	//---always initialize the parent-------
	tresult result = AudioEffect::initialize (context);
	// if everything Ok, continue
	if (result != kResultOk)
	{
		return result;
	}

	//--- create Audio IO ------
	//addAudioInput (STR16 ("Stereo In"), Steinberg::Vst::SpeakerArr::kStereo);
	addAudioOutput (STR16 ("Stereo Output"), Steinberg::Vst::SpeakerArr::kStereo);

	//	MIDI event input bus, 16 channels
	addEventInput (STR16 ("Event Input"), 16);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API NanoSynthProcessor::terminate ()
{
	// Here the Plug-in will be de-instanciated, last possibility to remove some memory!
	
	//---do not forget to call parent ------
	return AudioEffect::terminate ();
}

//------------------------------------------------------------------------
/*
	Processor::setActive()
	This is the analog of prepareForPlay() in RAFX since the Sample Rate is now set.

	VST3 plugins may be turned on or off; supposidly it should be dynamically delared when activated
	then deleted when de-activated. So,voice stacks are handled here.
*/
tresult PLUGIN_API NanoSynthProcessor::setActive (TBool state)
{
	if (state) {
		//	do ON stuff
		// 
		// 
		//	set sample rates
		m_Osc1.setSampleRate((double)processSetup.sampleRate);
		m_Osc2.setSampleRate((double)processSetup.sampleRate);
		m_LFO1.setSampleRate((double)processSetup.sampleRate);

		//	detune
		m_Osc2.m_nCents = 2.5; // +2.5 cents detuned

		//	update all
		update();
	} else {
		//	do OFF stuff
	}

	//--- called when the Plug-in is enable/disable (On/Off) -----
	return AudioEffect::setActive (state);
}

/*
	Processor::update()
	Custom function to update the voice(s) of the synth with UI Changes.
*/
void NanoSynthProcessor::update()
{
	//	Connection of the GUI controls to the synth
	//	transfering the GUI control variables over to the synth objects
	m_Osc1.m_uWaveform = m_uOscWaveform;
	m_Osc2.m_uWaveform = m_uOscWaveform;
	m_Osc1.update();
	m_Osc2.update();

	m_LFO1.m_uWaveform = m_uLFO1Waveform;
	m_LFO1.m_dAmplitude = m_dLFO1Amplitude;
	m_LFO1.m_dOscFo = m_dLFO1Rate;
	m_LFO1.m_uLFOMode = m_uLFO1Mode;
	m_LFO1.update();
}

/*
	Processor::doControlUpdate()
	Find and issue Control Changes (same as userInterfaceChange() in RAFX)
	returns true if a control was changed
*/
bool NanoSynthProcessor::doControlUpdate(Steinberg::Vst::ProcessData& data)
{
	bool paramChange = false;

	//	check
	if (!data.inputParameterChanges) {
		return paramChange;
	}

	//	get the param count and setup a loop for processing queue data
	int32 count = data.inputParameterChanges->getParameterCount();

	//	make sure there is something there
	if (count <= 0) {
		return paramChange;
	}

	//	loop
	for (int32 i = 0; i < count; i++) {
		#if(LOG_MIDI && _DEBUG)
			FDebugPrint("Something changed\n");
		#endif
		//	get the message queue for ith parameter
		Vst::IParamValueQueue* queue = data.inputParameterChanges->getParameterData(i);

		if (queue) {
			#if(LOG_MIDI && _DEBUG)
						FDebugPrint("Inside queue\n");
			#endif
			//	check for control points
			if (queue->getPointCount() <= 0) {
				return false;
			}
			int32 sampleOffset = 0.0;
			Vst::ParamValue value = 0.0;
			Vst::ParamID pid = queue->getParameterId();

			//	this is the same as userInterfaceChange(); these only are updated if a change has 
			//	occurred (a control got moved)
			//
			// NOTE: taking the last value in the queue: queue->getPointCount()-1
			//       the value parameter is [0..1] so MUST BE COOKED before using
			//
			// NOTE: These are NOT MIDI Events! Not possible to get the channel directly

			//	get the last point in queue
			if (queue->getPoint(queue->getPointCount() - 1, /* last update point */
				sampleOffset,			/* sample offset */
				value) == kResultTrue)	/* value = [0..1] */
			{
				//	at least one param changed
				paramChange = true;

				switch (pid) {
					//	GUI control code
					case OSC_WAVEFORM: {
						m_uOscWaveform = (UINT)cookVSTGUIVariable(MIN_PITCHED_OSC_WAVEFORM, MAX_PITCHED_OSC_WAVEFORM, value);
						break;
					}

					case LFO1_WAVEFORM: {
						m_uLFO1Waveform = (UINT)cookVSTGUIVariable(MIN_LFO_WAVEFORM, MAX_LFO_WAVEFORM, value);
						break;
					}

					case LFO1_RATE: {
						m_dLFO1Rate = cookVSTGUIVariable(MIN_LFO_RATE, MAX_LFO_RATE, value);
						break;
					}

					case LFO1_AMPLITUDE: {
						m_dLFO1Amplitude = cookVSTGUIVariable(MIN_UNIPOLAR, MAX_UNIPOLAR, value);
						break;
					}

					case LFO1_MODE: {
						m_uLFO1Mode = cookVSTGUIVariable(MIN_LFO_MODE, MAX_LFO_MODE, value);
						break;
					}

					//	MIDI messages
					//	want -1 to +1
					case MIDI_PITCHBEND: {
						m_dMIDIPitchBend = unipolarToBipolar(value);
						#if(LOG_MIDI && _DEBUG)
							FDebugPrint("Pitch Bend: %f\n", m_dMIDIPitchBend);
						#endif
						break;
					}
					//	want 0 to 127
					case MIDI_MODWHEEL: {
						m_uMIDIModWheel = unipolarToMIDI(value);
						#if(LOG_MIDI && _DEBUG)
							FDebugPrint("Mod Wheel: %d\n", m_uMIDIModWheel);
						#endif
						break;
					}
					//	want 0 to 127
					case MIDI_VOLUME_CC7: {
						m_uMIDIVolumeCC7 = unipolarToMIDI(value);
						#if(LOG_MIDI && _DEBUG)
							FDebugPrint("Volume: %d\n", m_uMIDIVolumeCC7);
						#endif
						break;
					}
					//	want 0 to 127
					case MIDI_PAN_CC10: {
						m_uMIDIPanCC10 = unipolarToMIDI(value);
						#if(LOG_MIDI && _DEBUG)
							FDebugPrint("Pan: %d\n", m_uMIDIPanCC10);
						#endif
						break;
					}
					//	want 0 to 127
					case MIDI_EXPRESSION_CC11: {
						m_uMIDIExpressionCC11 = unipolarToMIDI(value);
						#if(LOG_MIDI && _DEBUG)
							FDebugPrint("Expression: %d\n", m_uMIDIExpressionCC11);
						#endif
						break;
					}
					case MIDI_CHANNEL_PRESSURE: {
						#if(LOG_MIDI && _DEBUG)
							FDebugPrint("Channel Pressure: %f\n", value);
						#endif
						break;
					}
					// want 0 to 1
					case MIDI_SUSTAIN_PEDAL: {
						m_bSustainPedal = value > 0.5 ? true : false;
						#if(LOG_MIDI && _DEBUG)
							if (m_bSustainPedal) {
								FDebugPrint("Sustain Pedal ON\n");
							} else {
								FDebugPrint("Sustain Pedal OFF\n");
							}	
						#endif
						break;
					}
					case MIDI_ALL_NOTES_OFF: {
						#if(LOG_MIDI && _DEBUG)
							FDebugPrint("All Notes OFF\n");
						#endif
						break;
					}
				}
			}
		}
	}

	//	check and update
	if (paramChange) {
		update();
	}

	return paramChange;
}

bool NanoSynthProcessor::doProcessEvent(Vst::Event& vstEvent)
{
	bool noteEvent = false;

	//	process Note On or Note Off messages here
	switch (vstEvent.type) {
		//	NOTE ON
		case Vst::Event::kNoteOnEvent: {
			//	get the channel/note/vel
			UINT uMIDIChannel = (UINT)vstEvent.noteOn.channel;
			UINT uMIDINote = (UINT)vstEvent.noteOn.pitch;
			UINT uMIDIVelocity = (UINT)(127.0 * vstEvent.noteOn.velocity);

			//	test channel/ignore
			if (m_uMidiRxChannel != MIDI_CH_ALL && uMIDIChannel != m_uMidiRxChannel) {
				return false;
			}

			//	event occurred
			noteEvent = true;

			//	fix noteID as per SDK
			if (vstEvent.noteOn.noteId == -1) {
				vstEvent.noteOn.noteId = uMIDINote;
			}

			#if(LOG_MIDI && _DEBUG)
				FDebugPrint("Note ON: Channel: %d, Note: %d, Velocity: %d\n", uMIDIChannel, uMIDINote, uMIDIVelocity);
			#endif

			m_Osc1.m_dOscFo = midiFreqTable[uMIDINote];
			m_Osc1.update();

			m_Osc2.m_dOscFo = midiFreqTable[uMIDINote];
			m_Osc2.update();

			m_Osc1.startOscillator();
			m_Osc2.startOscillator();
			m_LFO1.startOscillator();

			break;
		}

		//	NOTE OFF
		case Vst::Event::kNoteOffEvent: {

			//	1get the channel/note/vel
			UINT uMIDIChannel = (UINT)vstEvent.noteOff.channel;
			UINT uMIDINote = (UINT)vstEvent.noteOff.pitch;
			UINT uMIDIVelocity = (UINT)(127.0 * vstEvent.noteOff.velocity); // not used

			//	test channel/ignore
			if (m_uMidiRxChannel != MIDI_CH_ALL && uMIDIChannel != m_uMidiRxChannel) {
				return false;
			}

			//	event occurred
			noteEvent = true;

			//	fix noteID as per SDK
			if (vstEvent.noteOff.noteId == -1) {
				vstEvent.noteOff.noteId = uMIDINote;
			}

			#if(LOG_MIDI && _DEBUG)
				FDebugPrint("Note OFF: Channel: %d, Note: %d, Velocity: %d\n", uMIDIChannel, uMIDINote, uMIDIVelocity);
			#endif

			m_Osc1.stopOscillator();
			m_Osc2.stopOscillator();
			m_LFO1.stopOscillator();
			break;
		}

		//	polyphonic aftertouch 0xAn
		case Vst::Event::kPolyPressureEvent: {
			//	get the channel
			UINT uMIDIChannel = (UINT)vstEvent.polyPressure.channel;
			UINT uMIDINote = (UINT)vstEvent.polyPressure.pitch;
			float fPressure = vstEvent.polyPressure.pressure;

			//	test channel/ignore
			if (m_uMidiRxChannel != MIDI_CH_ALL && uMIDIChannel != m_uMidiRxChannel) {
				return false;
			}

			//	note event did not occurr
			noteEvent = false;

			#if(LOG_MIDI && _DEBUG)
				FDebugPrint("Poly Pressure: Channel: %d, Note: %d, Pressure: %f\n", uMIDIChannel, uMIDINote, fPressure);
			#endif

			break;
		}
	}

	//	note event occurred?
	return noteEvent;
}


//------------------------------------------------------------------------
/*
	Processor::process()
	The most important function handles:
		Control Changes
		Synth voice rendering
		Output GUI Changes (allows you to write back to the GUI)
*/
tresult PLUGIN_API NanoSynthProcessor::process (Vst::ProcessData& data)
{
	//	check for presence of output buffers
	if (!data.outputs) {
		return kResultOk;
	}

	//	check for control chages and update synth if needed
	doControlUpdate(data);

	//	flush mode
	if (data.numOutputs < 1) {
		return kResultTrue;
	} else {
		//	process 32 samples at a time; MIDI events are then accurate to 0.7 mSec
		const int32 kBlockSize = SYNTH_PROC_BLOCKSIZE;

		//	32-bit is float
		//	if doing a 64-bit version, it is replaced with double*
		//	initialize audio output buffers
		float* buffers[OUTPUT_CHANNELS]; //	Precision is float - need to change this do DOUBLE if supporting 64 bit

		//	32-bit is float
		//	if doing a 64-bit version, it is replaced with double* here too
		for (int i = 0; i < OUTPUT_CHANNELS; i++) {
			//	data.outputs[0] = BUS 0
			buffers[i] = (float*)data.outputs[0].channelBuffers32[i];
			memset(buffers[i], 0, data.numSamples * sizeof(float));
		}

		//	total number of samples in the input Buffer
		int32 numSamples = data.numSamples;

		//	this is used when shoving an event into the next block 
		int32 samplesProcessed = 0;

		//	get list of events
		Vst::IEventList* inputEvents = data.inputEvents;
		Vst::Event e = { 0 };
		Vst::Event* eventPtr = 0;
		int32 eventIndex = 0;

		//	count of events
		int32 numEvents = inputEvents ? inputEvents->getEventCount() : 0;

		//	get the first event
		if (numEvents)	{
			inputEvents->getEvent(0, e);
			eventPtr = &e;
		}

		while (numSamples > 0)	{
			//	bound the samples to process to BLOCK SIZE (32)
			int32 samplesToProcess = std::min<int32>(kBlockSize, numSamples);

			while (eventPtr != 0)	{
				//	if the event is not in the current processing block 
				//  then adapt offset for next block
				if (e.sampleOffset > samplesToProcess)	{
					e.sampleOffset -= samplesToProcess;
					break;
				}

				//	find MIDI note-on/off and broadcast
				doProcessEvent(e);

				//	get next event
				eventIndex++;
				if (eventIndex < numEvents) {
					if (inputEvents->getEvent(eventIndex, e) == kResultTrue) {
						e.sampleOffset -= samplesProcessed;
					} else {
						eventPtr = 0;
					}
				} else {
					eventPtr = 0;
				}
			}

			//	output "accumulator"
			double dOut = 0.0;

			//	the loop - samplesToProcess is more like framesToProcess
			for (int32 j = 0; j < samplesToProcess; j++) {
				dOut = 0.0;

				if (m_Osc1.m_bNoteOn) {
					//	ARTICULATION BLOCK
					//	This is a mono synth so it's only necessary to 
					//	calculate one output per sample period
					//
					//	render LFO output
					double dLFO1Out = m_LFO1.doOscillate();

					//	apply to the Exp modulation inputs
					m_Osc1.setFoModExp(dLFO1Out * OSC_FO_MOD_RANGE);
					m_Osc2.setFoModExp(dLFO1Out * OSC_FO_MOD_RANGE);

					//	update 
					m_Osc1.update();
					m_Osc2.update();

					//	DIGITAL AUDIO ENGINE BLOCK
					dOut = 0.5 * m_Osc1.doOscillate() + 0.5 * m_Osc2.doOscillate();
				}

				//	write to buffers
				buffers[0][j] = dOut;	// left
				buffers[1][j] = dOut;	// right
			}

			//	update the counter
			for (int i = 0; i < OUTPUT_CHANNELS; i++) {
				buffers[i] += samplesToProcess;
			}

			//	update the samples processed/to process
			numSamples -= samplesToProcess;
			samplesProcessed += samplesToProcess;

		} //	end while (numSamples > 0)
	}

	//	can write OUT to the GUI like this:
	if (data.outputParameterChanges) {

	}
	//	set silence flags if no notes playing
	if (data.numOutputs > 0) {
		data.outputs[0].silenceFlags = 0x11; // left and right channel are silent
	}

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API NanoSynthProcessor::setupProcessing (Vst::ProcessSetup& newSetup)
{
	//--- called before any processing ----
	return AudioEffect::setupProcessing (newSetup);
}

//------------------------------------------------------------------------
/*
	Processor::canProcessSampleSize()
	Client queries us for our supported sample lengths
*/
tresult PLUGIN_API NanoSynthProcessor::canProcessSampleSize (int32 symbolicSampleSize)
{
	// by default kSample32 is supported
	if (symbolicSampleSize == Vst::kSample32) {
		return kResultTrue;
	}
	// disable the following comment if your processing support kSample64
	/* if (symbolicSampleSize == Vst::kSample64)
		return kResultTrue; */

	return kResultFalse;
}

//------------------------------------------------------------------------
/*
	Processor::setState()
	This is the READ part of the serialization process. We get the stream interface and use it
	to read from the filestream.

	NOTE: The datatypes/read order must EXACTLY match the getState() version or crashes may happen or variables
		  not initialized properly.
*/
tresult PLUGIN_API NanoSynthProcessor::setState (IBStream* state)
{
	//	called when we load a preset, the model has to be reloaded
	IBStreamer streamer (state, kLittleEndian);
	uint64 version = 0;

	//	needed to convert to UINT reads
	uint32 udata = 0;
	int32 data = 0;

	//	read the version
	if (!streamer.readInt64u(version)) {
		return kResultFalse;
	}

	//	read the current GUI parameters
	//  NOTE: the reading and writting must happen in the same order
	if (!streamer.readInt32u(udata)) {
		return kResultFalse;
	} else {
		m_uOscWaveform = udata;
	}
	if (!streamer.readInt32u(udata)) {
		return kResultFalse;
	} else {
		m_uLFO1Waveform = udata;
	}
	if (!streamer.readDouble(m_dLFO1Rate)) {
		return kResultFalse;
	}
	if (!streamer.readDouble(m_dLFO1Amplitude)) {
		return kResultFalse;
	}
	if (!streamer.readInt32u(udata)) {
		return kResultFalse;
	} else {
		m_uLFO1Mode = udata;
	}

	//	do next version...
	if (version >= 1)
	{
		// add v1 stuff here
	}
	
	return kResultOk;
}

//------------------------------------------------------------------------
/*
	Processor::getState()
	This is the WRITE part of the serialization process. It gets the stream interface and uses it
	to write to the filestream. This is important because it is how the Factory Default is set
	at startup, as well as when writing presets.
*/
tresult PLUGIN_API NanoSynthProcessor::getState (IBStream* state)
{
	// here we need to save the model
	IBStreamer streamer (state, kLittleEndian);

	//	NanoSynthVersion - place this at top so versioning can be used during the READ operation
	if (!streamer.writeInt64u(NanoSynthVersion)) {
		return kResultFalse;
	}

	//	save the current GUI control variables
	if (!streamer.writeInt32u(m_uOscWaveform)) {
		return kResultFalse;
	}
	if (!streamer.writeInt32u(m_uLFO1Waveform)) {
		return kResultFalse;
	}
	if (!streamer.writeDouble(m_dLFO1Rate)) {
		return kResultFalse;
	}
	if (!streamer.writeDouble(m_dLFO1Amplitude)) {
		return kResultFalse;
	}
	if (!streamer.writeInt32u(m_uLFO1Mode)) {
		return kResultFalse;
	}

	return kResultOk;
}

/*
	Processor::setBusArrangements()
	Client queries us for our supported Busses; this is where you can modify to support mono, surround, etc...
*/
tresult PLUGIN_API NanoSynthProcessor::setBusArrangements(Vst::SpeakerArrangement* inputs, int32 numIns,
	Vst::SpeakerArrangement* outputs, int32 numOuts)
{
	//	only support one stereo output bus
	if (numIns == 0 && numOuts == 1 && outputs[0] == Steinberg::Vst::SpeakerArr::kStereo)	{
		return AudioEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
	}
	return kResultFalse;
}

//------------------------------------------------------------------------
} // namespace Quero
