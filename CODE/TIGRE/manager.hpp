//
// MANAGER.HPP
//
// June 20, 1994
// WATCOM: October 3, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// Base class for all managers.
//
//----[]-------------------------------------------------------------


#ifndef	manager_hpp
#define	manager_hpp


#include "types.hpp"


class Manager
{
	public:
		void* operator new(size_t size);
};

#endif

