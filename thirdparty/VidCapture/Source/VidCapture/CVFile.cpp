// CVFile - basic file class
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
/// \file CVFile.cpp
/// \brief CVFile.cpp implements CVFile, a simple file class that should be useable 
/// on most platforms.
///
/// Mainly used to make it easier to implement a real file class later without
/// having to change external code much.  CVFile currently uses FILE*'s
/// for all operations.
///
/// Currently does not support huge files, file attributes, or really anything
/// aside from basic read and write operations.
///
/// $RCSfile: CVFile.cpp,v $
/// $Date: 2004/02/08 23:47:38 $
/// $Revision: 1.1.1.1 $
/// $Author: mikeellison $

#include <string.h>
#include "CVFile.h"
#include "CVUtil.h"

CVFile::CVFile()
{
   fFilePtr = 0;
}

CVFile::~CVFile()
{
   CVAssert(fFilePtr == 0, "CVFile destructor while file is still open!");
   if (fFilePtr != 0)
   {
      this->Close();
   }  
}

// OpenForRead()
//    Opens an existing file for reading.
CVRES CVFile::OpenForRead( const char* filename )
{  
   CVRES result = CVRES_SUCCESS;

   CVAssert(fFilePtr == 0,"You must close the previous file first.");
   if (fFilePtr != 0)
   {
      this->Close();
   }
   
   if (!FileExists(filename))
   {
      CVTrace("File does not exist.");
      CVTrace(filename);
      return CVRES_FILE_DOES_NOT_EXIST;
   }

   fFilePtr = fopen(filename,"rb");
   if (fFilePtr == 0)
   {
      result = CVRES_FILE_OPEN_ERROR;
   }

   return result;
}

// OpenForReadWrite
//    Opens a file for reading and writing.
//    If it does not exist, a file is created.
CVRES CVFile::OpenForReadWrite( const char* filename )
{
   CVRES result = CVRES_SUCCESS;

   CVAssert(fFilePtr == 0,"You must close the previous file first.");
   if (fFilePtr != 0)
   {
      this->Close();
   }

   fFilePtr = fopen(filename,"rb+");
   if (fFilePtr == 0)
   {
      result = CVRES_FILE_OPEN_ERROR;
   }

   return result;
}

// Create
//    Creates a file for writing. 
//    Deletes any previously existing file.
CVRES CVFile::Create( const char* filename )
{
   CVRES result = CVRES_SUCCESS;

   CVAssert(fFilePtr == 0,"You must close the previous file first.");
   if (fFilePtr != 0)
   {
      this->Close();
   }

   if (FileExists(filename))
   {      
      _unlink(filename);
   }

   fFilePtr = fopen(filename,"wb+");
   if (fFilePtr == 0)
   {
      result = CVRES_FILE_CREATE_ERROR;
   }

   return result;
}

// Close()
//    Closes the file.
CVRES CVFile::Close( )
{
   CVAssert(fFilePtr != 0, "Trying to close an already closed CVFile.");
   if (fFilePtr != 0)
   {
      fclose(fFilePtr);
      fFilePtr = 0;
      return CVRES_SUCCESS;
   }

   return CVRES_FILE_ALREADY_CLOSED;
}

// Read()
//    Reads length bytes into buffer from file.
//    Sets amountRead to actual number of bytes read.
//    Returns CVRES_FILE_EOF if there is no more data
//    to read after currently read buffer
CVRES CVFile::Read( void* buffer,
                    unsigned long length,
                    unsigned long& amountRead)
{
   CVRES result = CVRES_SUCCESS;

   CVAssert( fFilePtr != 0, 
             "File must be open before additional operations are performed.");
   CVAssert(buffer   != 0, "Invalid (null) buffer passed to CVFile::Read().");

   if (fFilePtr == 0)
   {
      return CVRES_FILE_MUST_BE_OPEN;
   }

   if (buffer == 0)
   {
      return CVRES_FILE_INVALID_BUFFER;
   }

   amountRead = fread(buffer,1,length,fFilePtr);

   // Check for EOF vs Read error
   if (amountRead != length)
   {
      // Note: CVRES_FILE_EOF is not an error! it's a stat. 
      // Thus, CVSUCCESS() will succeed on it, so EOF
      // must be checked explicitly.
      if (feof(fFilePtr))
         return CVRES_FILE_EOF;
      else
         return CVRES_FILE_READ_ERROR;
   }

   return CVRES_SUCCESS;
}

// ReadLine
//  Reads a text line from the file
CVRES CVFile::ReadLine( char*          buffer, 
                        unsigned long  maxLength, 
                        unsigned long& amountRead )
{  
   CVRES result   = CVRES_SUCCESS;
   amountRead     = 0;

   CVAssert(fFilePtr != 0, "File must be open first.");
   CVAssert(buffer   != 0, "Invalid (null) buffer passed to CVFile::Read().");

   if (fFilePtr == 0)
   {
      return CVRES_FILE_MUST_BE_OPEN;
   }

   if (buffer == 0)
   {
      return CVRES_FILE_INVALID_BUFFER;
   }

   char* curBuf = buffer;
   char* endBuf = buffer + maxLength;

   // Read chars from the file until either we hit EOF,
   // the buffer runs out, or we hit an 0x0a.
   unsigned long lastReadAmount = 0;

   do
   {     
      
      if (CVFAILED(result = this->Read(curBuf++,1,lastReadAmount)))
      {
         // Null terminate on previous position on error.
         *(curBuf-1) = 0;
         return result;
      }

      amountRead++;  

   } while ( (lastReadAmount) && 
              ( *(curBuf - 1) != 0x0a) && 
              (curBuf < endBuf)  ); 

   *curBuf = '\0';

   // Return result of last read
   // This may be CVRES_SUCCESS or CVRES_FILE_EOF.
   // Both are successful results!
   return result;
}


// Write
//    Writes length bytes from buffer to file
CVRES CVFile::Write( const void*    buffer,
                     unsigned long  length)
{
   CVRES result = CVRES_SUCCESS;

   CVAssert(fFilePtr != 0, "File must be open before first.");
   CVAssert(buffer   != 0, "Invalid buffer passed to CVFile::Write().");

   if (fFilePtr == 0)
   {
      return CVRES_FILE_MUST_BE_OPEN;
   }

   if (buffer == 0)
   {
      return CVRES_FILE_INVALID_BUFFER;
   }

   unsigned long amountWritten = fwrite(buffer,1,length,fFilePtr);

   // Check for EOF vs Read error
   if (amountWritten != length)
   {
      return CVRES_FILE_WRITE_ERROR;
   }

   return CVRES_SUCCESS;
}

// WriteString
//    Writes an ASCIIZ string to a file
CVRES CVFile::WriteString( const char* buffer)                    
{
   CVAssert(buffer   != 0, "Invalid buffer passed to CVFile::WriteString().");

   if (buffer == 0)
   {
      return CVRES_FILE_INVALID_BUFFER;
   }

   return this->Write(buffer, strlen(buffer));
}

// SeekAbs()
//    Seeks to absolute position
CVRES CVFile::SeekAbs( unsigned long   position)
{  
   CVAssert(   fFilePtr != 0, 
               "File must be open first.");
   if (fFilePtr == 0)
   {
      return CVRES_FILE_MUST_BE_OPEN;
   }

   if ((long)position == fseek(fFilePtr,position,SEEK_SET))
   {
      return CVRES_SUCCESS;
   }

   return CVRES_FILE_SEEK_ERROR;

}

// SeekEnd()
//    Seeks to end of file
CVRES CVFile::SeekEnd()
{
   CVAssert(fFilePtr != 0, "File must be open first.");
   if (fFilePtr == 0)
   {
      return CVRES_FILE_MUST_BE_OPEN;
   }

   fseek(fFilePtr,0,SEEK_END);

   return CVRES_SUCCESS;
}

// GetPos
//    Sets position to absolute file pos on success
CVRES CVFile::GetPos( unsigned long& position)
{
   CVRES result = CVRES_SUCCESS;
   CVAssert(fFilePtr != 0, "File must be open first.");
   if (fFilePtr == 0)
   {
      return CVRES_FILE_MUST_BE_OPEN;
   }

   // for now use ftell. Later, we should cache this and the length.
   position = ftell(fFilePtr);
   return result;
}

// Length()
//    Sets length to length of file on success
CVRES CVFile::Length( unsigned long& length)
{
   CVRES result = CVRES_SUCCESS;
   length       = 0;

   CVAssert(fFilePtr != 0, "File must be open first.");
   if (fFilePtr == 0)
   {
      return CVRES_FILE_MUST_BE_OPEN;
   }

   unsigned long orgPos = 0;
   if (CVFAILED(result = this->GetPos(orgPos)))
   {
      return result;
   }
   
   if (CVFAILED(result = this->SeekEnd()))
   {
      return result;
   }

   if (CVFAILED(result = this->GetPos(length)))
   {
      this->SeekAbs(orgPos);
      return result;
   }

   if (CVFAILED(result = this->SeekAbs(orgPos)))
   {     
      return result;
   }


   return result;
}     


//------------------------------------------------
// Static utility functions

// FileExists()
//  Just attempts to open for read when checking for existance.
//
// If converting to win32 functions, use ::GetAttributes() instead and
// double check to make sure it isn't a directory, either.
//
bool  CVFile::FileExists( const char* filename)
{
   FILE *fp = fopen(filename,"r");
   if (fp == 0)   
      return false;

   fclose(fp);
   return true;
}

