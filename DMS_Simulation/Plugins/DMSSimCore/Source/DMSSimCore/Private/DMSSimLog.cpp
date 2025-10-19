#include "DMSSimLog.h"
#include <algorithm>
#include <chrono>
#include <ctime>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include "DMSSimConfig.h"
#include "DMSSimUtils.h"

DECLARE_LOG_CATEGORY_EXTERN(DMSSIM, Log, All);
DEFINE_LOG_CATEGORY(DMSSIM);

namespace{

bool WithDirectory = false;
bool ConsoleOutput = false;
bool EditorConsoleOutput = !!WITH_EDITOR;
bool ScreenOutput = false;

std::wstring CurrentLogPath;
std::ofstream LogFile;

enum LogType {
	LogTypeDebug,
	LogTypeInfo,
	LogTypeWarn,
	LogTypeError,
	LogTypeFatal,
};

class DMSSimLogImpl: public DMSSimLog{
public:
	DMSSimLogImpl(const LogType Type, const FColor Color, const ELogVerbosity::Type Verbosity): Type_(Type), Color_(Color), Verbosity_(Verbosity) { }

	LogType GetType() const { return Type_; }
	const char* GetTypeStr() const;
	const FColor& GetColor() const { return Color_; }
	ELogVerbosity::Type GetVerbosity() const { return Verbosity_; }
	std::ostream& GetStream() { return Stream_; }
	std::string FlushMessage();
private:
	LogType             Type_;
	FColor              Color_;
	ELogVerbosity::Type Verbosity_;
	std::stringstream   Stream_;
};

const char* DMSSimLogImpl::GetTypeStr() const {
	switch (Type_) {
	case LogTypeDebug:
		return "[DEBUG]";
	case LogTypeInfo:
		return "[INFO]";
	case LogTypeWarn:
		return "[WARNING]";
	case LogTypeError:
		return "[ERROR]";
	case LogTypeFatal:
		return "[FATAL]";
	}
	return "[NONE]";
}

std::string DMSSimLogImpl::FlushMessage() {
	std::string Message = Stream_.str();
	Stream_.str("");
	Stream_.clear();
	return Message;
}

template <class T>
DMSSimLog& LogWrite(DMSSimLog& Log, const T& value) {
	auto& LogImpl = static_cast<DMSSimLogImpl&>(Log);
	LogImpl.GetStream() << value;
	return Log;
}

DMSSimLog& LogWriteW(DMSSimLog& Log, const wchar_t* const value){
	if (value == nullptr) { return LogWrite(Log, (const char*)nullptr); }
	const auto Res = WideToNarrow(value);
	return LogWrite(Log, Res);
}

void InitLogFile() {
	if (LogFile.is_open() && WithDirectory == DMSSimConfig::IsOutputDirectoryPresent()) { return; }
	LogFile.close();
	WithDirectory = DMSSimConfig::IsOutputDirectoryPresent();
	std::wstring FileName = DMSSimConfig::GetFilePrefix() + L".log";
	if (!CurrentLogPath.empty() && CurrentLogPath != FileName) {
		auto& FileManager = FPlatformFileManager::Get().GetPlatformFile();
		const auto PlatformFile = FileManager.GetLowerLevel();
		PlatformFile->MoveFile(FileName.c_str(), CurrentLogPath.c_str());
	}
	CurrentLogPath = FileName;
	LogFile.open(FileName, std::ios::app);
}

void WriteLogMessage(std::ostream& Stream, const char* Type, const char* Time, const std::string& Message) { Stream << std::setw(10) << std::left << Type << Time << ": " << Message << std::endl; }

DMSSimLog& FlushMessage(DMSSimLog& Log) {
	auto& LogImpl = static_cast<DMSSimLogImpl&>(Log);
	std::mutex Mutex;
	std::lock_guard<std::mutex> MutexGuard(Mutex);
	InitLogFile();
	auto Message = LogImpl.FlushMessage();
	if (!Message.empty()) {
		const auto TimeNow = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		char TimeBuffer[64] = {};
		const size_t TimeBufferSize = sizeof(TimeBuffer);
		ctime_s(TimeBuffer, TimeBufferSize, &TimeNow);
		std::replace(TimeBuffer, TimeBuffer + TimeBufferSize, '\n', '\0');
		WriteLogMessage(LogFile, LogImpl.GetTypeStr(), TimeBuffer, Message);

		if (ConsoleOutput) { WriteLogMessage(std::cout, LogImpl.GetTypeStr(), TimeBuffer, Message); }

		if (EditorConsoleOutput) {
			FString MessageStr(Message.c_str());
			switch (LogImpl.GetType()) {
			case LogTypeDebug:
				UE_LOG(DMSSIM, Verbose, TEXT("%s"), *MessageStr);
				break;
			case LogTypeInfo:
				UE_LOG(DMSSIM, Display, TEXT("%s"), *MessageStr);
				break;
			case LogTypeWarn:
				UE_LOG(DMSSIM, Warning, TEXT("%s"), *MessageStr);
				break;
			case LogTypeError:
				UE_LOG(DMSSIM, Error, TEXT("%s"), *MessageStr);
				break;
			case LogTypeFatal:
				UE_LOG(DMSSIM, Error, TEXT("%s"), *MessageStr);
				break;
			}
		}

		if (ScreenOutput && GEngine) { GEngine->AddOnScreenDebugMessage(INDEX_NONE, 4, LogImpl.GetColor(), Message.c_str(), true); }
	}
	return Log;
}

thread_local DMSSimLogImpl LogDebug(LogTypeDebug, FColor::Blue, ELogVerbosity::Verbose);
thread_local DMSSimLogImpl LogInfo(LogTypeInfo, FColor::Green, ELogVerbosity::Display);
thread_local DMSSimLogImpl LogWarn(LogTypeWarn, FColor::Yellow, ELogVerbosity::Warning);
thread_local DMSSimLogImpl LogError(LogTypeError, FColor::Red, ELogVerbosity::Error);
thread_local DMSSimLogImpl LogFatal(LogTypeFatal, FColor::Red, ELogVerbosity::Fatal);
} // anonymous namespace

DMSSimLog::DMSSimLog() {}

DMSSimLog::~DMSSimLog() {}

DMSSimLog& DMSSimLog::Debug() { return LogDebug; }

DMSSimLog& DMSSimLog::Info() { return LogInfo; }

DMSSimLog& DMSSimLog::Warn() { return LogWarn; }

DMSSimLog& DMSSimLog::Error() { return LogError; }

DMSSimLog& DMSSimLog::Fatal() { return LogFatal; }

DMSSimLog& DMSSimLog::operator << (bool val) noexcept {	return LogWrite(*this, val); }

DMSSimLog& DMSSimLog::operator << (short val) noexcept { return LogWrite(*this, val); }

DMSSimLog& DMSSimLog::operator << (unsigned short val) noexcept { return LogWrite(*this, val); }

DMSSimLog& DMSSimLog::operator << (int val) noexcept { return LogWrite(*this, val); }

DMSSimLog& DMSSimLog::operator << (unsigned int val) noexcept { return LogWrite(*this, val); }

DMSSimLog& DMSSimLog::operator << (long val) noexcept {	return LogWrite(*this, val); }

DMSSimLog& DMSSimLog::operator << (unsigned long val) noexcept { return LogWrite(*this, val); }

DMSSimLog& DMSSimLog::operator << (long long val) noexcept { return LogWrite(*this, val); }

DMSSimLog& DMSSimLog::operator << (unsigned long long val) noexcept { return LogWrite(*this, val); }

DMSSimLog& DMSSimLog::operator << (float val) noexcept { return LogWrite(*this, val); }

DMSSimLog& DMSSimLog::operator << (double val) noexcept { return LogWrite(*this, val); }

DMSSimLog& DMSSimLog::operator << (const char* val) noexcept { return LogWrite(*this, val); }

DMSSimLog& DMSSimLog::operator << (const wchar_t* val) noexcept { return LogWriteW(*this, val); }

DMSSimLog& DMSSimLog::operator << (const std::string& val) noexcept { return LogWrite(*this, val); }

DMSSimLog& DMSSimLog::operator << (const std::wstring& val) noexcept { return LogWriteW(*this, val.c_str()); }

DMSSimLog& DMSSimLog::operator << (const FString& val) noexcept { return *this << *val; }

DMSSimLog& DMSSimLog::operator << (std::ostream& (*val)(std::ostream&)) noexcept { return LogWrite(*this, val); }

DMSSimLog& DMSSimLog::operator << (std::ios_base& (*val)(std::ios_base&)) noexcept { return LogWrite(*this, val); }

DMSSimLog& DMSSimLog::operator << (DMSSimFlushLogMarker) noexcept { return FlushMessage(*this); }

void DMSSimLog::EnableConsoleOutput(bool Enable) { ConsoleOutput = Enable; }

void DMSSimLog::EnableScreenOutput(bool Enable) { ScreenOutput = Enable; }