// Optional Smacker playback facade.
// The restoration build skips movies because the RAD decoder is not present.
#ifndef FLICSMACKER_HPP
#define FLICSMACKER_HPP

#include "Tigre.hpp"

class TFlicSmacker : public TMovable
{
	public:
		TFlicSmacker();
		~TFlicSmacker();
		int Play(int cineNum, int extraBuf = 0, int simSpeed = 0,
			int startFrame = 1, bool skipFrames = TRUE);

		bool fFlicDone;
};

#endif
