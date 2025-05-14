//
// MANAGER.CPP
//
// June 20, 1994
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// Base class for all managers.
//
//----[]-------------------------------------------------------------

#include "manager.hpp"
#include "mem.hpp"


void*
Manager:: operator new(size_t size)
{
	// allocate this using a system grip
	return newSys(size);	
}

