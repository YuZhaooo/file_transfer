#pragma once

#include <iomanip>
#include <ostream>
#include <string>

struct DMSSimFlushLogMarker {
};

constexpr DMSSimFlushLogMarker FL;

/**
 * Logging subsystem.
 * Outputs log messages to the log file, or screen, if enabled.
 */
class DMSSimLog{
public:

	static void EnableConsoleOutput(bool Enable);
	static void EnableScreenOutput(bool Enable);

	static DMSSimLog& Debug();
	static DMSSimLog& Info();
	static DMSSimLog& Warn();
	static DMSSimLog& Error();
	static DMSSimLog& Fatal();

	DMSSimLog& operator << (bool val) noexcept;
	DMSSimLog& operator << (short val) noexcept;
	DMSSimLog& operator << (unsigned short val) noexcept;
	DMSSimLog& operator << (int val) noexcept;
	DMSSimLog& operator << (unsigned int val) noexcept;
	DMSSimLog& operator << (long val) noexcept;
	DMSSimLog& operator << (unsigned long val) noexcept;
	DMSSimLog& operator << (long long val) noexcept;
	DMSSimLog& operator << (unsigned long long val) noexcept;
	DMSSimLog& operator << (float val) noexcept;
	DMSSimLog& operator << (double val) noexcept;
	DMSSimLog& operator << (const char* val) noexcept;
	DMSSimLog& operator << (const wchar_t* val) noexcept;
	DMSSimLog& operator << (const std::string& val) noexcept;
	DMSSimLog& operator << (const std::wstring& val) noexcept;
	DMSSimLog& operator << (const FString& val) noexcept;
	DMSSimLog& operator << (std::ostream& (*val)(std::ostream&)) noexcept;
	DMSSimLog& operator << (std::ios_base& (*val)(std::ios_base&)) noexcept;
	DMSSimLog& operator << (DMSSimFlushLogMarker) noexcept;
protected:
	DMSSimLog();
	virtual ~DMSSimLog();
	DMSSimLog(const DMSSimLog&) = delete;
	DMSSimLog& operator=(const DMSSimLog&) = delete;
};
