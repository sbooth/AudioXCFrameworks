/*
 *  stderr - Message output system
 *
 *  (C) Frank Klemm, Janne Hyv�inen 2002. All rights reserved.
 *
 *  Principles:
 *
 *  History:
 *    2001              created
 *    2002 Spring       added functionality to switch on and off printing to easily allow silent modes
 *    2002-10-10        Escape sequence handling for Windows added.
 *
 *  Global functions:
 *    - SetStderrSilent()
 *    - GetStderrSilent()
 *    - stderr_printf()
 *
 *  TODO:
 *    -
 */

#include <mpc/mpc_types.h>

#ifdef _WIN32
# include <windows.h>
#endif

#include <stdio.h>
#include <stdarg.h>
// #include "mpcenc.h"

#define WRITE(fp,ptr,len)      fwrite (ptr, 1, len, fp)     // WRITE   returns -1 or 0 on error/EOF, otherwise > 0

static mpc_bool_t  stderr_silent = 0;


void
SetStderrSilent ( mpc_bool_t state )
{
    stderr_silent = state;
}


mpc_bool_t
GetStderrSilent ( void )
{
    return stderr_silent;
}


int mpc_cdecl
stderr_printf ( const char* format, ... )
{
    char     buff [2 * 1024 + 3072];
    char*    p = buff;
    int      ret;
    va_list  v;

    /* print to a buffer */
    va_start ( v, format );
    ret = vsprintf ( p, format, v );
    va_end ( v );

    if ( !stderr_silent ) {

#if   defined __unix__  ||  defined __UNIX__

        WRITE ( stderr, buff, ret );

#elif defined _WIN32

# define FOREGROUND_ALL         ( FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED )
# define BACKGROUND_ALL         ( BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED )

        // for Windows systems we must merge carriage returns into the stream to avoid staircases
        // Also escape sequences must be detected and replaced (incomplete now)

        char                            buff [128], *q;
        static int                      init = 0;
        CONSOLE_SCREEN_BUFFER_INFO      con_info;
        static HANDLE                   hSTDERR;
        static WORD                     attr;
        static WORD                     attr_initial;
        DWORD                           written;

        if ( init == 0 ) {
            hSTDERR = GetStdHandle ( STD_ERROR_HANDLE );
            attr    = hSTDERR == INVALID_HANDLE_VALUE  ||  GetConsoleScreenBufferInfo ( hSTDERR, &con_info ) == 0
                      ?  FOREGROUND_ALL  :  con_info.wAttributes;
            attr_initial = attr;
            init    = 1;
        }

        if ( hSTDERR == INVALID_HANDLE_VALUE ) {
            while ( ( q = strchr (p, '\n')) != NULL ) {
                WRITE ( stderr, p, q-p );
                WRITE ( stderr, "\r\n", 2 );
                p = q+1;
            }
            WRITE ( stderr, p, strlen (p) );
        }
        else {
            for ( ; *p; p++ ) {
                switch ( *p ) {
                case '\n':
                    SetConsoleTextAttribute ( hSTDERR, attr_initial );
                    fprintf ( stderr, "\r\n" );
                    SetConsoleTextAttribute ( hSTDERR, attr );
                    break;

                case '\x1B':
                    if ( p[1] == '[' ) {
                        unsigned int  tmp;

                        p++;
                cont:   p++;
                        for ( tmp = 0; (unsigned int)( *p - '0' ) < 10u; p++ )
                            tmp = 10 * tmp + ( *p - '0' );

                        switch ( *p ) {
                        case ';':
                        case 'm':
                            switch ( tmp ) {
                            case  0: attr  =  FOREGROUND_ALL;                                                   break; // reset defaults
                            case  1: attr |=  FOREGROUND_INTENSITY;                                             break; // high intensity on
                            case  2: attr &= ~FOREGROUND_ALL; attr |= FOREGROUND_INTENSITY;                     break; // (very) low intensity
                            case  3:                                                                            break; // italic on
                            case  4:                                                                            break; // underline on
                            case  5:                                                                            break; // blinking on
                            case  7:                                                                            break; // reverse
                            case  8: attr  =  0;                                                                break; // invisible
                            case 30: attr &= ~FOREGROUND_ALL;                                                   break;
                            case 31: attr &= ~FOREGROUND_ALL; attr |= FOREGROUND_RED;                           break;
                            case 32: attr &= ~FOREGROUND_ALL; attr |= FOREGROUND_GREEN;                         break;
                            case 33: attr &= ~FOREGROUND_ALL; attr |= FOREGROUND_RED | FOREGROUND_GREEN;        break;
                            case 34: attr &= ~FOREGROUND_ALL; attr |= FOREGROUND_BLUE;                          break;
                            case 35: attr &= ~FOREGROUND_ALL; attr |= FOREGROUND_RED | FOREGROUND_BLUE;         break;
                            case 36: attr &= ~FOREGROUND_ALL; attr |= FOREGROUND_GREEN | FOREGROUND_BLUE;       break;
                            case 37: case 39:                 attr |= FOREGROUND_ALL;                           break;
                            case 40: case 49:
                                     attr &= ~BACKGROUND_ALL;                                                   break;
                            case 41: attr &= ~BACKGROUND_ALL; attr |= BACKGROUND_RED;                           break;
                            case 42: attr &= ~BACKGROUND_ALL; attr |= BACKGROUND_GREEN;                         break;
                            case 43: attr &= ~BACKGROUND_ALL; attr |= BACKGROUND_RED | BACKGROUND_GREEN;        break;
                            case 44: attr &= ~BACKGROUND_ALL; attr |= BACKGROUND_BLUE;                          break;
                            case 45: attr &= ~BACKGROUND_ALL; attr |= BACKGROUND_RED | BACKGROUND_BLUE;         break;
                            case 46: attr &= ~BACKGROUND_ALL; attr |= BACKGROUND_GREEN | BACKGROUND_BLUE;       break;
                            case 47:                          attr |= BACKGROUND_ALL;                           break;
                            }
                            SetConsoleTextAttribute ( hSTDERR, attr );
                            if ( *p == ';' )
                                goto cont;
                            break;

                        default:
                            WriteFile ( hSTDERR, buff, sprintf ( buff, "Unknown escape sequence ending with '%c'\n", *p ), &written, NULL );
                            break;
                        }
                        break;
                    }
                default:
                    fputc ( *p, stderr );
                    break;
                }
            } /* end for */
        }

#else
		char * q;
        // for non-Unix systems we must merge carriage returns into the stream to avoid staircases
        while ( ( q = strchr (p, '\n')) != NULL ) {
            WRITE ( stderr, p, q-p );
            WRITE ( stderr, "\r\n", 2 );
            p = q+1;
        }
        WRITE ( stderr, p, strlen (p) );

#endif

    }

    return ret;
}

/* end of stderr.c */
