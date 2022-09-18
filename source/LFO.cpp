#include "LFO.h"

//	Initialize the LFO mode variable to sync
//	(waveform is initialized as sine)
LFO::LFO(void) {
	m_uLFOMode = sync;
}

LFO::~LFO(void) {
}

//	call tghe base class for reset
void LFO::reset() {
	Oscillator::reset();
}

void LFO::startOscillator() {
	//	if not in free run mode (one shot or sync'd LFO) then reset 
	if (m_uLFOMode == sync || m_uLFOMode == shot)
		reset();

	//	set flag
	m_bNoteOn = true;
}

//	clears flag
void LFO::stopOscillator() {
	m_bNoteOn = false;
}

