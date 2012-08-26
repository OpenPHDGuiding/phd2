// CVTrace - basic debugging functions ( CVTrace, CVAssert, and timing)
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
/// \file CVTrace.h
/// \brief CVTrace.h defines the debugging macros.
///
/// CVTrace.h provides some simple debugging macros that will compile out
/// in release mode: CVTrace(), CVAssert(), CVStartTime(), CVEndTime().
///
///
/// $RCSfile: CVTrace.h,v $
/// $Date: 2004/02/08 23:47:39 $
/// $Revision: 1.1.1.1 $
/// $Author: mikeellison $

#ifndef _CVTrace_H_
#define _CVTrace_H_


/// _CVTrace() sends a message to the debug output.
///
/// Use the CVTrace() macro instead if you want it to compile out
/// in release mode.
///
/// \param msg - ASCIIZ trace message to send to debug output
/// \param file - ASCIIZ filename of originating file (__FILE__)
/// \param line - line number of caller (__LINE__)
///
/// \sa CVTrace()
void _CVTrace (const char* msg, const char* file, int line);

/// _CVAssert() implements the assertion function.
///
/// Don't call this directly - use CVAssert() macro instead.
///
/// \param expression - ASCIIZ translation of asserting expression
/// \param file - ASCIIZ filename of originating file (__FILE__)
/// \param line - line number of caller (__LINE__)
/// \param description - description of assertion
///
/// \sa CVAssert()
void _CVAssert(   const char* expression, 
                  const char* file, 
                  int         line, 
                  const char* description = 0);

/// _CVStartTime() stores the cycle count to aid in manual profiling.
///
/// Use CVStartTime() macro instead to allow
/// it to be removed from the compile in release mode.
///
/// \sa CVStartTime(), CVEndTime()
void _CVStartTime();

/// _CVEndTime() completes a timing started by _CVStartTime() and sends
/// the amount of time elapsed to the debugging console. 
///
/// Use the CVEndTime() macro instead to allow
/// it to be removed from the compile in release mode.
///
/// \sa CVEndtime(), CVStartTime()
void _CVEndTime();

/// _CVInitTicks() initializes the timer information.  
///
/// It is automatically called the first time _CVStartTime() is called 
/// and does not need to be called directly.
///
/// \sa CVStartTime(), CVEndTime()
void _CVInitTicks();


#define __CVEXPSTRING__(x) #x
#define __CVSTRINGCONV__(x) __CVEXPSTRING__(x)
#define __CVLOCINFO__ __FILE__ "("__CVSTRINGCONV__(__LINE__)") : "
/// The CVREMINDER macro is used with #pragma to print out a reminder 
/// during compile that you can click on from the Visual Studio 
/// interface to go to the code that generated it.  
///
/// It's useful for leaving todo's and such
/// in the code.  The __CV*__ macros above it are to jump through
/// the hoops required to convert the __LINE__ macro into
/// a line number string.
///
/// Usage: #pragma CVREMINDER("Fix this.")
#define CVREMINDER(x) message (__CVLOCINFO__ x)


#ifdef _DEBUG
   /// CVTrace() sends a message to the debug output console
   /// CVTrace() macros are only defined in Debug mode.
   ///
   /// \param msg - ASCIIZ message to send
   /// \sa _CVTrace()
   #define CVTrace(msg)   \
           _CVTrace( (msg), __FILE__, __LINE__)

   /// CVAssert() halts the program if an assertion failed.
   ///
   /// CVAssert() macros are only defined in Debug mode.
   /// Do NOT put procedural code in them, or you will make the
   /// code behave differently between debug and release!
   ///
   /// \param exp = boolean expression to check
   /// \param info = ASCIIZ string describing the assertion
   /// \sa _CVAssert()
   #define CVAssert(exp,info) \
          (void)( (exp) || (_CVAssert(#exp, __FILE__, __LINE__, info), 0) )

   
   /// CVStartTime() starts a high resolution timer to assist in
   /// manual profiling.  
   ///
   /// CVStartTime() is only defined in Debug mode.
   ///
   /// \sa CVEndTime()
   #define CVStartTime()   _CVStartTime()

   /// CVEndTime() completes a high resolution timer run and sends
   /// the amount of time elapsed to the debugging console. 
   ///
   /// CVEndTime() is only defined in Debug mode.
   ///
   /// \sa CVStartTime()
   #define CVEndTime()     _CVEndTime()

#else
   // Define out the traces and assertions for release mode.
   #define CVTrace(msg)          ((void)0)
   #define CVAssert(exp,info)    ((void)0)
   #define CVStartTime()         ((void)0)
   #define CVEndTime()           ((void)0)
#endif


#endif // _CVTrace_H_

