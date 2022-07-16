/*
 *  Opens a communication channel to another program using unnamed pipe mechanism and stdin/stdout.
 *
 *  (C) Frank Klemm 2001,02. All rights reserved.
 *
 *  Principles:
 *
 *  History:
 *    2001          created
 *    2002
 *
 *  Global functions:
 *    - pipeopen
 *
 *  TODO:
 *    -
 */

#include <mpc/mpc_types.h>
#include <ctype.h>

#include "mpcenc.h"


/*
 *
 */

static int
EscapeProgramPathName ( const char*  longprogname,
                        char*        escaped,
                        size_t       len )
{
    int   ret = 0;

#ifdef _WIN32
    ret = GetShortPathName ( longprogname, escaped, len );
#else
    if ( strlen (longprogname) <= len-3 )
        ret = sprintf ( escaped, "\"%s\"", longprogname );      // Note that this only helps against spaces and some similar things in the file name, not against all strange stuff
#endif

    if ( ret <= 0  ||  ret >= (int)len ) {
    }

    return ret;
}


/*
 *
 */

static FILE*
OpenPipeWhenBinaryExist ( const char*  path,
                          size_t       pathlen,
                          const char*  executable_filename,
                          const char*  command_line )
{
    char   filename [4096];
    char   cmdline  [4096];
    char*  p  = filename;
    FILE*  fp;

    for ( ; *path  &&  pathlen--; path++ )
        if ( *path != '"' )
            *p++ = *path;
    *p++ = PATH_SEP;
    strcpy ( p, executable_filename );
#ifdef DEBUG2
    stderr_printf ("Test for file %s        \n", filename );
#endif
    fp = fopen ( filename, "rb" );
    if ( fp != NULL ) {
        fclose ( fp );
        EscapeProgramPathName ( filename, cmdline, sizeof cmdline );
        strcat ( cmdline, command_line );
        fp = POPEN_READ_BINARY_OPEN ( cmdline );
#ifdef DEBUG2
        stderr_printf ("Executed %s\n", cmdline );
#endif
   }
   return fp;
}


/*
 *
 */

static FILE*
TracePathList ( const char*  p,
                const char*  executable_filename,
                const char*  command_line )
{
    const char*  nextsep;
    FILE*        fp;

    while ( p != NULL   &&  *p != '\0' ) {
        if ( (nextsep = strchr (p, ENVPATH_SEP)) == NULL ) {
            fp = OpenPipeWhenBinaryExist ( p, (size_t)         -1, executable_filename, command_line );
            p  = NULL;
        }
        else {
            fp = OpenPipeWhenBinaryExist ( p, (size_t)(nextsep-p), executable_filename, command_line );
            p  = nextsep + 1;
        }
        if ( fp != NULL )
            return fp;
    }
    return NULL;
}


/*
 *  Executes command line given by command.
 *  The command must be found in some predefined paths or in the ${PATH} aka %PATH%
 *  The char # in command is replaced by the contents
 *  of filename. Special characters are escaped.
 */

FILE*
pipeopen ( const char* command, const char* filename )
{
    static const char  pathlist [] =
#ifdef _WIN32
        ".";
#else
        "/usr/bin:/usr/local/bin:/opt/mpp:.";
#endif
    char          command_line        [4096];           //  -o - bar.pac
    char          executable_filename [4096];           // foo.exe
    char*         p;
    const char*   q;
    FILE*         fp;

    // does the source file exist and is readble?
    if ( (fp = fopen (filename, "rb")) == NULL ) {
        stderr_printf ("file '%s' not found.\n", filename );
        return NULL;
    }
    fclose (fp);

    // extract executable name from the 'command' to executable_filename, append executable extention
    p = executable_filename;
    for ( ; *command != ' '  &&  *command != '\0'; command++ )
        *p++ = *command;
    strcpy ( p, EXE_EXT );


    // Copy 'command' to 'command_line' replacing '#' by filename
    p = command_line;
    for ( ; *command != '\0'; command++ ) {
        if ( *command != '#' ) {
            *p++ = *command;
        }
        else {
            q = filename;
            if (*q == '-') {
                *p++ = '.';
                *p++ = PATH_SEP;
            }
#ifdef _WIN32                           // Windows secure Way to "escape"
            *p++ = '"';
            while (*q)
                *p++ = *q++;
            *p++ = '"';
#else                                   // Unix secure Way to \e\s\c\a\p\e
            while (*q) {
                if ( !isalnum(*q)  &&  *q != '.'  &&  *q != '-'  &&  *q != '_'  &&  *q != '/' )
                    *p++ = '\\';
                *p++ = *q++;
            }
#endif
        }
    }
    *p = '\0';

    // Try the several built-in paths to find binary
    fp = TracePathList ( pathlist       , executable_filename, command_line );
    if ( fp != NULL )
        return fp;

    // Try the PATH settings to find binary (Why we must search for the executable in all PATH settings? --> popen itself do not return useful information)
    fp = TracePathList ( getenv ("PATH"), executable_filename, command_line );
    if ( fp != NULL )
        return fp;

#ifdef DEBUG2
    stderr_printf ("Nothing found to execute.\n" );
#endif
    return NULL;
}

/* end of pipeopen.c */
