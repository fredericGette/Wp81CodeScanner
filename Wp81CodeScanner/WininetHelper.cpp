#include "pch.h"
#include "WininetHelper.h"

void WininetHelper::Helper::Debug(const char* format, ...)
{
	va_list args;
	va_start(args, format);

	char buffer[100];
	vsnprintf_s(buffer, sizeof(buffer), format, args);

	OutputDebugStringA(buffer);

	va_end(args);
}

std::string WininetHelper::Helper::getHResultErrorMessage(int HResult)
{
	//https://github.com/mic101/windows/blob/master/WRK-v1.2/public/sdk/inc/winerror.h
	int severityCode = (HResult >> 30) & 0x3;
	int facilityCode = (HResult >> 16) & 0xFFF;
	int facilityCodeStatus = HResult & 0xFFFF;
	Debug("Severity code %u\n", severityCode);
	Debug("Facility code %u\n", facilityCode);
	Debug("Facility's status code %u\n", facilityCodeStatus);

	std::string result = "";

	// Facility Win32
	if (facilityCode == 7) {
		switch (facilityCodeStatus) {
		case ERROR_INTERNET_NAME_NOT_RESOLVED:
			result = "ERROR_INTERNET_NAME_NOT_RESOLVED";
			break;
		case ERROR_INTERNET_CANNOT_CONNECT:
			result = "ERROR_INTERNET_CANNOT_CONNECT";
			break;
		}
	}

	return result;
}
