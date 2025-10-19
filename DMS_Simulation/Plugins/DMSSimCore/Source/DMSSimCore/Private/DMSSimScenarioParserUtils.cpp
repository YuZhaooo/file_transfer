#include "DMSSimScenarioParserUtils.h"
#include "DMSSimLog.h"
#include "GenericPlatform/GenericPlatformMisc.h"
#include "Misc/Paths.h"
#include "Templates/SharedPointer.h"
#include "HAL/PlatformFilemanager.h"

namespace {
	constexpr char DMSSIM_DEFAULT_CONFIG_PATH[] = "DMSSIM_DEFAULT_CONFIG";
	constexpr char DMSSIM_DEFAULT_CONFIG_NAME[] = "config.yml";
	constexpr char DMSSIM_DEFAULT_CONFIG_FOLDER[] = "Plugins/DMSSimCore/Source/DMSSimCore/Public/";
} // anonymous namespace

static TSharedPtr<DMSSimConfigParser> GetConfig(const FString& DirectoryPath) {
	FString ConfigPath = "";

	ConfigPath = FPaths::ConvertRelativePathToFull(DirectoryPath, DMSSIM_DEFAULT_CONFIG_NAME);
	if (!FPaths::FileExists(ConfigPath)) {
		DMSSimLog::Info() << DMSSIM_DEFAULT_CONFIG_PATH << " not set. " << DMSSIM_DEFAULT_CONFIG_PATH << " environment variable can be used to override the default config." << FL;
		ConfigPath.Empty();
	}

	if (ConfigPath.IsEmpty()) {
		ConfigPath = FGenericPlatformMisc::GetEnvironmentVariable(*FString(DMSSIM_DEFAULT_CONFIG_PATH));
		if (ConfigPath.IsEmpty() || !FPaths::FileExists(ConfigPath)) {
			DMSSimLog::Info() << "No config in the scenario folder. Config can be overriden by placing " << DMSSIM_DEFAULT_CONFIG_NAME << " in the scenario folder." << FL;
			ConfigPath.Empty();
		}
	}

	if (ConfigPath.IsEmpty()) {
		//const auto ContentDirectory = FPaths::ProjectContentDir();
		ConfigPath = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
		ConfigPath.Append(DMSSIM_DEFAULT_CONFIG_FOLDER);
		ConfigPath.Append(DMSSIM_DEFAULT_CONFIG_NAME);
		DMSSimLog::Info() << "Trying to load config at " << ConfigPath << FL;
		IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
		if (!FileManager.FileExists(*ConfigPath)) {
			DMSSimLog::Info() << "Config not found at " << ConfigPath  << "!" << FL;
			ConfigPath.Empty();
		}
	}

	if (ConfigPath.IsEmpty()) { throw std::runtime_error("Could not find the configuration file"); }

	return TSharedPtr<DMSSimConfigParser>(DMSSimConfigParser::Create(*ConfigPath));
}

TSharedPtr<DMSSimScenarioParser> CreateScenarioParser(const FString& FilePath) {
	const auto DirectoryPath = FPaths::GetPath(FilePath);
	const auto Config = GetConfig(DirectoryPath);
	return MakeShareable(DMSSimScenarioParser::Create(*FilePath, *Config));
}

std::vector<TSharedPtr<DMSSimScenarioParser>> CreateScenarioParsers(const FString& DirectoryPath) {
	const auto Config = GetConfig(DirectoryPath);
	std::vector<TSharedPtr<DMSSimScenarioParser>> Result;
	const auto FileVisitor = [&Result, &Config](const TCHAR* FilePath, bool bIsDirectory) -> bool {
		if (!bIsDirectory) {
			if (FPaths::GetCleanFilename(FilePath) == FString(DMSSIM_DEFAULT_CONFIG_NAME)) {
				DMSSimLog::Info() << "File with default config name detected in input folder, it's not loaded as scenario" << FL;
			} else {
				auto Parser = DMSSimScenarioParser::Create(FilePath, *Config);
				if(Parser != nullptr) {
					DMSSimLog::Info() << "Loaded scenario: " << FilePath << FL;
					Result.push_back(MakeShareable(Parser));
				}
			}
		}
		return true; // Continue iteration
	};

	IPlatformFile& FileManager = FPlatformFileManager::Get().GetPlatformFile();
	// Iterate over the directory
	FileManager.IterateDirectory(*DirectoryPath, FileVisitor);
	return Result;
}
