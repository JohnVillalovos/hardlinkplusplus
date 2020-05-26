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
#include <map>
#include <iostream>
#include <fstream>
#include <iterator>

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
};


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
bool hardlinkfiles( const file_info & sourcefile, const file_info & destfile );
void hardlink_identical_files( const string & filename );

class cHardLinkStats
{
    public:
	void add( const string & sourcefilename, const string & destfilename )
	{
	    hardlinkedMap.insert( pair<string, string>( sourcefilename, destfilename ) );
	}
	void print( void );

    private:
	multimap< string, string > hardlinkedMap;

};

void cHardLinkStats::print( void )
{
    if ( !hardlinkedMap.empty() )
    {
	cout << "Files Hardlinked this run:" << endl;
	multimap< string, string >::iterator pos;
	for ( pos = hardlinkedMap.begin(); pos != hardlinkedMap.end(); ++pos )
	{
	    cout << "Hardlinked: " << pos->first << endl;
	    cout << "        to: " << pos->second << endl;
	}
	cout << endl;
    }
}


class cStatistics
{
    public:
	// cStats( void ):dircount(0) {}

	void foundDirectory( void ) { ++dircount; }
	void foundRegularFile( void ) { ++regularfiles; }
	void didComparison( void ) { ++comparisons; }
	void foundHardlink( const off_t filesize  )
	{
	    ++hardlinked_previously;
	    bytes_saved_previously += filesize;
	}
	void didHardlink( const string & sourcefile, const string & destfile,
		const off_t filesize )
	{
	    ++hardlinked_thisrun;
	    bytes_saved_thisrun += filesize;
	    hardlinkstats.add( sourcefile, destfile );
	}

	void print( void );


    private:
	long long dircount;	    // how many directories we find
	long long regularfiles;	    // how many regular files we find
	long long comparisons;	    // how many file content comparisons
	long long hardlinked_thisrun;	// hardlinks done this run
	long long hardlinked_previously; // hardlinks that are already existing
	long long bytes_saved_thisrun;	// bytes saved by hardlinking this run
	long long bytes_saved_previously; // bytes saved by previous hardlinks
	cHardLinkStats hardlinkstats;	// list of files hardlinked
};

void cStatistics::print( void )
{
    cout << "\n\n";
    cout << "Hard linking Statistics:" << endl;
    // Print out the stats for the files we hardlinked, if any
    hardlinkstats.print();
    cout << "Directories           : " << dircount << endl;
    cout << "Regular files         : " << regularfiles << endl;
    cout << "Comparisons           : " << comparisons << endl;
    cout << "Hardlinked this run   : " << hardlinked_thisrun << endl;
    cout << "Total hardlinks       : " << hardlinked_previously +
	    hardlinked_thisrun << endl;
    cout << "Bytes saved this run  : " << bytes_saved_thisrun << endl;
    long long totalbytes = bytes_saved_thisrun + bytes_saved_previously;
    if ( totalbytes > 1024 * 1024 * 1024 )
    {
	cout << "Total gibibytes saved : " << totalbytes / (1024.0 * 1024 * 1024) << endl;
    }
    if ( totalbytes > 1024 * 1024 )
    {
	cout << "Total mibibytes saved : " << totalbytes / (1024.0 * 1024) << endl;
    }
    if ( totalbytes > 1024 )
    {
	cout << "Total kibibytes saved : " << totalbytes / 1024.0 << endl;
    }
    cout << "Total bytes saved     : " << totalbytes << endl;
}


#endif // #ifndef __hardlink_h_94761943
