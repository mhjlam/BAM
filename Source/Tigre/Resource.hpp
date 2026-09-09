//
// RESOURCE.HPP
//
// November 12, 1993
// (c) Copyright 1993, Tachyon, Inc.  All rights reserved.
//
// Resource class definition.
//
//		A Resource is a header of sorts for resources loaded from disk.
//
//----[]-------------------------------------------------------------


#ifndef	resource_hpp
#define	resource_hpp


#include "Types.hpp"
#include "WriteRes.hpp"


#define	RES_DYNAMIC	0x80000000

enum	purge_t
{
	RES_NOPURGE,
	RES_PURGE
};


class Resource : public Object
{
	public:
		Resource();
		~Resource();

		res_t		type;			// Resource type
		uint		number;		// Resource number
		uint16	lockCount;	// Cumulative locks
		uint16	attr;			// Attributes
		uint16	compType;	// Compression type
		uint32	size;			// Size of data allocation only
		uint16	time;			// Time stamp (for LIFO)
		uint16	data[4];		// Resource-specific data
		uint		numHeaders;	// Number of headers in the resource
		purge_t	purgeType;	// What to do when useCount is 0

		virtual	void			Init(res_t rType, uint rNum);
		virtual	void			Lock();
		virtual	void			Unlock(bool force = FALSE);
		virtual	bool			CheckAttr(uint16 a);
		virtual	void			SetAttr(uint16 a, bool on);
		virtual	uchar*		Data();
		virtual	CelHeader*	GetHeader(uint c);
		virtual	Resource*	CopyAndInit(grip gRes, res_t rType, uint rNum);
		virtual	bool			IsDynamic();
		virtual	void			SetUseCount(uint newCount);
		virtual	uint			GetUseCount();
		virtual	bool			Purge();

	private:
		uint		useCount;	// Number of objects "owning" this resource

};

#endif


/*

----[About ResMgr]---------------------------------------------------

	A resource file consists of three parts: the resource header, any
number of cel headers, and one or more data buffers.

	The resource header contains information that is required to read
the resource from the disk file into memory and represent it as a
Resource.  The ResHeader structure is used to represent the resource
header.

	Animation-related resources (cels, anims, and pics) are made up of
cel data, consisting of one or more headers and an equal number of data
buffers.  Cel headers contain information about the individual cel, such
as size, priority, and origin location.  Each cel header contains a
absolute offset to the beginning of its corresponding data.

	Data buffers contain the raw (compressed or uncompressed) bitmap data
that will be blitted to the visual.


	A Resource object is a C++ object that contains information about a
resource.  The data members include resource-tracking data fields as well
as resource header information.  A Resource also contains a total number
of headers, a pointer to the data block, and an offset to the start of
the actual cel data.

		������������Ŀ
		�  Resource  �
		��������������
		 |
		 �- gData ->������������Ŀ
					 	� Cel Header �
					 	������������Ĵ
					 	� Cel Header �
					 	������������Ĵ
					 	� Cel Header �
					 	������������Ĵ
					 	�  Cel Data  �
					 	������������Ĵ
					 	�  Cel Data  �
					 	������������Ĵ
					 	�  Cel Data  �
					 	��������������


Resource types:

RES_CEL

	������������Ŀ		
	�  Resource  �
	������������Ĵ
	� Cel Header �
	������������Ĵ
	�  Cel Data  �
	��������������


RES_ANIM, RES_PIC

	��������������Ŀ		
	�   Resource   �
	��������������Ĵ
	� Cel Header 1 �
	��������������Ĵ
	� Cel Header 2 �
	��������������Ĵ
	� Cel Header 3 �
	��������������Ĵ
	�  Cel Data 1  �
	��������������Ĵ
	�  Cel Data 2  �
	��������������Ĵ
	�  Cel Data 3  �
	����������������

	*/
