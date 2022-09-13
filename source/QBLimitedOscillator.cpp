#include "QBLimitedOscillator.h"

// Everything implemented in the base class
QBLimitedOscillator::QBLimitedOscillator(void) {
}

QBLimitedOscillator::~QBLimitedOscillator(void) {
}

//	Always calls the base class first
void QBLimitedOscillator::reset() {
	Oscillator::reset();

	//	saw/tri starts at 0.5
	if (m_uWaveform == SAW1 || m_uWaveform == SAW2 ||
		m_uWaveform == SAW3 || m_uWaveform == TRI) {
		m_dModulo = 0.5;
	}
}

// Calls reset and sets the note on flag
void QBLimitedOscillator::startOscillator() {
	reset();
	m_bNoteOn = true;
}

//	Clears the note on flag
void QBLimitedOscillator::stopOscillator() {
	m_bNoteOn = false;
}
