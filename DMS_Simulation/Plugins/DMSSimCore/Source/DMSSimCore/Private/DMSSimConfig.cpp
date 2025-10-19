#include "DMSSimConfig.h"
#include <cassert>
#include <ctime>
#include <mutex>
#include <string>
#include <vector>
#include "DMSSimLog.h"
#include "DMSSimScenarioParserUtils.h"
#include "DMSSimUtils.h"
#include "Misc/CommandLine.h"

namespace DMSSimConfig {
namespace {
bool Recording = false;
std::vector<TSharedPtr<DMSSimScenarioParser>> ScenarioParsers_;
TSharedPtr<DMSSimScenarioParser> CurrentScenarioParser_;

std::string ScenarioParserErrorMessage;
DMSSimGroundTruthFrame GroundTruthFrame = {};
const wchar_t* OutputDirectoryPtr = nullptr;
std::wstring OutputDirectory;
float FrameTime = 0.0f;
int   FrameIndex = 0;
int NumRemainingFrames_ = 0;
CameraSetCallback CameraCallback;

std::wstring MakeDateTimeStr()
{
	const time_t Now = std::time(0);
	wchar_t Buffer[80];
#pragma warning(push)
#pragma warning(disable : 4996)
	struct tm TimeInfo = *std::localtime(&Now);
#pragma warning(pop)
	wcsftime(Buffer, sizeof(Buffer) - 1, L"%Y_%m_%d_%H_%M_%S", &TimeInfo);
	return Buffer;
}
} // anonymous namespace

const TSharedPtr<DMSSimScenarioParser> GetScenarioParser(const int32 index) {
	DMSSimLog::Info() << "Get Scenario Parser " << index << " from " << ScenarioParsers_.size() << FL;
	if (index >= ScenarioParsers_.size()) { return nullptr; }
	return ScenarioParsers_[index];
}

void SetCurrentScenarioParser(const TSharedPtr<DMSSimScenarioParser> Parser) { CurrentScenarioParser_ = Parser; }

void SetScenarioParsers(const std::vector<TSharedPtr<DMSSimScenarioParser>> Parsers) { ScenarioParsers_ = Parsers; }

void ResetScenarioParsers() { ScenarioParsers_.clear(); }

void AddScenarioParser(const TSharedPtr<DMSSimScenarioParser> Parser) { ScenarioParsers_.emplace_back(Parser); }

TSharedPtr<DMSSimScenarioParser> GetCurrentScenarioParser() { return CurrentScenarioParser_; }

int32 GetScenarioParsersCount() { return ScenarioParsers_.size(); }

std::string GetScenarioParserErrorMessage() { return ScenarioParserErrorMessage; }

std::wstring GetFilePrefix() {
	static std::wstring TimeStr;
	std::mutex Mutex;
	std::lock_guard<std::mutex> MutexGuard(Mutex);
	std::wstring FilePrefix;
	if (TimeStr.empty()) { TimeStr = L"dms_" + MakeDateTimeStr(); }
	if (OutputDirectoryPtr) {
		FilePrefix = OutputDirectoryPtr;
		FilePrefix += L"/";
	}
	FilePrefix += TimeStr;
	return FilePrefix;
}

const DMSSimCoordinateSpace& GetCoordinateSpace() {
	if (CurrentScenarioParser_) { return CurrentScenarioParser_.Get()->GetCoordinateSpace(); }
	return DMSSimScenarioParser::GetDefaultCoordinateSpace();
}

const DMSSimCamera& GetCamera() {
	if (CurrentScenarioParser_) { return CurrentScenarioParser_.Get()->GetCamera(); }
	return DMSSimScenarioParser::GetDefaultCamera();
}

bool IsRecording() { return Recording; }

void SetNumRemainingFrames(int NumFrames) { NumRemainingFrames_ = NumFrames; }

int GetNumRemainingFrames() { return NumRemainingFrames_; }

bool StartRecording() {
	if (!Recording) {
		Recording = true;
		return true;
	}
	return false;
}

bool StopRecording() {
	if (Recording) {
		Recording = false;
		return true;
	}
	return false;
}

void UpdateDisplayedFrameTime(const float Time) {
	FrameTime = Time;
	++FrameIndex;
}

int GetCurrentFrame() { return FrameIndex; }

float GetCurrentTime() { return FrameTime; }

void SetCameraComponent(UCameraComponent* const Camera) { if (CameraCallback) { CameraCallback(Camera); } }

void SetCameraCallback(const CameraSetCallback& Callback) { CameraCallback = Callback; }

void ResetGroundTruthData() { memset(&GroundTruthFrame, 0, sizeof(GroundTruthFrame)); }

const DMSSimGroundTruthFrame& GetGroundTruthFrame() { return GroundTruthFrame; }

void SetGroundTruthCommonData(const DMSSimGroundTruthCommon& CommonData) { GroundTruthFrame.Common = CommonData; }

void SetGroundTruthOccupantData(FDMSSimOccupantType OccupantType, const DMSSimGroundTruthOccupant& OccupantData) {
	assert(OccupantType < FDMSSimOccupantType::PassengerCount);
	GroundTruthFrame.Occupants[static_cast<uint8>(OccupantType)] = OccupantData;
}

bool IsOutputDirectoryPresent() { return !!OutputDirectoryPtr; }

void Initialize() {
	//if (WITH_EDITOR) return;
	check(CurrentScenarioParser_.Get() == nullptr);

	const TCHAR* const CommandLine = FCommandLine::Get();
	if (!CommandLine) {
		DMSSimLog::Info() << "No command line arguments" << FL;
		return;
	}

	DMSSimLog::Info() << "Run with Command Line: " << CommandLine << FL;

	TArray<FString> Tokens;
	TArray<FString> Switches;
	FCommandLine::Parse(CommandLine, Tokens, Switches);

	// Log all tokens
	for (int32 i = 0; i < Tokens.Num(); ++i) { DMSSimLog::Info() << "Token " << i << ": " << Tokens[i] << FL; }

	if (Tokens.Num() < 2) {
		DMSSimLog::Error() << "Invalid number of command line arguments" << CommandLine << FL;
		return;
	}

	std::wstring ScenarioPath;
	std::wstring OutDir;
	for (size_t i = 0; i < Tokens.Num(); ++i) {
		const auto ArgSwitch = Tokens[i].ToLower();
		const auto ArgValue = ((i + 1) < Tokens.Num())? Tokens[i + 1] : FString();
		if (ArgSwitch == TEXT("d3d11") 
			|| ArgSwitch == TEXT("d3d12") 
			|| ArgSwitch == TEXT("dx11")
			|| ArgSwitch == TEXT("dx12")
			|| ArgSwitch == TEXT("vulkan")
			|| ArgSwitch == TEXT("sm5")
			|| ArgSwitch == TEXT("opengl")
			|| ArgSwitch == TEXT("console")
			|| ArgSwitch == TEXT("cookonthefly")
			|| ArgSwitch == TEXT("filehostip")
			|| ArgSwitch == TEXT("stdout")
			|| ArgSwitch == TEXT("fullstdoutlogoutput")
			|| ArgSwitch == TEXT("renderoffscreen")
			|| ArgSwitch == TEXT("unattended")
			|| ArgSwitch == TEXT("windowed")
			|| ArgSwitch == TEXT("resx")
			|| ArgSwitch == TEXT("resy"))
		{
			// Pass through important standard attributes
			// todo: make parsing more precise and graceful to unknown args
			continue;
		}

		switch(ArgSwitch[0]) {
		case TEXT('c'):
			ScenarioPath = FStringToWide(ArgValue);
			DMSSimLog::Info() << "Scenario path: " << ScenarioPath << FL;
			++i;
			break;
		case TEXT('d'):
			OutDir = FStringToWide(ArgValue);
			DMSSimLog::Info() << "Output directory: " << OutDir << FL;
			++i;
			break;
		case TEXT('p'): // skip profile (handled in the profile selection module)
			++i;
			break;
		case TEXT('l'):
			{
				for (const auto C: ArgSwitch) {
					switch (C) {
					case TEXT('c'):
						DMSSimLog::EnableConsoleOutput(true);
						break;
					case TEXT('s'):
						DMSSimLog::EnableScreenOutput(true);
						break;
					}
				}
			}
			break;
		default:
			DMSSimLog::Warn() << "Invalid argument switch: " << ArgSwitch << FL;
		}
	}

	auto& FileManager = FPlatformFileManager::Get().GetPlatformFile();

	if (!OutDir.empty()) {
		const auto PlatformFile = FileManager.GetLowerLevel();

		if (!PlatformFile) { DMSSimLog::Error() << "FileManager.GetLowerLevel() returned nullptr. Can't create an output directory." << FL; }
		else {
			if (!PlatformFile->DirectoryExists(OutDir.c_str())) { PlatformFile->CreateDirectory(OutDir.c_str()); }
			if (!PlatformFile->DirectoryExists(OutDir.c_str())) { DMSSimLog::Error() << "Failed to create output directory: " << OutDir << FL; }
			else {
				OutputDirectory = OutDir;
				OutputDirectoryPtr = OutputDirectory.c_str();
			}
		}
		
	}
	DMSSimLog::Info() << "Check for Scenario Path " << FL;

	try {
		if (FileManager.FileExists(ScenarioPath.c_str())) {
			DMSSimLog::Info() << "Loading scenario: " << ScenarioPath << FL;
			ScenarioParsers_.emplace_back(CreateScenarioParser(ScenarioPath.c_str()));
		} else if (FileManager.DirectoryExists(ScenarioPath.c_str())) {
			DMSSimLog::Info() << "Loading all scenarios from directory: " << ScenarioPath << FL;
			ScenarioParsers_ = CreateScenarioParsers(ScenarioPath.c_str());
		} else {
			DMSSimLog::Warn() << "No scenario path specified" << FL;
			return;
		}
	}
	catch (const std::exception& e) {
		ScenarioParserErrorMessage = e.what();
		DMSSimLog::Fatal() << "Scenario Parser: " << e.what() << FL;
	}
}

} // namespace DMSSimConfig
