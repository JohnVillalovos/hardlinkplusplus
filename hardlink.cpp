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
   email: john@sodarock.com
   http://www.sodarock.com/


   Idea for this program came from hardlink.c code.  I liked what it did but
   did not like the code itself.  To me it was very unmaintainable.  So I
   converted hardlink.c to hardlink.cpp.
*/

#include "hardlink.h"

const string HARDLINK_VERSION = "0.02";

const bool debug = true;

const bool debug1 = false;

// Vector of dirs that we need to go through
vector<string> dirs;
// Vector of all of our files that we have info about
vector< vector<file_info> > file_vector( MAX_HASHES );

// statistics globals
cStatistics gStats;


int main( int argc, char **argv )
{
    // See if they passed in any parameters
    if ( argc < 2 )
    {
	cout << "hardlink++ version " << HARDLINK_VERSION;
	cout << ", Copyright (C) 2003 John L. Villalovos" << endl;
	cout << "Contact: software@sodarock.com" << endl;
	cout << "hardlink++ comes with ABSOLUTELY NO WARRANTY; for details" << endl;
	cout << "see the COPYING file.  This is free software, and you are" << endl;
	cout << "welcome to redistribute it under certain conditions; see" << endl;
	cout << "the COPYING file for details." << endl;
	cout << endl;
	cout << "This program will recurse through all the directories you" << endl;
	cout << "specify and try to find files which can be hardlinked" << endl;
	cout << "together to save disk space.  This is most useful when" << endl;
	cout << "you are a mirror for ftp sites.  Many times there will be" << endl;
	cout << "duplicate packages on a mirror site and this can be a" << endl;
	cout << "great savings of disk space" << endl;
	cout << "Usage:" << endl;
	cout << "  hardlink directory_name [ directory_name ... ]" << endl;
	exit(1);
    }

    // We need to use something like popt for this

    // Add all our directories to the list
    for ( int i = 1; i < argc; i++ )
    {
	dirs.push_back( argv[ i ] );
    }

    // Now go through all the directories that have been added.
    // NOTE: hardlink_identical_files() will add more directories to the global
    //	     dirs vector as it finds them.
    while ( ! dirs.empty() )
    {
        // Get the last directory in the list
        string dirname = dirs.back();
        // Delete the last directory in the list
        dirs.pop_back();
        dirname += "/";
        // Open the directory
        DIR * dir_handle = opendir ( dirname.c_str() );
        if ( NULL == dir_handle )
        {
            continue;
        }
        // Bump directory count
	gStats.foundDirectory();

        struct dirent *dir_entry;
        // Loop through all the files in the directory
        while ( ( dir_entry = readdir( dir_handle ) ) != NULL )
        {
            string entry_name = dir_entry->d_name;
            if ( entry_name.empty() )
            {
                continue;
            }
            // Look at files/dirs beginning with "."
            if ( '.' == entry_name[ 0 ] )
            {
                // Continue on if we have the "." or ".." entries
                if ( ( "." == entry_name ) || ( ".." == entry_name ) )
                {
                    continue;
                }
                // Ignore any mirror.pl files.  These are the files that start with
                // ".in."
                if ( entry_name.find( ".in." ) == 0 )
                {
                    cout << "Skipping mirror.pl work file: " << entry_name << endl;
                    continue;
                }
                // Ignore any RSYNC files.  These are files that have the
                // format .FILENAME.??????
                {
                    // Find the last "." in the file name
                    string::size_type location = entry_name.rfind( "." );
                    // Make sure that:
                    // *   We don't find the first "." in the file
                    // *   There are six characters after the "."
                    if ( ( 0 != location ) && ( RSYNC_WORKFILE_EXTENSION_SIZE
                                                == ( entry_name.size() - location - 1 ) ) )
                    {
                        cout << "Skipping rsync work file: " << entry_name << endl;
                        continue;
                    }
                }
            }
            string work_file = dirname + entry_name;
            hardlink_identical_files( work_file );
        }
        closedir( dir_handle );
    }

    /*
        // Debug output.
        for (unsigned int i = 0; i < file_vector.size(); ++i)
        {
     if ( !file_vector[i].empty() )
     {
         cout << i << endl;
         for (unsigned int j=0; j < file_vector[i].size(); ++j)
         {
      cout << "\t" << file_vector[i][j].filename << endl;
         }
     }
        }
    */

    // Print out our statistics
    gStats.print();
    return 0;
}


// Create a hash from a file's size and time values
inline unsigned int hash( off_t size, time_t mtime )
{
    return ( size ^ mtime ) & ( MAX_HASHES - 1 );
}


// If two files have the same inode and are on the same device then they are
// already hardlinked.
bool isAlreadyHardlinked(
    const struct stat & st1,     // first file's status
    const struct stat & st2 )    // second file's status
{
    bool result = (
                      ( st1.st_ino == st2.st_ino ) &&  // Inodes equal
                      ( st1.st_dev == st2.st_dev )     // Devices equal
                  );
    return result;
}


// Determine if a file is eligibile for hardlinking.  Files will only be
// considered for hardlinking if this function returns true.
bool eligibleForHardlink(
    const struct stat & st1,     // first file's status
    const struct stat & st2 )    // second file's status
{
    return
        (
            // Must meet the following
            // criteria:
            ( !isAlreadyHardlinked( st1, st2 ) ) &&  // NOT already hard linked
            ( st1.st_mode == st2.st_mode ) &&  // file mode is the same
            ( st1.st_uid == st2.st_uid ) &&  // owner user id is the same
            ( st1.st_gid == st2.st_gid ) &&  // owner group id is the same
            ( st1.st_size == st2.st_size ) &&  // size is the same
            ( st1.st_mtime == st2.st_mtime ) &&  // modified time is the same
            ( st1.st_dev == st2.st_dev )           // device is the same
        );
}

bool getFileStatus( const string & filename, struct stat & st )
{
    // Get the file status
    if ( !lstat ( filename.c_str(), &st ) )
    {
        // success
        return true;
    }
    return false;
}

// Determine if the contents of two files are equal.
//
// **!! This function assumes that the file sizes of the two files are equal.
bool areFileContentsEqual( const string & filename1, const string & filename2 )
{
    bool result = false;

    // Open our two files
    ifstream file1( filename1.c_str() );
    ifstream file2( filename2.c_str() );
    // Make sure open succeeded
    if ( ! ( file1 && file2 ) )
    {
        cerr << "Error opening file in areFileContentsEqual" << endl;
        cerr << "Was attempting to open:" << endl;
        cerr << "file1: " << filename1 << endl;
        cerr << "file2: " << filename2 << endl;
        result = false;
    }
    else
    {
        cout << "Comp: " << filename1 << endl;
        cout << "  to: " << filename2 << endl;
        // Handy typedef
        typedef istream_iterator<char> char_input;
        // Use the STL algorithm EQUAL to determine if the contents of the two
        // files are equal.
        result = equal( char_input( file1 ), char_input(),
                        char_input( file2 ) );

    }
    gStats.didComparison();
    return result;
}

// Determines if two files should be hard linked together.
bool areFilesHardlinkable( const file_info & file1, const file_info & file2 )
{
    bool result = false;
    // See if the files are eligible for hardlinking
    if ( eligibleForHardlink( file1.stat_info, file2.stat_info ) )
    {
        // Now see if the contents of the file are the same.  If they are then
        // these two files should be hardlinked.
        result = areFileContentsEqual( file1.filename, file2.filename );
    }
    else
    {
        result = false;
    }
    return result;
}


// Hardlink two files together
bool hardlinkfiles( const file_info & sourcefile, const file_info & destfile )
{
    bool result = false;

    // rename the destination file to save it
    string temp_name = destfile.filename + ".$$$___cleanit___$$$";
    if ( rename ( destfile.filename.c_str(), temp_name.c_str() ) )
    {
        cerr << "Failed to rename " << destfile.filename << " to " << temp_name << endl;
        result = false;
    }
    else
    {
        // Now link the sourcefile to the destination file
        if ( link ( sourcefile.filename.c_str(), destfile.filename.c_str() ) )
        {
            cerr << "Failed to hardlink " << sourcefile.filename << " to " << destfile.filename << endl;
            // Try to recover
            if ( rename ( temp_name.c_str(), destfile.filename.c_str() ) )
            {
                cerr << "BAD BAD - failed to rename back " << temp_name << " to " << destfile.filename << endl;
            }
            result = false;
        }
        else
            // hard link succeeded
        {
            // Delete the renamed version since we don't need it.
            unlink ( temp_name.c_str() );
            // update our stats
	    gStats.didHardlink( sourcefile.filename, destfile.filename,
		    sourcefile.stat_info.st_size );
            cout << "Linked " << sourcefile.filename << " to " <<
            destfile.filename << ", saved " << sourcefile.stat_info.st_size
            << endl;
            result = true;
        }
    }
    return result;
}


void hardlink_identical_files( const string & filename )
{
    // The purpose of this function is to hardlink files together if the files
    // are the same.  To be considered the same they must be equal in the
    // following criteria:
    //       * file mode
    //       * owner user id
    //       * owner group id
    //       * file size
    //       * modified time
    //       * file contents
    //
    // Also, files will only be hardlinked if they are on the same device.
    // This is because hardlink does not allow you to hardlink across file
    // systems.

    // The basic idea on how this is done is as follows:
    //
    //     Walk the directory tree building up a list of the files.
    //
    //  For each file, generate a simple hash based on the size and
    //  modified time.
    //
    //  For any other files which share this hash make sure that they
    //  are not identical to this file.  If they are identical than
    //  hardlink the files.
    //
    //  Add the file info to the list of files that have the same hash
    //  value.

    // Create our structures to hold our file info
    struct stat stat_info;
    // Get the file status info
    if ( !getFileStatus( filename, stat_info ) )
    {
        // We didn't get the file status info :(
        return ;
    }
    // Is it a directory?
    if ( S_ISDIR ( stat_info.st_mode ) )
        // If it is a directory then add it to the vector
    {
        if ( debug1 )
            cout << "Dir : " << filename << endl;
        // Add this to our vector of directories
        dirs.push_back( filename );
    }
    // Is it a regular file?
    else if ( S_ISREG ( stat_info.st_mode ) )
    {
        // Create the hash for the file
        unsigned int file_hash = hash ( stat_info.st_size, stat_info.st_mtime );
        // Bump count of regular files found
        gStats.foundRegularFile();

        cout << "File: " << filename << endl;

        file_info work_file_info;
        work_file_info.filename = filename;
        work_file_info.stat_info = stat_info;

        // Our iterator
        vector<file_info>::iterator pos = file_vector[ file_hash ].begin();
        // See if anything matches us already
        bool found_match = false;
        while ( pos < file_vector[ file_hash ].end() )
        {
            if ( debug1 )
                cout << "Hash Match:" << ( *pos ).filename << endl;
            // Check if we are already hard linked to this file
            if ( isAlreadyHardlinked( stat_info, ( *pos ).stat_info ) )
            {
                if ( debug1 )
                    cout << "Files already hardlinked" << endl;
                // Don't save it since we already have it, via hardlink, in the
                // vector
                found_match = true;
		gStats.foundHardlink( stat_info.st_size );
                break;
            }
            // See if the files should be hardlinked
            if ( areFilesHardlinkable( work_file_info, ( *pos ) ) )
            {
                found_match = true;
                // Hard link the files
                if ( !hardlinkfiles( *pos, work_file_info ) )
                {
                    cout << "Hardlink failed :(" << endl;
                    return ;
                }
                break;
            }
            ++pos;
        }
        // If we hadn't found a match to this before then add it to the vector
        if ( !found_match )
        {
            file_vector[ file_hash ].push_back( work_file_info );
        }
    }
} // hardlink_identical_files
