/*
 *  Keyboard input functions
 *
 *  (C) Frank Klemm 2002. All rights reserved.
 *
 *  Principles:
 *
 *  History:
 *    ca. 1998    created
 *    2002
 *
 *  Global functions:
 *    -
 *
 *  TODO:
 *    -
 */

#include "mpcenc.h"

#if defined _WIN32  ||  defined __TURBOC__

# include <conio.h>

int
WaitKey ( void )
{
    return getch ();
}

int
CheckKeyKeep ( void )
{
    int  ch;

    if ( !kbhit () )
        return -1;

    ch = getch ();
    ungetch (ch);
    return ch;
}

int
CheckKey ( void )
{
    if ( !kbhit () )
        return -1;

    return getch ();
}

#else

# ifdef USE_TERMIOS
#  include <termios.h>

static struct termios  stored_settings;

static void
echo_on ( void )
{
    tcsetattr ( 0, TCSANOW, &stored_settings );
}

static void
echo_off ( void )
{
    struct termios  new_settings;

    tcgetattr ( 0, &stored_settings );
    new_settings = stored_settings;

    new_settings.c_lflag     &= ~ECHO;
    new_settings.c_lflag     &= ~ICANON;        // Disable canonical mode, and set buffer size to 1 byte
    new_settings.c_cc[VTIME]  = 0;
    new_settings.c_cc[VMIN]   = 1;

    tcsetattr ( 0, TCSANOW, &new_settings );
}

# else
#  define echo_off()  (void)0
#  define echo_on()   (void)0
# endif

int
WaitKey ( void )
{
    unsigned char  buff [1];
    int            ret;

    echo_off ();
    ret = read ( 0, buff, 1 );
    echo_on ();
    return ret == 1  ?  buff[0]  :  -1;
}

int
CheckKeyKeep ( void )
{
    struct timeval  tv = { 0, 0 };      // Do not wait at all, not even a microsecond
    fd_set          read_fd;

    FD_ZERO ( &read_fd );               // Must be done first to initialize read_fd
    FD_SET ( 0, &read_fd );             // Makes select() ask if input is ready;  0 is file descriptor for stdin

    if ( -1 == select ( 1,              // number of the largest fd to check + 1
                        &read_fd,
                        NULL,           // No writes
                        NULL,           // No exceptions
                        &tv ) )
        return -1;                      // an error occured

    return FD_ISSET (0, &read_fd)  ?  0xFF  :  -1;   // read_fd now holds a bit map of files that are readable. We test the entry for the standard input (file 0).
}

int
CheckKey ( void )
{
    if ( CheckKeyKeep () < 0 )
        return -1;
    return WaitKey ();
}

#endif

/* end of keyboard.c */
