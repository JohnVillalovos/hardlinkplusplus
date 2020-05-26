/* hardlink - Goes through a directory structure and creates hardlinks for
   files which are identical

   Copyright (C) 2003  John L. Villalovos

   This program is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by the Free
   Software Foundation; either version 2 of the License, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
   more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc., 59
   Temple Place, Suite 330, Boston, MA  02111-1307, USA.

   John Villalovos
   email: software@sodarock.com
   http://www.sodarock.com/
*/

#ifndef __hardlink_h_94761943
#define __hardlink_h_94761943

using namespace std;


// C++ includes
#include <string>
#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <iterator>

//  #include <sys/types.h>
//  #include <stdlib.h>
//  #include <stdio.h>
//  #include <sys/mman.h>
//  #include <string.h>
//  #include <fcntl.h>

// C includes
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>



// Global structs

// Structure to hold the file info we care about
struct file_info
{
    string filename;     // file name
    struct stat stat_info;  // file status info
}
;


// Global constants
const int MAX_HASHES = 128 * 1024;
const string::size_type RSYNC_WORKFILE_EXTENSION_SIZE = 6;


// declarations
inline unsigned int hash( off_t size, time_t mtime );
inline bool isAlreadyHardlinked( const struct stat & st1, const struct stat & st2 );
inline bool eligibleForHardlink( const struct stat & st1, const struct stat & st2 );
bool getFileStatus( const string & filename, struct stat & st );
bool areFileContentsEqual( const string & filename1, const string & filename2 );
bool areFilesEqual( const file_info & file1, const file_info & file2 );
void doexit( int i );
bool hardlinkfiles( const file_info & sourcefile, const file_info & destfile );
void hardlink_identical_files( const string & filename );

#endif // #ifndef __hardlink_h_94761943
