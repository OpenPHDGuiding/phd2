/// \file CVFile.h
/// \brief CVFile.h defines file class CVFile
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
/// \class CVFile CVFile.h
/// CVFile is a simple file class that should be useable on most platforms.
/// 
/// It does not support huge files, file attributes, or really anything
/// aside from basic read and write operations.
/// CVFile uses FILE*'s for all operations.
///
/// $RCSfile: CVFile.h,v $
/// $Date: 2004/02/08 23:47:38 $
/// $Revision: 1.1.1.1 $
/// $Author: mikeellison $

#ifndef _CVFILE_H_
#define _CVFILE_H_

#include <stdio.h>
#include "CVResFile.h"

class CVFile
{
   public:
      CVFile();
      virtual ~CVFile();

      /// OpenForRead opens a file for reading, and fails 
      /// if it does not exist.
      /// You must call Close() on the file when done.
      ///
      /// \param filename - ASCIIZ pointer to filename to open.
      /// \return CVRES result code
      /// \sa OpenForReadWrite(), Create(), Close()
      /// \sa CVRes.h, CVResFile.h
      CVRES OpenForRead       (  const char*    filename );
      
      /// OpenForReadWrite() opens or creates a file for 
      /// reading and writing.  If the file exists, it opens
      /// it without truncation and sets the file pointer
      /// at the start of the file.  If the file does not
      /// exist, then a new file is created.
      /// 
      /// You must call Close() on the file when done.
      ///
      /// \param filename - ASCIIZ pointer to filename to open.
      /// \return CVRES result code
      /// \sa OpenForRead(), Create(), Close()
      /// \sa CVRes.h, CVResFile.h
      CVRES OpenForReadWrite  (  const char*    filename );
      
      /// Create() creates a new file. 
      /// If a file of the same name exists, the file
      /// is deleted.
      /// You must call Close() on the file when done.
      ///
      /// \param filename - ASCIIZ pointer to filename to open.
      /// \return CVRES result code
      /// \sa OpenForRead(), Create(), Close()
      /// \sa CVRes.h, CVResFile.h
      CVRES Create            (  const char*    filename );
      
      /// Close() closes a previously opened file.
      /// 
      /// \return CVRES result code.
      /// \sa OpenForRead(), OpenForReadWrite(), Create()
      /// \sa CVRes.h, CVResFile.h
      CVRES Close             (  );

      /// Read() reads the specified amount from the file.
      /// If EOF is reached it returns CVRES_FILE_EOF,
      /// which is NOT an error. CVSUCCESS() will treat it
      /// as a successful result, so you may need to check
      /// for it explicitly if you need to know about
      /// EOF conditions.  The file must have been previously
      /// opened.
      /// 
      /// \param buffer - destination buffer for read. Must
      ///                 be at least as long as length.
      /// \param length - maximum length of read
      /// \param amountRead - set to the amount read on return.
      /// \return CVRES result code.
      /// 
      /// \sa ReadLine(), Write(), WriteString()
      /// \sa CVRes.h, CVResFile.h
      CVRES Read              (  void*          buffer,
                                 unsigned long  length,
                                 unsigned long& amountRead);

      /// ReadLine() reads a line up to a carriage return, or
      /// the maxLength of buffer specified, whichever is
      /// shorter.
      /// 
      /// \param buffer - destination buffer for read. Must
      ///                 be at least as long as length.
      /// \param maxLength - maximum length of read
      /// \param amountRead - set to the amount read on return.
      /// \return CVRES result code.
      /// 
      /// \sa Read(), Write(), WriteString()
      /// \sa CVRes.h, CVResFile.h
      CVRES ReadLine          (  char*          buffer,
                                 unsigned long  maxLength,
                                 unsigned long& amountRead);

      /// Write() writes the specified amount from buffer to the file.
      ///
      /// \param buffer - source buffer to write from. Must be 
      ///                 be at least as long as writeLength.
      /// \param writeLength - length of buffer to write to the file.
      /// \return CVRES result code.
      ///
      /// \sa Read(), WriteString()
      /// \sa CVRes.h, CVResFile.h
      CVRES Write             (  const void*    buffer,
                                 unsigned long  writeLength);

      /// WriteString() writes the string passed in strBuffer to
      /// the file. It does not write the terminating NULL char.
      ///
      /// \param strBuffer - ASCIIZ (null-terminated) string to write.
      /// \return CVRES result code.
      /// \sa Read(), ReadLine(), Write()
      /// \sa CVRes.h, CVResFile.h
      CVRES WriteString       (  const char*    strBuffer);

      /// SeekAbs() sets the file pointer to an absolute position 
      /// from the start of the file.
      /// \param position - position in bytes from the start of the
      ///        file to seek to.
      /// \return CVRES result code.
      /// \sa SeekEnd(), GetPos(), CVRes.h, CVResFile.h
      CVRES SeekAbs           (  unsigned long  position);

      /// SeekEnd() sets the file pointer to the end of the file.      
      /// \return CVRES result code.
      /// \sa SeekAbs(), GetPos(), CVRes.h, CVResFile.h
      CVRES SeekEnd           (  );

      /// GetPos() returns the absolute position of the file pointer
      /// within the file.
      /// \param position - set to file position in bytes on return.
      /// \return CVRES result code.
      /// \sa SeekAbs(), SeekEnd(), Length(), CVRes.h, CVResFile.h
      CVRES GetPos            (  unsigned long& position);

      /// Retrieves the length of the file in bytes.
      /// \param length - the length of the file in bytes.
      /// \return CVRES result code.
      /// \sa SeekAbs(), SeekEnd(), GetPos(), CVRes.h, CVResFile.h
      CVRES Length            (  unsigned long& length);    
      
      //----------------------------------------------------
      // Static utility functions

      /// FileExists() returns true if the file exists.
      /// \param filename - ASCIIZ string containing file name.
      /// \return bool - true if file exists, false otherwise.
      /// \sa OpenForRead(), OpenForReadWrite(), Create()      
      static bool FileExists  ( const char*     filename);
      //----------------------------------------------------
   protected:
      FILE* fFilePtr;
};

#endif // _CVFILE_H_

