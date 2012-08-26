/// \file CVUtil.h
/// \brief CVUtil.h defines some basic utility functions usable throughout
///        CodeVis apps.
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
#ifndef _CVUTIL_H_
#define _CVUTIL_H_

#include "CVTrace.h"    // Include assertions and such.
#include <limits.h>     // Assert on bad floats.

/// CVSwap swaps to values of the same type
template<class T>
inline void CVSwap(T& a, T& b)
{
   T c = a;
   a = b;
   b = c;
}

/// CVMin returns the minimum of two values. Types must be the same.
template<class T>
inline T const& CVMin(T const& a, T const& b)
{
   // return minimum
   return a < b ? a : b;
}

/// CVMax returns the maximum of two values. Types must be the same.
template<class T>
inline T const& CVMax(T const& a, T const& b)
{
   // return maximum
   return a > b ? a : b;
}

/// Round a floating point to the nearest integer
/// Assumption: float must be within valid integer range.
/// \param floatVal - floating point value to round. 
///        Must be within integer range.
/// \return int - floatVal rounded to nearest integer
inline int CVRound(float floatVal)
{
   CVAssert( (floatVal >= INT_MIN) && (floatVal <= INT_MAX),
             "CVRound returns an integer, so the passed in value " \
             "to round must be within range for an int. It isn't.");

   if (floatVal < 0) 
   {
      return (int)(floatVal - 0.5f);
   }
   else
   {
      return (int)(floatVal + 0.5f);
   }   
}  

/// Round a double-precision floating point to the nearest integer
/// Assumption: float must be within valid integer range.
/// \param dblVal - double-precision floating point value to round. 
///        Must be within integer range.
/// \return int - dblVal rounded to nearest integer
inline int CVRound(double dblVal)
{
   CVAssert( (dblVal >= INT_MIN) && (dblVal <= INT_MAX),
             "CVRound returns an integer, so the passed in value " \
             "to round must be within range for an int. It isn't.");

   if (dblVal < 0) 
   {
      return (int)(dblVal - 0.5);
   }
   else
   {
      return (int)(dblVal + 0.5);
   }   
}  


#endif _CVUTIL_H_