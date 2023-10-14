#pragma once

namespace APE
{

/**************************************************************************************************
Query which optimizations are compiled in
**************************************************************************************************/
bool GetSSE2Available();
bool GetSSE41Available();
bool GetAVX2Available();
bool GetAVX512Available();

bool GetNeonAvailable();

/**************************************************************************************************
Test for supported CPU features
**************************************************************************************************/
bool GetSSE2Supported();
bool GetSSE41Supported();
bool GetAVX2Supported();
bool GetAVX512Supported();

bool GetNeonSupported();

}
