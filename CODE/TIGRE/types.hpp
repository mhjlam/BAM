//
// TYPES.HPP
//
// September 22, 1994
// (C) 1993, Tachyon, Inc.  All rights reserved.
//
//	Type definitions.
//
//----[]-------------------------------------------------------------


#ifndef types_hpp
#define types_hpp

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "res_cfg.hpp"

// Standard typedefs — aliases to <stdint.h> to make platform expectations explicit

typedef int8_t			int8;
typedef uint8_t			uint8;
typedef int16_t			int16;
typedef uint16_t		uint16;
typedef int32_t			int32;
typedef unsigned int	uint;
typedef uint32_t		uint32;
typedef uint32_t	ticks_t;	/* pinned to 32-bit to match DOS binary file layout */

// Generational handle into MemoryMgr::gripDataArray[].
// index: slot (1..GRIPS_MAX-1); generation: increments on every free of that slot.
// Same 4-byte size as the old typedef uint grip.
struct grip {
	uint16 index;
	uint16 generation;

	// Default-constructed grip is indeterminate; use grip{0,0} for null.
	// No user-provided constructors — grip is an aggregate (C++17) and POD,
	// allowing it to be placed in __attribute__((packed)) structs.
	grip() = default;

	explicit operator bool() const { return index != 0; }
	bool operator==(const grip& o) const { return index == o.index && generation == o.generation; }
	bool operator!=(const grip& o) const { return !(*this == o); }
};

// Pack/unpack grip into void* / uintptr_t for storage in TIGRE pointer-based lists.
// index occupies bits 0-15, generation occupies bits 16-31.
// ptr_to_grip(0) == grip{0,0} == null grip (operator bool returns false).
inline void* grip_to_ptr(grip g) {
    uintptr_t v = (uintptr_t)g.index | ((uintptr_t)g.generation << 16);
    return reinterpret_cast<void*>(v);
}
inline grip ptr_to_grip(void* p) {
    uintptr_t v = reinterpret_cast<uintptr_t>(p);
    return grip{(uint16)(v & 0xFFFF), (uint16)((v >> 16) & 0xFFFF)};
}
inline grip ptr_to_grip(uintptr_t v) {
    return grip{(uint16)(v & 0xFFFF), (uint16)((v >> 16) & 0xFFFF)};
}

typedef void*				dgrip;
typedef unsigned char	uchar;
typedef int					coord;


// Dialog draw to dynamic cel procedure ptr
typedef	void	(*DrawProc)(grip gDynCel);


// Message & Notice Types
typedef uint16	evt_t;

enum notice_t
{
	N_NULL,
	N_CUE,					// tell script to advance state
	N_SETUP,
	N_QUIT,					// tell context to quit
	N_CLICK,					// click messages for responders
	N_DO_CONV_EXCHANGE,	
	N_NEXT_CONV_EXCHANGE,
	N_CHANGE_LOCATION,
	N_TIMER_FIRED,
	N_PAUSE_CLOCK,
	N_LAST
};

enum msg_t
{
	MSG_NULL,
	MSG_EVENT,
	MSG_NOTICE,
	MSG_ABORTED
};


// Text justification
enum justify
{
	DG_JUST_LEFT,
	DG_JUST_CENTER,
	DG_JUST_RIGHT,
	DG_JUST_TOP,
	DG_JUST_BOTTOM
};


// Control types
enum ctrl_t
{
	CTRL_BUTTON,		// Standard button
	CTRL_TEXT,			// Edit field
	CTRL_LABEL,			// Non-editable text
	CTRL_RBUTTON,		// Radio button (mutually exclusive)
	CTRL_CHKBOX,		// Check box
	CTRL_LISTBOX		// List box, pick list, etc
};


// "Modality", "Modalnes", "Modalage", "Modalocity", et al.
enum modalType
{
	M_INACTIVE,
	M_MODELESS,
	M_SEMI,
	M_MODAL
};


// Graphics

// Used in VGABuffer, bitmap_t is a platform specific reference to
// a pixel map.
typedef	grip			bitmap_t;


#endif

