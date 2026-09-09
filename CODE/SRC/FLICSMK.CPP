#include "FlicSmk.hpp"

TFlicSmacker::TFlicSmacker()
{
	fFlicDone = TRUE;
}

TFlicSmacker::~TFlicSmacker()
{
}

int TFlicSmacker::Play(int, int, int, int, bool)
{
	// Report success so the cinematic room advances to normal gameplay.
	fFlicDone = TRUE;
	return 0;
}
