/*
 * Musepack audio compression
 * Copyright (C) 1999-2004 Buschmann/Klemm/Piecha/Wolf
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "mpcenc.h"

#ifdef _WIN32

#include <windows.h>

static HWND  FrontEndHandle;


int
SearchForFrontend ( void )
{
    FrontEndHandle = FindWindow ( NULL, "mpcdispatcher" );      // check for dispatcher window and (send startup-message???)

    return FrontEndHandle != 0;
}


static void
SendMsg ( const char* s )
{
    COPYDATASTRUCT  MsgData;

    MsgData.dwData = 3;         // build message
    MsgData.lpData = (char*)s;
    MsgData.cbData = strlen(s) + 1;

    SendMessage ( FrontEndHandle, WM_COPYDATA, (WPARAM) NULL, (LPARAM) &MsgData );  // send message
}


void
SendStartupMessage ( const char*  Version,
                     const int    SV)
{
    char  startup [120];

    sprintf ( startup, "#START#MP+ v%s SV%i", Version, SV);   // fill startup-message
    SendMsg ( startup );
}


void
SendQuitMessage ( void )
{
    SendMsg ("#EOF#");
}


void
SendModeMessage ( const int Profile )
{
    char  message [32];

    sprintf ( message, "#PARAM#%d#", Profile-8 );  // fill message
    SendMsg ( message );
}


void                                            /* sends progress information to the frontend */
SendProgressMessage ( const int    bitrate,
                      const float  speed,
                      const float  percent )
{
    char  message [64];

    sprintf ( message, "#STAT#%4ik %5.2fx %5.1f%%#", bitrate, speed, percent );
    SendMsg ( message );
}

#endif /* _WIN32 */
