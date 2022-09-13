#include "LFO.h"

//	Initialize the LFO mode variable to sync
//	(waveform is initialized as sine)
CLFO::CLFO(void) {
	m_uLFOMode = sync;
}

CLFO::~CLFO(void) {
}

//	call tghe base class for reset
void CLFO::reset() {
	Oscillator::reset();
}

void CLFO::startOscillator() {
	//	if not in free run mode (one shot or sync'd LFO) then reset 
	if (m_uLFOMode == sync || m_uLFOMode == shot)
		reset();

	//	set flag
	m_bNoteOn = true;
}

//	clears flag
void CLFO::stopOscillator() {
	m_bNoteOn = false;
}

