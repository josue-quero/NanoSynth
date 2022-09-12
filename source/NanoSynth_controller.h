//------------------------------------------------------------------------
// Copyright(c) 2022 quero.
//------------------------------------------------------------------------

#ifndef __vst_synth_controller__
#define __vst_synth_controller__

#include "public.sdk/source/vst/vsteditcontroller.h"
#include "pluginterfaces/vst/ivstnoteexpression.h"
#include "vstgui/plugin-bindings/vst3editor.h"


namespace Quero {

//------------------------------------------------------------------------
//  NanoSynthController
//------------------------------------------------------------------------
/*
	The Controller handles the GUI and MIDI Mapping details
	In VST3, all non-note messages (ie everything BUT noteOn and noteOff) are
	handled via a controller mapping. In some cases, create dummy
	controls that are not intended for the final GUI so that I can get these
	messages. WP

	NOTE: Multiple Inheriance
		  EditController - the base controller stuff
		  IMidiMapping - the MIDI Mapping Interface allowing us to RX MIDI

*/
class NanoSynthController : public Steinberg::Vst::EditControllerEx1, public Steinberg::Vst::IMidiMapping
{
public:
//------------------------------------------------------------------------
	NanoSynthController () = default;
	~NanoSynthController () SMTG_OVERRIDE = default;

    // Create function
	static Steinberg::FUnknown* createInstance (void* /*context*/)
	{
		return (Steinberg::Vst::IEditController*)new NanoSynthController;
	}

	// IPluginBase
	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API terminate () SMTG_OVERRIDE;

	// EditController
	Steinberg::tresult PLUGIN_API setComponentState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::IPlugView* PLUGIN_API createView (Steinberg::FIDString name) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setParamNormalized (Steinberg::Vst::ParamID tag,
                                                      Steinberg::Vst::ParamValue value) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getParamStringByValue (Steinberg::Vst::ParamID tag,
                                                         Steinberg::Vst::ParamValue valueNormalized,
                                                         Steinberg::Vst::String128 string) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getParamValueByString (Steinberg::Vst::ParamID tag,
                                                         Steinberg::Vst::TChar* string,
                                                         Steinberg::Vst::ParamValue& valueNormalized) SMTG_OVERRIDE;

	//	IMidiMapping
	virtual Steinberg::tresult PLUGIN_API getMidiControllerAssignment(Steinberg::int32 busIndex, Steinberg::int16 channel, Steinberg::Vst::CtrlNumber midiControllerNumber, Steinberg::Vst::ParamID& id/*out*/);

	Steinberg::tresult PLUGIN_API setParamNormalizedFromFile(Steinberg::Vst::ParamID tag, Steinberg::Vst::ParamValue value);

	//	oridinarily not needed; see documentation on Automation for using these
	virtual Steinberg::Vst::ParamValue PLUGIN_API normalizedParamToPlain(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue valueNormalized);
	virtual Steinberg::Vst::ParamValue PLUGIN_API plainParamToNormalized(Steinberg::Vst::ParamID id, Steinberg::Vst::ParamValue plainValue);

	
	//	define the controller and interface
	OBJ_METHODS(NanoSynthController, EditControllerEx1)
	DEFINE_INTERFACES
		DEF_INTERFACE(IMidiMapping)
	END_DEFINE_INTERFACES(EditControllerEx1)
	REFCOUNT_METHODS(EditControllerEx1)

};

//------------------------------------------------------------------------
} // namespace Quero
#endif