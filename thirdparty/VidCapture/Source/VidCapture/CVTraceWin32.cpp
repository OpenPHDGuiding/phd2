#ifndef WIN32
   #pragma message ("WIN32 not defined. Not compiling CVPlatformWin32")
#else

// CVTrace - basic debugging functions ( CVTrace, CVAssert, and timing)
// Win32 implementation.
//
// Written by Michael Ellison
//-------------------------------------------------------------------------
//                      CodeVis's Free License
//                         www.codevis.com
//
// Copyright (c) 2003 by Michael Ellison (mike@codevis.com)
// All rights reserved.
//
// You may use this software in source and/or binary form, with or without
// modification, for commercial or non-commercial purposes, provided that 
// you comply with the following conditions:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer. 
//
// * Redistributions of modified source must be clearly marked as modified,
//   and due notice must be placed in the modified source indicating the
//   type of modification(s) and the name(s) of the person(s) performing
//   said modification(s).
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED 
// TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//---------------------------------------------------------------------------
// Modifications:
//
//---------------------------------------------------------------------------
/// \file CVTraceWin32.cpp
/// \brief CVTraceWin32.cpp implements the debugging macros.
///
/// CVTraceWin32 .cpp implements simple debugging macros that will compile out
/// in release mode: CVTrace(), CVAssert(), CVStartTime(), CVEndTime().
/// These implementations are for Windows environments only - create another
/// file for other platforms.
///
/// $RCSfile: CVTraceWin32.cpp,v $
/// $Date: 2004/02/08 23:47:39 $
/// $Revision: 1.1.1.1 $
/// $Author: mikeellison $

#include <windows.h>
#include <stdio.h>
#include <time.h>
#include "CVUtil.h"

//-------------------------------------------------------------------------
// _CVTrace() sends a message to the debug output.
//
// Use the CVTrace() macro instead if you want it to compile out
// in release mode.
//-------------------------------------------------------------------------
void _CVTrace(const char* msg, const char* file, int line)
{
   DWORD lastError = GetLastError();
   
   // 100 is a somewhat arbitrary length here. 
   // If you change the text, make sure it 
   // stays below this for extra fields or increase it.  
   //
   // _MAX_PATH handles the file argument
   // line's max length should be 16 with possible commas and neg sign
   
   char* dbgMsgBuf = new char[_MAX_PATH + 100 + strlen(msg)];
   if (dbgMsgBuf)
   {
      sprintf(dbgMsgBuf,"%s(%d) : %s\n", file, line, msg);
      ::OutputDebugStringA(dbgMsgBuf);
      delete [] dbgMsgBuf;
   }

   ::SetLastError(lastError);
}

//-------------------------------------------------------------------------
// _CVAssert() implements the assertion function.
//
// Don't call this directly - use CVAssert() macro instead.
//
//-------------------------------------------------------------------------
void _CVAssert(const char *expression, 
               const char* file, 
               int line, 
               const char* description)
{
   DWORD lastError = GetLastError();
   
   // 100 is a somewhat arbitrary length here. 
   // If you change the text, make sure it 
   // stays below this for extra fields or increase it.  
   // We print out two lines, to take the larger buffer size of the two.
   unsigned int bufferLength = _MAX_PATH + 100 + strlen(expression);
   if (strlen(description) + 100 > bufferLength)
   {
      bufferLength = strlen(description) + 100;
   }
      
   char* dbgMsgBuf = new char[bufferLength];

   if (dbgMsgBuf)
   {
      sprintf(dbgMsgBuf,   "%s(%d) : ASSERT FAILED: %s\n", 
                           file, 
                           line, 
                           expression);

      ::OutputDebugStringA(dbgMsgBuf);

      sprintf(dbgMsgBuf,"%s\n",description);
      ::OutputDebugStringA(dbgMsgBuf);
      
      delete [] dbgMsgBuf;
   }

   // This is a hard breakpoint. If you've stopped here, 
   // it means you've hit an assertion that failed.
   // In Visual Studio 6.0, hit Shift-F11 to run back up 
   // to the assertion code. Also, check the Output->Debug 
   // window for trace messages.
   _asm {int 3};

   ::SetLastError(lastError);
}


// global timer variables
unsigned int CVStartTickLow      =  0;
unsigned int CVStartTickHigh     =  0;
unsigned int CVEndTickLow        =  0;
unsigned int CVEndTickHigh       =  0;

double       CVTicksPerSec       =  0;
bool         CVTickInitialized   =  false;

//-------------------------------------------------------------------------
// _CVInitTicks() initializes the timer information.  
//
// It is automatically called the first time _CVStartTime() is called and
// does not need to be called directly.
//
//-------------------------------------------------------------------------
void _CVInitTicks()
{  
   
   clock_t startClock = clock();

   _asm
   {
      _emit 0x0f; // RTDSC;
      _emit 0x31; 
      mov   CVStartTickLow,   eax;
      mov   CVStartTickHigh,  edx;
   }  
   
   Sleep(1000);
   
   _asm
   {
      _emit 0x0f; // RTDSC;
      _emit 0x31; 
      mov   CVEndTickLow,     eax;
      mov   CVEndTickHigh,    edx;
   }
   
   clock_t endClock = clock();

   __int64 startTick = ((__int64)CVStartTickHigh << 32) + CVStartTickLow;
   __int64 endTick   = ((__int64)CVEndTickHigh   << 32) + CVEndTickLow;
   __int64 resultTick = endTick - startTick;

   double totalTime = ((double)(endClock - startClock))/CLOCKS_PER_SEC;

   CVTicksPerSec = (double)resultTick/totalTime;

   // Send a debug message indicating the processor speed.
   // On some processors, this will be totally bogus.
   // In those cases, so will the results of CVStartTime()/CVEndTime().
   char dbgMsg[255];
   sprintf(dbgMsg,"Initialized RTDSC: Processor seems to be %dMHz\n",
           (int)CVTicksPerSec/1000000);

   ::OutputDebugStringA(dbgMsg);

   CVTickInitialized = true;  
}
//-------------------------------------------------------------------------
// _CVStartTime() stores the cycle count to aid in manual profiling.
//
// Use CVStartTime() macro instead to allow
// it to be removed from the compile in release mode.
//
//-------------------------------------------------------------------------
void _CVStartTime()
{
   // Initialize ticks if we haven't already. This can cause us to 
   // take a performance hit on the first startTime(), although the
   // timestamp isn't affected by it since the start marker is read
   // after initialization.
   if (!CVTickInitialized)
      _CVInitTicks();

   _asm
   {
      _emit 0x0f; // RTDSC;
      _emit 0x31; 
      mov   CVStartTickLow,   eax;
      mov   CVStartTickHigh,  edx;
   }  
}

//-------------------------------------------------------------------------
// _CVEndTime() completes a timing started by _CVStartTime() and sends
// the amount of time elapsed to the debugging console. 
//
// Use the CVEndTime() macro instead to allow
// it to be removed from the compile in release mode.
//
//-------------------------------------------------------------------------
void _CVEndTime()
{
   // Store ending time
   _asm
   {
      _emit 0x0f; // RTDSC;
      _emit 0x31; 
      mov   CVEndTickLow,     eax;
      mov   CVEndTickHigh,    edx;
   }

   // Calc tick count
   __int64 startTick = ((__int64)CVStartTickHigh << 32) + CVStartTickLow;
   __int64 endTick   = ((__int64)CVEndTickHigh   << 32) + CVEndTickLow;
   __int64 resultTick = (endTick - startTick);
   double  seconds = (double)resultTick / CVTicksPerSec;
   char dbgMsg[255];
   sprintf(dbgMsg, "CVTickCount: %f sec\n",seconds);
   ::OutputDebugStringA(dbgMsg);
}

#endif // WIN32
