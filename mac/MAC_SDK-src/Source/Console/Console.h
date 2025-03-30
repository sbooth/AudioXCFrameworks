#pragma once

#include "All.h"

// functions
#ifndef PLATFORM_WINDOWS
int main(int argc, char * argv[]);
#else
int _tmain(int argc, TCHAR * argv[]);
#endif
