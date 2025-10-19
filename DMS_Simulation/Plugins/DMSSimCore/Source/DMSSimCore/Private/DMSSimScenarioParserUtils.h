#pragma once

#include <vector>

#include "Containers/UnrealString.h"
#include "DMSSimScenarioParser.h"

/**
 * Helper function to create scenario parser.
 * The configuration (coordinate space) file path is taken from DMSSIM_DEFAULT_CONFIG environment variable,
 * or from Plugins/DMSSimCore/Source/DMSSimCore/Public/config.yml if it's not set.
 */
TSharedPtr<DMSSimScenarioParser> CreateScenarioParser(const FString& FilePath);

/**
 * Helper function to create vector of scenario parsers.
 * The configuration (coordinate space) file path is taken from DMSSIM_DEFAULT_CONFIG environment variable,
 * or from Plugins/DMSSimCore/Source/DMSSimCore/Public/config.yml if it's not set.
 */
std::vector<TSharedPtr<DMSSimScenarioParser>> CreateScenarioParsers(const FString& DirectoryPath);