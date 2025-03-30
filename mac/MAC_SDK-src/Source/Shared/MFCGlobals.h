#pragma once

#include <cmath>

inline CString FormatDuration(double dSeconds, bool bAddDecimal = false)
{
    if (std::isinf(dSeconds))
    {
        return _T("Unknown time");
    }

    int nHours = static_cast<int>(dSeconds) / 3600;
    dSeconds -= static_cast<double>(nHours) * 3600;

    int nMinutes = static_cast<int>(dSeconds) / 60;
    dSeconds -= static_cast<double>(nMinutes) * 60;

    int nSeconds = static_cast<int>(dSeconds);

    CString strDuration;

    if (bAddDecimal)
    {
        int nDecimal = static_cast<int>((dSeconds - static_cast<double>(nSeconds)) * 100);
        if (nHours > 0)
            strDuration.Format(_T("%d:%02d:%02d.%02d"), nHours, nMinutes, nSeconds, nDecimal);
        else
            strDuration.Format(_T("%d:%02d.%02d"), nMinutes, nSeconds, nDecimal);
    }
    else
    {
        if (nHours > 0)
            strDuration.Format(_T("%d:%02d:%02d"), nHours, nMinutes, nSeconds);
        else
            strDuration.Format(_T("%d:%02d"), nMinutes, nSeconds);
    }

    return strDuration;
}
