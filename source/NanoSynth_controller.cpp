//------------------------------------------------------------------------
// Copyright(c) 2022 quero.
//------------------------------------------------------------------------

#include "NanoSynth_controller.h"
#include "NanoSynth_cids.h"
#include "vstgui/plugin-bindings/vst3editor.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/futils.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "base/source/fstring.h"
#include "logscale.h"
#include "base/source/fstreamer.h"
#include "synthfunctions.h"
#include "synthparamlimits.h"

using namespace Steinberg;

namespace Quero {

// this defines a logarithmig scaling for the filter Fc control
Vst::LogScale<Vst::ParamValue> filterLogScale(MIN_UNIPOLAR,	/* VST GUI Variable MIN (0) */
	MAX_UNIPOLAR,	/* VST GUI Variable MAX (1) */
	MIN_FILTER_FC,	/* filter fc LOW */
	MAX_FILTER_FC,	/* filter fc HIGH */
	FILTER_RAW_MAP,/* the value at position 0.5 will be: */
	FILTER_COOKED_MAP);/* 1800 Hz */

//------------------------------------------------------------------------
// NanoSynthController Implementation
//------------------------------------------------------------------------
/*
	Controller::initialize()

	This is where we describe the Default GUI controls and link them to control IDs.
	This must be done if you want a Custom GUI too so you have to implement this no matter what.
*/
tresult PLUGIN_API NanoSynthController::initialize (FUnknown* context)
{
	// Here the Plug-in will be instanciated

	//	do not forget to call parent
	tresult result = EditControllerEx1::initialize (context);
	if (result == kResultOk)
	{
		//	Init parameters
		Vst::Parameter* param;

		// MIDI Params - these have no knobs in main GUI but do have to appear in default
		// NOTE: this is for VST3 ONLY!
		param = new Vst::RangeParameter(USTRING("PitchBend"), MIDI_PITCHBEND, USTRING(""),
			MIN_UNIPOLAR, MAX_UNIPOLAR, DEFAULT_UNIPOLAR);
		param->setPrecision(1); // fractional sig digits
		parameters.addParameter(param);

		param = new Vst::RangeParameter(USTRING("MIDI Vol"), MIDI_VOLUME_CC7, USTRING(""),
			MIN_UNIPOLAR, MAX_UNIPOLAR, DEFAULT_UNIPOLAR);
		param->setPrecision(1); // fractional sig digits
		parameters.addParameter(param);

		param = new Vst::RangeParameter(USTRING("MIDI Pan"), MIDI_PAN_CC10, USTRING(""),
			MIN_UNIPOLAR, MAX_UNIPOLAR, DEFAULT_UNIPOLAR);
		param->setPrecision(1); // fractional sig digits
		parameters.addParameter(param);

		param = new Vst::RangeParameter(USTRING("MIDI Mod Wheel"), MIDI_MODWHEEL, USTRING(""),
			MIN_UNIPOLAR, MAX_UNIPOLAR, DEFAULT_UNIPOLAR);
		param->setPrecision(1); // fractional sig digits
		parameters.addParameter(param);

		param = new Vst::RangeParameter(USTRING("MIDI Expression"), MIDI_EXPRESSION_CC11, USTRING(""),
			MIN_UNIPOLAR, MAX_UNIPOLAR, DEFAULT_UNIPOLAR);
		param->setPrecision(1); // fractional sig digits
		parameters.addParameter(param);

		param = new Vst::RangeParameter(USTRING("MIDI Channel Pressure"), MIDI_CHANNEL_PRESSURE, USTRING(""),
			MIN_UNIPOLAR, MAX_UNIPOLAR, DEFAULT_UNIPOLAR);
		param->setPrecision(1); // fractional sig digits
		parameters.addParameter(param);

		param = new Vst::RangeParameter(USTRING("MIDI Sustain Pedal"), MIDI_SUSTAIN_PEDAL, USTRING(""),
			MIN_UNIPOLAR, MAX_UNIPOLAR, DEFAULT_UNIPOLAR);
		param->setPrecision(1); // fractional sig digits
		parameters.addParameter(param);

		param = new Vst::RangeParameter(USTRING("All Notes Off"), MIDI_ALL_NOTES_OFF, USTRING(""),
			MIN_UNIPOLAR, MAX_UNIPOLAR, DEFAULT_UNIPOLAR);
		param->setPrecision(1); // fractional sig digits
		parameters.addParameter(param);
	}

	// Here you could register some parameters

	return kResultOk;
}

//------------------------------------------------------------------------
/*
	Controller::terminate()
	The End.
*/
tresult PLUGIN_API NanoSynthController::terminate ()
{
	// Here the Plug-in will be de-instanciated, last possibility to remove some memory!

	//---do not forget to call parent ------
	return EditControllerEx1::terminate ();
}

/*
	Controller::setParamNormalizedFromFile()
	This sets the normalized (raw) parameter value
	from the file's cooked (plain) value

	tag - the Index value of a control
	value - the cooked value from the file
*/
tresult PLUGIN_API NanoSynthController::setParamNormalizedFromFile(Vst::ParamID tag, Vst::ParamValue value)
{
	//	get the parameter
	Vst::Parameter* pParam = EditController::getParameterObject(tag);

	//	verify pointer
	if (!pParam) {
		return kResultFalse;
	}

	//	convert serialized value to normalized (raw)
	return setParamNormalized(tag, pParam->toNormalized(value));
}

//------------------------------------------------------------------------
/*
	Controller::setComponentState()
	This is the serialization-read function so the GUI can
	be updated from a preset or startup.

	fileStream - the IBStream interface from the client
*/
tresult PLUGIN_API NanoSynthController::setComponentState (IBStream* state)
{
	//	make a streamer interface using the 
	//  IBStream* fileStream; this is for PC so
	//  data is LittleEndian
	IBStreamer stream(state, kLittleEndian);

	//	variables for reading
	uint64 version = 0;
	double dDoubleParam = 0;

	//	needed to convert to our UINT reads
	uint32 udata = 0;
	int32 data = 0;

	//	read the version
	if (!stream.readInt64u(version)) {
		return kResultFalse;
	}

	//	do next version...
	if (version >= 1) {
		//	add v1 stuff here
	}

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API NanoSynthController::setState (IBStream* state)
{
	// Here you get the state of the controller

	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API NanoSynthController::getState (IBStream* state)
{
	// Here you are asked to deliver the state of the controller (if needed)
	// Note: the real state of your plug-in is saved in the processor

	return kResultTrue;
}

/* --- IMIDIMapping Interface
	Controller::getMidiControllerAssignment()

	The client queries this 129 times for 130 possible control messages, see ivstsmidicontrollers.h for
	the VST defines for kPitchBend, kCtrlModWheel, etc... for each MIDI Channel in our Event Bus

	We respond with our ControlID value that we'll use to process the MIDI Messages in Processor::process().

	On the default GUI, these controls will actually move with the MIDI messages, but we don't want that on
	the final UI so that we can have any Modulation Matrix mapping we want.
*/
tresult PLUGIN_API NanoSynthController::getMidiControllerAssignment(int32 busIndex, int16 channel, Vst::CtrlNumber midiControllerNumber, Vst::ParamID& id/*out*/)
{
	// NOTE: we only have one EventBus(0)
	//       but it has 16 channels on it
	if (busIndex == 0) {
		id = -1;
		switch (midiControllerNumber) {
			// these messages handled in the Processor::process() method
		case Vst::kPitchBend:
			id = MIDI_PITCHBEND;
			break;
		case Vst::kCtrlModWheel:
			id = MIDI_MODWHEEL;
			break;
		case Vst::kCtrlVolume:
			id = MIDI_VOLUME_CC7;
			break;
		case Vst::kCtrlPan:
			id = MIDI_PAN_CC10;
			break;
		case Vst::kCtrlExpression:
			id = MIDI_EXPRESSION_CC11;
			break;
		case Vst::kAfterTouch:
			id = MIDI_CHANNEL_PRESSURE;
			break;
		case Vst::kCtrlSustainOnOff:
			id = MIDI_SUSTAIN_PEDAL;
			break;
		case Vst::kCtrlAllNotesOff:
			id = MIDI_ALL_NOTES_OFF;
			break;
		}
		if (id >= 0)
		{
			return kResultOk;
		}
		else
			return kResultFalse;

	}

	return kResultFalse;
}

//------------------------------------------------------------------------
/*
	Controller::createView()
	Create our VSTGUI editor from the UIDESC file.
*/
IPlugView* PLUGIN_API NanoSynthController::createView (FIDString name)
{
	// Here the Host wants to open your editor (if you have one)
	if (FIDStringsEqual (name, Vst::ViewType::kEditor)) {
		//	create the editor using the .uidesc file (in our Resources, edited with WYSIWYG editor in client)
		auto* view = new VSTGUI::VST3Editor (this, "Editor", "NanoSynth_editor.uidesc");
		return view;
	}
	return nullptr;
}

//------------------------------------------------------------------------
tresult PLUGIN_API NanoSynthController::setParamNormalized (Vst::ParamID tag, Vst::ParamValue value)
{
	// called by host to update your parameters
	tresult result = EditControllerEx1::setParamNormalized (tag, value);
	return result;
}

//------------------------------------------------------------------------
tresult PLUGIN_API NanoSynthController::getParamStringByValue (Vst::ParamID tag, Vst::ParamValue valueNormalized, Vst::String128 string)
{
	// called by host to get a string for given normalized value of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamStringByValue (tag, valueNormalized, string);
}

//------------------------------------------------------------------------
tresult PLUGIN_API NanoSynthController::getParamValueByString (Vst::ParamID tag, Vst::TChar* string, Vst::ParamValue& valueNormalized)
{
	// called by host to get a normalized value from a string representation of a specific parameter
	// (without having to set the value!)
	return EditControllerEx1::getParamValueByString (tag, string, valueNormalized);
}

/* See Automation in the docs
	Non-linear Scaling
	If the DSP representation of a value does not scale in a linear way to the exported
	normalized representation (which can happen when a decibel scale is used for example),
	the edit controller must provide a conversion to a plain representation.
	This allows the host to move automation data (being in GUI representation)
	and keep the original value relations intact.
	(Steinberg::Vst::IEditController::normalizedParamToPlain / Steinberg::Vst::IEditController::plainParamToNormalized).

	*** NOTE ***
	We do not use these since our controls are linear or logscale-controlled.
	I am just leaving them here in case you need to implement them. See docs.
*/
Vst::ParamValue PLUGIN_API NanoSynthController::plainParamToNormalized(Vst::ParamID tag, Vst::ParamValue plainValue)
{
	return EditController::plainParamToNormalized(tag, plainValue);
}
Vst::ParamValue PLUGIN_API NanoSynthController::normalizedParamToPlain(Vst::ParamID tag, Vst::ParamValue valueNormalized)
{
	return EditController::normalizedParamToPlain(tag, valueNormalized);
}

//------------------------------------------------------------------------
} // namespace Quero
