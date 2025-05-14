//
// T12.CPP
//
// February 8, 1994
// WATCOM: September 20, 1994  BKH
// (c) Copyright 1994, Tachyon, Inc.  All rights reserved.
//
// T12 Data Compression
//
//		Tigre 12 bit LZ compression.  This method of compression is
//		basically the enhanced 15 bit compression from "The Data
//		Compression Book" (by Mark Nelson) modified to use only 12 bits.
//		Reducing the number of bits greatly reduced the amount of memory
//		needed to run the compression.
//
//		This compression starts out with a table that already has the
//		first 256 characters.  Therefore, "phrases" immediately start
//		building.  The compression codes start at 9 bits and increase
//		to 12 bits as the table values require more bits.  When the
//		table becomes full, it's reinitialized.
//		
//----[]-------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api.hpp"
#include "t12.hpp"
#include "writeres.hpp"

/*
 * Constants used throughout the program.  BITS defines the maximum
 * number of bits that can be used in the output code.  TABLE_SIZE defines
 * the size of the dictionary table.  TABLE_BANKS are the number of
 * 256 element dictionary pages needed.  The code defines should be
 * self-explanatory.
 */

#define BITS                       12
#define MAX_CODE                   ( ( 1 << BITS ) - 1 )
#define TABLE_SIZE                 5021
#define TABLE_BANKS                ( ( TABLE_SIZE >> 8 ) + 1 )
#define END_OF_STREAM              256
#define BUMP_CODE                  257
#define FLUSH_CODE                 258
#define FIRST_CODE                 259
#define UNUSED                     -1

/*
 * Local prototypes.
 */

uint16 T12_Find_Child_Node( uint16 parent_code, int16 child_character );
uint16 T12_Decode_String( uint16 offset, uint16 code );

void T12_OutputBits(FILE *bit_file, uint32 code, int16 count );
uint32 T12_InputBits(FILE *bit_file, int16 bit_count, char* pInputBuffer);

/*
 * This data structure defines the dictionary.  Each entry in the dictionary
 * has a code value.  This is the code emitted by the compressor.  Each
 * code is actually made up of two pieces:  a parent_code, and a
 * character.  Code values of less than 256 are actually plain
 * text codes.
 *
 * Every reference to the dictionary was replaced by a macro that did
 * a pointer dereference first.  
 */

// #pragma pack(1)

struct dictionary {
    int16 code_value;
    int16 parent_code;
    char character;
} dict[ TABLE_BANKS ][ 256];

// #pragma pack()

/*
 * The special DICT() macro is used to translate
 * indices into pairs of references.
 */

#define DICT( i ) dict[ i >> 8 ][ i & 0xff ]

/*
 * Other global data structures.  The decode_stack is used to reverse
 * strings that come out of the tree during decoding.  next_code is the
 * next code to be added to the dictionary, both during compression and
 * decompression.  current_code_bits defines how many bits are currently
 * being used for output, and next_bump_code defines the code that will
 * trigger the next jump in word size.
 */

char decode_stack[ TABLE_SIZE ];
uint16 next_code;
int16 current_code_bits;
uint16 next_bump_code;

// bit file variables

unsigned char 	t12_mask;
int16 			t12_rack;
uint32			t12_buffer_count;
uint32			t12_buffer_max;
uint32			t12_return_count;

/*
 * This routine is used to initialize the dictionary, both when the
 * compressor or decompressor first starts up, and also when a flush
 * code comes in.  Note that even thought the decompressor sets all
 * the code_value elements to UNUSED, it doesn't really need to.
 */
void InitializeDictionary();

void InitializeDictionary()
{
   uint16 i;

   for ( i = 0 ; i < TABLE_SIZE ; i++ )
	{
      DICT( i ).code_value = UNUSED;
	}

   next_code = FIRST_CODE;
   current_code_bits = 9;
   next_bump_code = 511;
}

/*
 * The compressor is short and simple.  It reads in new symbols one
 * at a time from the input file.  It then  checks to see if the
 * combination of the current symbol and the current code are already
 * defined in the dictionary.  If they are not, they are added to the
 * dictionary, and we start over with a new one symbol code.  If they
 * are, the code for the combination of the code and character becomes
 * our new code.  Note that in this enhanced version of LZW, the
 * encoder needs to check the codes for boundary conditions.
 */

uint32 
T12_Flatten(FILE* pOutputFile, uint32 inputSize, char* pInputBuffer)
{
   int16 character;
   int16 string_code;

   uint16 index;
	char	*pBufferEnd = pInputBuffer + inputSize;

   InitializeDictionary();

	t12_return_count = 0;

	// initialize the bit file variables
	t12_rack = 0;
   t12_mask = 0x80;

	if (pInputBuffer < pBufferEnd) {
		string_code = *pInputBuffer;
		string_code &= 0xff;
		pInputBuffer++;
	}
	else {
      string_code = END_OF_STREAM;
	}
 
   while (pInputBuffer < pBufferEnd)
	{
		character = *pInputBuffer;
		character &= 0xff;
		pInputBuffer++;

      index = T12_Find_Child_Node( string_code, character );
      if ( DICT( index ).code_value != - 1 )
		{
         string_code = DICT( index ).code_value;
		}
      else {
         DICT( index ).code_value = next_code++;
         DICT( index ).parent_code = string_code;
         DICT( index ).character = (char) character;
         T12_OutputBits( pOutputFile,
                     (uint32) string_code, current_code_bits );
         string_code = character;

         if ( next_code > MAX_CODE ) 
			{
               T12_OutputBits( pOutputFile,
                           (uint32) FLUSH_CODE, current_code_bits );
               InitializeDictionary();
         } 
			else
			{
				if ( next_code > next_bump_code ) 
				{
               T12_OutputBits( pOutputFile,
                           (uint32) BUMP_CODE, current_code_bits );
               current_code_bits++;
               next_bump_code <<= 1;
               next_bump_code |= 1;
         	}
			}
      }
   }

   T12_OutputBits( pOutputFile, (uint32) string_code, current_code_bits );
   T12_OutputBits( pOutputFile, (uint32) END_OF_STREAM, current_code_bits);

	if (t12_mask != 0x80 ) 
	{
   	if ( putc( t12_rack, pOutputFile ) != t12_rack )
      	APanic( "T12_Flatten: unable to write rack!\n" );
		t12_return_count++;
	}

	return (t12_return_count);
}

/*
 * The file expander operates much like the encoder.  It has to
 * read in codes, then convert the codes to a string of characters.
 * The only catch in the whole operation occurs when the encoder
 * encounters a CHAR+STRING+CHAR+STRING+CHAR sequence.  When this
 * occurs, the encoder outputs a code that is not presently defined
 * in the table.  This is handled as an exception.  All of the special
 * input codes are handled in various ways.
 */

uint32 
T12_Fluff(FILE* pInputFile, uint32 inputSize, char* pOutputBuffer, char* pInputBuffer)
{
	uint16 	new_code;
	uint16 	old_code;
	int16 	character;
	uint16 	count;
	char*		pOriginalOutputBuffer;

	pOriginalOutputBuffer = pOutputBuffer;

	// initialize the bit file variables
	t12_rack = 0;
   t12_mask = 0x80;
	t12_buffer_max = inputSize;
	t12_buffer_count = 0;

	for ( ; ; ) 
	{
   	InitializeDictionary();
   	old_code = (uint16) T12_InputBits(pInputFile, current_code_bits, pInputBuffer);
   	if ( old_code == END_OF_STREAM )
		{
			return (pOutputBuffer - pOriginalOutputBuffer);
		}

   	character = old_code;
		*pOutputBuffer = (char) old_code;
		pOutputBuffer++;

		for ( ; ; ) 
		{
   		new_code = (uint16) T12_InputBits(pInputFile, current_code_bits, pInputBuffer);
   		if ( new_code == END_OF_STREAM )
			{
				return (pOutputBuffer - pOriginalOutputBuffer);
			}
   		if ( new_code == FLUSH_CODE )
			{
         	break;
			}
   		if ( new_code == BUMP_CODE ) 
			{
         	current_code_bits++;
         	continue;
   		}

   		if ( new_code >= next_code ) 
			{
         	decode_stack[ 0 ] = (char) character;
         	count = T12_Decode_String( 1, old_code );
   		}
   		else
			{
         	count = T12_Decode_String( 0, new_code );
			}

   		character = decode_stack[ count - 1 ];
   		while ( count > 0 )
			{
				*pOutputBuffer = decode_stack[--count];
				pOutputBuffer++;
			}

   		DICT( next_code ).parent_code = old_code;
   		DICT( next_code ).character = (char) character;
   		next_code++;
   		old_code = new_code;
		}
	}
}

/*
 * This hashing routine is responsible for finding the table location
 * for a string/character combination.  The table index is created
 * by using an exclusive OR combination of the prefix and character.
 * This code also has to check for collisions, and handles them by
 * jumping around in the table.
 */

uint16 
T12_Find_Child_Node(uint16 parent_code, int16 child_character )
{
	uint16	index;
	uint16	offset;

	index = (uint16) (child_character << (BITS - 8)) ^ parent_code;

	if (index == 0)
	{
   	offset = 1;
	}
	else
	{
   	offset = (uint16) (TABLE_SIZE - index);
	}

	for ( ; ; ) 
	{
   	if ( DICT( index ).code_value == UNUSED )
		{
      	return( (uint16) index );
		}

   	if ( DICT( index ).parent_code == parent_code &&
         	DICT( index ).character == (char) child_character )
		{
      	return( index );
		}

   	if ( index >= offset )
		{
      	index -= offset;
		}
   	else
		{
      	index += TABLE_SIZE - offset;
		}
	}
}

/*
 * This routine decodes a string from the dictionary, and stores it
 * in the decode_stack data structure.  It returns a count to the
 * calling program of how many characters were placed in the stack.
 */

uint16 T12_Decode_String(uint16 count, uint16 code )
{
	while ( code > 255 ) 
	{
   	decode_stack[ count++ ] = DICT( code ).character;
   	code = DICT( code ).parent_code;
	}

	decode_stack[ count++ ] = (char) code;
	return( count );
}


/*
 * Bit oriented file routines
 */

void 
T12_OutputBits(FILE *bit_file, uint32 code, int16 count )
{
	uint32 mask;

	mask = 1L << ( count - 1 );
	while ( mask != 0) 
	{
		if ( mask & code ) 
		{
   		t12_rack |= t12_mask;
		}

		t12_mask >>= 1;

		if ( t12_mask == 0 ) 
		{
			if ( putc( t12_rack, bit_file ) != t12_rack ) 
			{
				APanic( "T12_Outputbits: rack error!\n" );
			}
			t12_return_count++;

			t12_rack = 0;
  			t12_mask = 0x80;
		}

		mask >>= 1;
	}
}

uint32 
T12_InputBits(FILE *bit_file, int16 bit_count, char* pInputBuffer)
{
	uint32 mask;
	uint32 return_value;

	mask = 1L << ( bit_count - 1 );
	return_value = 0;
	while ( mask != 0) 
	{
		if (t12_mask == 0x80)
		{
			if (bit_file)
			{
				t12_rack = (int16) getc(bit_file);
			}
			else
			{
				t12_rack = *pInputBuffer;
				++pInputBuffer;
			}

			if ((t12_buffer_count >= t12_buffer_max) ||
				(t12_rack == EOF))
			{
				APanic("T12_InputBits: unexpected end of data!\n");
			}

			t12_buffer_count++;
		}

		if ( t12_rack & t12_mask ) 
		{
   		return_value |= mask;
		}

   	mask >>= 1;
   	t12_mask >>= 1;

   	if ( t12_mask == 0 ) 
		{
      	t12_mask = 0x80;
		}
	}

	return( return_value );
}



