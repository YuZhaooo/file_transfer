#include "DMSSimUtils.h"
#include <cstdlib>
#include <locale>
#include <vector>

namespace {
	template <class TChar>
	std::wstring FStringToWideInternal(const TChar* const Str) { return Str; }

	template <>
	std::wstring FStringToWideInternal<char>(const char* const Str) {
		std::vector<wchar_t> Buffer;
		const size_t Len = strlen(Str);
		Buffer.resize(Len + 1);
#pragma warning(push)
#pragma warning(disable : 4996)
		mbstowcs(&Buffer[0], Str, Len);
#pragma warning(pop)
		return &Buffer[0];
	}
} // anonymous namespace

std::wstring FStringToWide(const FString& Str){ return FStringToWideInternal(*Str); }

std::string WideToNarrow(const wchar_t* StrW) {
	if (StrW == nullptr) { return std::string(); }

	std::vector<char> Buffer;
	const size_t Len = wcslen(StrW);
	Buffer.resize(Len + 1);
#pragma warning(push)
#pragma warning(disable : 4996)
	wcstombs(&Buffer[0], StrW, Len);
#pragma warning(pop)
	return &Buffer[0];
}