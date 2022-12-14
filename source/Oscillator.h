#pragma once
#include "pluginconstants.h"
#include "synthfunctions.h"

#define OSC_FO_MOD_RANGE 2			//	2 semitone default
#define OSC_HARD_SYNC_RATIO_RANGE 4	//
#define OSC_PITCHBEND_MOD_RANGE 12	//	12 semitone default
#define OSC_FO_MIN 20				//	20Hz
#define OSC_FO_MAX 20480			//	20.480kHz = 10 octaves up from 20Hz
#define OSC_FO_DEFAULT 440.0		//	A5
#define OSC_PULSEWIDTH_MIN 2		//	2%
#define OSC_PULSEWIDTH_MAX 98		//	98%
#define OSC_PULSEWIDTH_DEFAULT 50	//	50%

class Oscillator {
public:
	Oscillator(void);
	virtual ~Oscillator(void);

	//	ATTRIBUTES
	//	PUBLIC: these variables may be get/set  
	//	will add function call layer	
	//	oscillator run flag
	bool m_bNoteOn;

	//	user controls or MIDI 
	double m_dOscFo;		//	oscillator frequency from MIDI note number
	double m_dFoRatio;	    //	FM Synth Modulator OR Hard Sync ratio 
	double m_dAmplitude;	//	0->1 from GUI

	//	pulse width in % (sqr only) from GUI
	double m_dPulseWidthControl;

	//	modulo counter and inc for timebase
	double m_dModulo;		//	modulo counter 0->1
	double m_dInc;			//	phase inc = fo/fs

	//	more pitch mods
	int m_nOctave;			//	octave tweak
	int m_nSemitones;		//	semitones tweak
	int m_nCents;			//	cents tweak

	//	for pitched oscillators
	enum { SINE, SAW1, SAW2, SAW3, TRI, SQUARE, NOISE, PNOISE };
	UINT m_uWaveform;	//	to store type

	//	for LFOs
	enum { sine, usaw, dsaw, tri, square, expo, rsh, qrsh };

	//	for LFOs - MODE
	enum { sync, shot, free };
	UINT m_uLFOMode;	//	to store MODE

	//	MIDI note that is being played
	UINT m_uMIDINoteNumber;

protected:
	//	PROTECTED: generally these are either basic calc variables
	//	and modulation stuff
	
	//	calculation variables
	double m_dSampleRate;	//	fs
	double m_dFo;			//	current (actual) frequency of oscillator	
	double m_dPulseWidth;	//	pulse width in % for calculation

	//	for noise and random sample/hold
	UINT   m_uPNRegister;	//	for PN Noise sequence
	int    m_nRSHCounter;	//	random sample/hold counter
	double m_dRSHValue;		//	currnet rsh output

	//	for DPW
	double m_dDPWSquareModulator;	//	square toggle
	double m_dDPW_z1; //	memory register for differentiator

	//	modulation inputs
	double m_dFoMod;			//	modulation input -1 to +1
	double m_dPitchBendMod;	    //	modulation input -1 to +1
	double m_dFoModLin;			//	FM modulation input -1 to +1 (not actually used in Yamaha FM!)
	double m_dPhaseMod;			//	Phase modulation input -1 to +1 (used for DX synth)
	double m_dPWMod;			//	modulation input for PWM -1 to +1
	double m_dAmpMod;			//	output amplitude modulation for AM 0 to +1 (not dB)

public:
	//	FUNCTIONS: all public
	//
	//	modulo functions for master/slave operation
	//	increment the modulo counters
	inline void incModulo() {
		m_dModulo += m_dInc;
	}

	//	check and wrap the modulo
	//	returns true if modulo wrapped
	inline bool checkWrapModulo() {
		//	for positive frequencies
		if (m_dInc > 0 && m_dModulo >= 1.0) {
			m_dModulo -= 1.0;
			return true;
		}
		//	for negative frequencies
		if (m_dInc < 0 && m_dModulo <= 0.0) {
			m_dModulo += 1.0;
			return true;
		}
		return false;
	}

	//	reset the modulo (required for master->slave operations)
	inline void resetModulo(double d = 0.0) { 
		m_dModulo = d;
	}

	//	modulation functions - NOT needed/used if you implement the Modulation Matrix!
	//
	//	output amplitude modulation (AM, not tremolo (dB); 0->1, NOT dB)
	inline void setAmplitudeMod(double dAmp) { 
		m_dAmpMod = dAmp; 
	}

	//	modulation, exponential
	inline void setFoModExp(double dMod) {
		m_dFoMod = dMod;
	}

	inline void setPitchBendMod(double dMod) {
		m_dPitchBendMod = dMod;
	}

	//	for FM only (not used in Yamaha or DX synths)
	inline void setFoModLin(double dMod) {
		m_dFoModLin = dMod;
	}

	//	for Yamaha and DX Synth
	inline void setPhaseMod(double dMod) {
		m_dPhaseMod = dMod;
	}

	//	PWM for square waves only
	inline void setPWMod(double dMod) {
		m_dPWMod = dMod;
	}

	//	VIRTUAL FUNCTIONS
	//
	//	PURE ABSTRACT: derived class MUST implement
	//	start/stop control
	virtual void startOscillator() = 0;
	virtual void stopOscillator() = 0;

	//	render a sample
	//	for LFO:	 pAuxOutput = QuadPhaseOutput
	//	Pitched: pAuxOutput = Right channel (return value is left Channel)
	virtual double doOscillate(double* pAuxOutput = NULL) = 0;

	//	ABSTRACT: derived class overrides if needed
	virtual void setSampleRate(double dFs) {
		m_dSampleRate = dFs;
	}

	//	reset counters, and the others
	virtual void reset();

	//	INLINE FUNCTIONS: these are inlined because they will be 
	//	called every sample period

	//	update the frequency, amp mod and PWM.
	//	it will be called on every sample interval
	inline virtual void update() {
		//	ignore LFO mode for noise sources
		if (m_uWaveform == rsh || m_uWaveform == qrsh) {
			m_uLFOMode = free;
		}

		//	do the  complete frequency mod
		m_dFo = m_dOscFo * m_dFoRatio * pitchShiftMultiplier(m_dFoMod +
			m_dPitchBendMod +
			m_nOctave * 12.0 +
			m_nSemitones +
			m_nCents / 100.0);

		//	apply linear FM (not used in book projects)
		m_dFo += m_dFoModLin;

		//	bound Fo (can go outside for FM/PM mod)
		//	+/- 20480 for FM/PM
		if (m_dFo > OSC_FO_MAX) {
			m_dFo = OSC_FO_MAX;
		}
		if (m_dFo < -OSC_FO_MAX) {
			m_dFo = -OSC_FO_MAX;
		}

		//	calculate increment
		m_dInc = m_dFo / m_dSampleRate;

		//	Pulse Width Modulation
		//	limits are 2% and 98%
		m_dPulseWidth = m_dPulseWidthControl + m_dPWMod * (OSC_PULSEWIDTH_MAX - OSC_PULSEWIDTH_MIN) / OSC_PULSEWIDTH_MIN;

		//	bound the PWM to the range
		m_dPulseWidth = fmin(m_dPulseWidth, OSC_PULSEWIDTH_MAX);
		m_dPulseWidth = fmax(m_dPulseWidth, OSC_PULSEWIDTH_MIN);
	}
};
