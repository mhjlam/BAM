;***************************************************************************
; general equates
;***************************************************************************
_MAX_VOICES          equ      32
_ERR_NO_SLOTS        equ      -1
_ERR_NO_ERROR        equ      0
_ERR_INVALID_HANDLE  equ      10
_TRUE                equ      1
_FALSE               equ      0

;***************************************************************************
; macro definitions follow
;***************************************************************************

; macro for function start
CFuncStart  macro

            push  ebx
            push  ecx
            push  edx
            push  esi
            push  edi
            push  es

            endm

; macro for function end
CFuncEnd    macro

            pop   es
            pop   edi
            pop   esi
            pop   edx
            pop   ecx
            pop   ebx

            endm

;***************************************************************************
; equates for sample flags
;***************************************************************************
_ACTIVE                 equ      8000h
_LOOPING                equ      4000h
_FIRST_TIME             equ      2000h
_PENDING_RELEASE        equ      1000h
_CONTINUE_BLOCK         equ      0800h
_PITCH_SHIFT            equ      0400h
_PANNING                equ      0200h
_VOLUME                 equ      0100h
_TRANSLATE16TO8         equ      0080h
_STAGE_LOOP             equ      0040h
_TRANSLATE8TO16         equ      0020h
_STEREOTOMONO           equ      0010h
_SKIP_PROCESSED_CB      equ      0008h
_SKIP_LOOP_CB           equ      0004h
_SKIP_DONE_CB           equ      0002h

;***************************************************************************
; data stucture definitions follow.
;***************************************************************************

_SOS_START_SAMPLE    struc

                     ; pointer to sample data
                     pSamplePtr           dd       0
                                          dd       0

                     ; size of the sample
                     dwSampleSize         dd       0

                     ; loop count
                     wLoopCount           dd       0

                     ; channel   
                     wChannel             dd       0

                     ; volume
                     wVolume              dd       0

                     ; sample ID
                     wSampleID            dd       0

                     ; call back
                     pCallback            dd       0
                                          dd       0

                     ; port
                     wSamplePort          dd       0

                     ; flags
                     wSampleFlags         dd       0

                     ; length in bytes
                     dwSampleByteLength   dd       0

                     ; loop point for sample
                     dwSampleLoopPoint    dd       0
                     dwSampleLoopLength   dd       0

                     ; pitch shifting components
                     dwSamplePitchAdd     dd       0
                     wSamplePitchFraction dd       0

                     ; panning components
                     wSamplePanLocation   dd       0
                     wSamplePanSpeed      dd       0
                     wSamplePanDirection  dd       0
                     wSamplePanStart      dd       0
                     wSamplePanEnd        dd       0

                     ; delay components
                     wSampleDelayBytes    dd       0
                     wSampleDelayRepeat   dd       0

                     ; compression components
                     dwSampleADPCMPredict dd       0
                     wSampleADPCMIndex    dd       0

                     ; root note for MIDI
                     wSampleRootNoteMIDI  dd       0

                     ; filler
                     dwSampleTemp1        dd       0
                     dwSampleTemp2        dd       0
                     dwSampleTemp3        dd       0

                     ends

sampleStartSize      equ   type  _SOS_START_SAMPLE

; this structure is used inside the timer handlers
timerSAMPLE             struc

        samplePtr             dq       0                 ; pointer to sample data
        sampleDataPtr         dq       0                 ; pointer to active data
        sampleLoopPtr         dq       0                 ; pointer to loop back

        sampleLength          dd       0                 ; DWORD length of sample
        sampleIndex           dd       0                 ; DWORD index into sample
        sampleLoopLength      dd       0                 ; DWORD length of loop segment

        sampleBytesLeft       dd       0                 ; bytes left to play

        sampleLoopPoint       dd       0                 ; byte count for loop point
        sampleLoopEndLength   dd       0                 ; length of end loop point

        sampleFlags           dw       0                 ; sample flags
        sampleVolume          dw       0                 ; sample attinuation value
        sampleID              dw       0                 ; sample ID value

        sampleChannel         dw       0                 ; current play channel
        sampleLoopCount       dw       0                 ; loop count
        sampleLastFill        dw       0                 ; last point of filling
        sampleCallback        dq       0                 ; call back function

        samplePitchAdd        dd       0                 ; pitch shift value
        samplePitchFraction   dw       0                 ; sample pitch fraction

        samplePort            dw       0                 ; port for non-DMA device

        sampleTotalBytes      dd       0                 ; total bytes processed
        sampleByteLength      dd       0                 ; actual length of sample

        samplePanLocation     dw       0                 ; location of pan
        samplePanSpeed        dw       0                 ; speed of pan
        samplePanDirection    dw       0                 ; pan direction +- 1..
        samplePanStart        dw       0                 ; pan start
        samplePanEnd          dw       0                 ; pan end

        sampleDelayBytes      dw       0                 ; bytes for delay
        sampleDelayRepeat     dw       0                 ; number of repeats

        sampleADPCMPredicted  dd       0                 ; predicted value
        sampleADPCMIndex      dw       0                 ; index into table

        sampleRootNoteMIDI    dw       0                 ; root note for sample

        sampleFraction        dw       0                 ; reserved

timerSAMPLE             ends

timerSAMPLESize         equ     type    timerSAMPLE

; structure to represent a "far" pointer w/selector and offset
SFAR           struc

               ; selector and offset
               wOffset      dd    0
               wSelector    dd    0

SFAR           ends

