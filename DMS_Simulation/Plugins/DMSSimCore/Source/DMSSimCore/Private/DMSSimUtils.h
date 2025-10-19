#pragma once

#include <string>

std::wstring FStringToWide(const FString& Str);
std::string WideToNarrow(const wchar_t* StrW);