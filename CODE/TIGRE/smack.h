#ifndef SMACKH
#define SMACKH

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SMACKVERSION "2.0y"

typedef struct SmackTag {
  uint32_t Version;           // SMK2 only right now
  uint32_t Width;             // Width (1 based, 640 for example)
  uint32_t Height;            // Height (1 based, 480 for example)
  uint32_t Frames;            // Number of frames (1 based, 100 = 100 frames)
  uint32_t MSPerFrame;        // Frame Rate
  uint32_t SmackerType;       // bit 0 set=ring frame
  uint32_t LargestInTrack[7]; // Largest single size for each track
  uint32_t tablesize;         // Size of the init tables
  uint32_t codesize;          // Compression info
  uint32_t absize;            // ditto
  uint32_t detailsize;        // ditto
  uint32_t typesize;          // ditto
  uint32_t TrackType[7];      // high byte=0x80-Comp,0x40-PCM data,0x20-16 bit,0x10-stereo
  uint32_t extra;             // extra value (should be zero)
  uint32_t NewPalette;        // set to one if the palette changed
  uint8_t  Palette[772];      // palette data
  uint32_t FrameNum;          // Frame Number to be displayed
  uint32_t LastRectx;         // Rect set in from SmackToBufferRect (X coord)
  uint32_t LastRecty;         // Rect set in from SmackToBufferRect (Y coord)
  uint32_t LastRectw;         // Rect set in from SmackToBufferRect (Width)
  uint32_t LastRecth;         // Rect set in from SmackToBufferRect (Height)
  uint32_t OpenFlags;         // flags used on open
  uint32_t LeftOfs;           // Left Offset used in SmackTo
  uint32_t TopOfs;            // Top Offset used in SmackTo
} Smack;

//=======================================================================
#define SMACKNEEDVOLUME 0x00040L // Will be setting the volume
#define SMACKSIMULATE   0x00800L // Simulate the speed (call SmackSim first)
#define SMACKLOADEXTRA  0x00100L // Load the extra buffer during SmackOpen
#define SMACKNOSKIP     0x00400L // Don't skip frames if falling behind
#define SMACKFILEHANDLE 0x01000L // Use when passing in a file handle
#define SMACKTRACK1     0x02000L // Play audio track 1
#define SMACKTRACK2     0x04000L // Play audio track 2
#define SMACKTRACK3     0x08000L // Play audio track 3
#define SMACKTRACK4     0x10000L // Play audio track 4
#define SMACKTRACK5     0x20000L // Play audio track 5
#define SMACKTRACK6     0x40000L // Play audio track 6
#define SMACKTRACK7     0x80000L // Play audio track 7
#define SMACKTRACKS (SMACKTRACK1|SMACKTRACK2|SMACKTRACK3|SMACKTRACK4|SMACKTRACK5|SMACKTRACK6|SMACKTRACK7)

#define SMACKAUTOEXTRA 0xffffffffL // NOT A FLAG! - Use as extrabuf param
//=======================================================================

Smack* SmackOpen(char* name, uint32_t flags, uint32_t extrabuf);

uint32_t SmackDoFrame(Smack* smk);
void     SmackNextFrame(Smack* smk);
uint32_t SmackWait(Smack* smk);
void     SmackClose(Smack* smk);

void SmackVolumePan(Smack* smk, uint32_t trackflag, uint32_t volume, uint32_t pan);

uint32_t SmackSoundOnOff(Smack* smk, uint32_t on);

void     SmackToBuffer(Smack* smk, uint32_t left, uint32_t top, uint32_t Pitch, uint32_t destheight, void* buf, uint32_t Reversed);
uint32_t SmackToBufferRect(Smack* smk, uint32_t SmackSurface);

void SmackSimulate(uint32_t sim);

#ifdef __cplusplus
}
#endif

#endif
