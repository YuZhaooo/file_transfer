#include "DMSSimParserBase.h"
#include "DMSSimYamlObj.h"
#include <algorithm>
#include <cassert>
#include <fstream>
#include <functional>
#include <regex>
#include <sstream>


using namespace DMSSimParserBaseHelpers;

namespace {

	std::string& LTrim(std::string& s) {
		s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
		return s;
	}

	std::string& RTrim(std::string& s) {
		s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
		return s;
	}

	std::string& Trim(std::string& s) { return LTrim(RTrim(s)); }

	bool CheckEndOfScenario(const std::string& Text, size_t LineIndex) {
		size_t StartPos = 0;
		for (size_t i = 0; i < LineIndex; ++i) {
			StartPos = Text.find('\n', StartPos);
			if (StartPos == std::string::npos) { return false; }
			++StartPos;
		}

		for (;;) {
			size_t NextPos = Text.find('\n', StartPos);
			std::string Line = Text.substr(StartPos, (NextPos != std::string::npos) ? (NextPos - StartPos) : std::string::npos);

			const size_t CommentStart = Line.find('#');
			if (CommentStart != std::string::npos) { Line = Line.substr(0, CommentStart); }

			Trim(Line);

			if (!Line.empty())
				return false;

			if (NextPos == std::string::npos) { break; }
			StartPos = NextPos;
			++StartPos;
		}
		return true;
	}

	std::string ExtractLineSubstring(const std::string& Text, size_t LineIndex, size_t Column, size_t MaxLen) {
		std::string Result;
		size_t StartPos = 0;
		for (size_t i = 0; i < LineIndex; ++i) {
			StartPos = Text.find('\n', StartPos);
			if (StartPos == std::string::npos) { return Result; }
			++StartPos;
		}
		const size_t EndPos = Text.find('\n', StartPos + 1);
		Result = (EndPos != std::string::npos) ? Text.substr(StartPos, EndPos - StartPos) : Text.substr(StartPos);
		const size_t CommentStart = Result.find('#');
		if (CommentStart != std::string::npos) { Result = Result.substr(0, CommentStart); }
		Trim(Result);
		if (Column < Result.length()) {
			while (Column > 0) {
				if (std::isspace(Result[Column])) {
					++Column;
					break;
				}
				--Column;
			}
			Result = Result.substr(Column, MaxLen);
		}
		return Result;
	}

	template <class TNumber>
	TNumber ParseNumber(const yaml_event_t* const Event, const yaml_char_t* const StrY, const char* const Name) {
		const auto Str = reinterpret_cast<const char*>(StrY);
		std::istringstream NumberStream(Str);
		TNumber Value = 0;
		NumberStream >> std::noskipws >> Value;
		if (!NumberStream.eof() || NumberStream.fail()) {
			std::string Message("Invalid number format of ");
			Message += Name;
			Message += ": ";
			Message += "\"";
			Message += Str;
			Message += "\"";
			ThrowExceptionWithLineN(Message.c_str(), Event);
		}
		return Value;
	}

	template <class TNumber>
	TNumber ParseNumberEx(const yaml_event_t* const Event, const yaml_char_t* const StrY, const char* const Name, const TNumber MinValue, const TNumber MaxValue) {
		const TNumber Value = ParseNumber<TNumber>(Event, StrY, Name);
		if (Value < MinValue || Value > MaxValue) {
			std::string Message("Invalid value of ");
			Message += Name;
			Message += " \"";
			Message += std::to_string(Value);
			Message += "\".";
			Message += " Must be in range [ ";
			Message += std::to_string(MinValue);
			Message += ", ";
			Message += std::to_string(MaxValue);
			Message += " ].";
			ThrowExceptionWithLineN(Message.c_str(), Event);
		}
		return Value;
	}

	std::string LoadFile(const wchar_t* FileName) {
		std::ifstream File(FileName);
		std::string Content((std::istreambuf_iterator<char>(File)), std::istreambuf_iterator<char>());
		return Content;
	}

	void PopObjStack(std::stack<YamlObj*>& Stack) {
		if (Stack.empty()) { return; }
		Stack.pop();
	}
} // anonymous namespace

bool DMSSimParserBase::InitializeInternal(const wchar_t* const FilePath) {
	assert(YamlObjStack_.size() == 0);
	const std::string ScenarioStr = LoadFile(FilePath);
	if (ScenarioStr.empty()) { throw std::exception("failed to load scenario"); }

	const size_t NumberOfLines = std::count(ScenarioStr.begin(), ScenarioStr.end(), '\n') + 1;
	yaml_parser_t Parser = {};
	yaml_parser_initialize(&Parser);
	yaml_parser_set_input_string(&Parser, (unsigned char*)ScenarioStr.c_str(), ScenarioStr.length());

	const char* CurrentFunc = nullptr;
	yaml_event_type_t PrevEventType = YAML_NO_EVENT;

	yaml_mark_t PrevEndMark = {};
	bool InputOk = true;
	while (InputOk) {
		yaml_event_t Event = {};
		const int Status = yaml_parser_parse(&Parser, &Event);
		std::shared_ptr<yaml_event_t> EventDeleter(&Event, &yaml_event_delete);

		if (!Status || Event.type == YAML_NO_EVENT) {
			const size_t PrevLine = PrevEndMark.line;
			if ((PrevLine + 1) < NumberOfLines && !CheckEndOfScenario(ScenarioStr, PrevLine + 1)) {
				std::stringstream ErrorMessage;
				ErrorMessage << "Line " << (PrevLine + 1) << ": ";
				ErrorMessage << "Yaml syntax error after ";
				const auto LineExample = ExtractLineSubstring(ScenarioStr, PrevLine, PrevEndMark.column, 12);
				if (!LineExample.empty()) { ErrorMessage << LineExample << "..."; }
				throw std::exception(ErrorMessage.str().c_str());
			}
			break;
		}

		PrevEndMark = Event.end_mark;

		switch (Event.type) {
		case YAML_STREAM_START_EVENT:
		case YAML_STREAM_END_EVENT:
		case YAML_DOCUMENT_START_EVENT:
		case YAML_DOCUMENT_END_EVENT:
		case YAML_ALIAS_EVENT:
			break;
		case YAML_SEQUENCE_START_EVENT:
			if (!YamlObjStack_.empty()) {
				YamlObj* const ParentObj = YamlObjStack_.top();
				if (ParentObj) {
					ParentObj->Sequence_ = true;
					if (ParentObj->IsComplexSequence()) { YamlObjStack_.push(ParentObj); }
				}
			}
			break;
		case YAML_SEQUENCE_END_EVENT:
			CurrentFunc = nullptr;
			if (!YamlObjStack_.empty()) {
				YamlObj* const ParentObj = YamlObjStack_.top();
				if (ParentObj) { ParentObj->Sequence_ = false; }
				PopObjStack(YamlObjStack_);
			}
			break;
		case YAML_SCALAR_EVENT: {
			YamlObj* const ParentObj = YamlObjStack_.empty() ? nullptr : YamlObjStack_.top();
			if (CurrentFunc) {
				ProcessEvent(CurrentFunc, Event, ParentObj, false);
				if (!ParentObj || !ParentObj->Sequence_) {
					CurrentFunc = nullptr;
					PopObjStack(YamlObjStack_);
				}
			}
			else {
				CurrentFunc = FindFunction(Event);
				if (!CurrentFunc) {
					InputOk = false;
					break;
				}
				YamlObj* const Obj = ProcessEvent(CurrentFunc, Event, ParentObj, true);
				YamlObjStack_.push(Obj);
			}
		}
		break;
		case YAML_MAPPING_START_EVENT:
			CurrentFunc = nullptr;
			if (!YamlObjStack_.empty()) {
				const auto NewObj = YamlObjStack_.top()->StartMapping(Event);
				if (NewObj) { YamlObjStack_.push(NewObj); }
			}
			break;
		case YAML_MAPPING_END_EVENT:
			if (!YamlObjStack_.empty()) { PopObjStack(YamlObjStack_); }
			break;
		}
		PrevEventType = Event.type;
	}
	return InputOk;
}

float DMSSimParserBase::ParseFloat(const yaml_event_t* const Event, const yaml_char_t* const StrY, const char* const Name) { return ParseNumber<float>(Event, StrY, Name); }

float DMSSimParserBase::ParseFloatEx(const yaml_event_t* const Event, const yaml_char_t* const StrY, const char* const Name, const float MinValue, const float MaxValue) { return ParseNumberEx<float>(Event, StrY, Name, MinValue, MaxValue); }

int DMSSimParserBase::ParseIntEx(const yaml_event_t* const Event, const yaml_char_t* const StrY, const char* const Name, const int MinValue, const int MaxValue) { return ParseNumberEx<int>(Event, StrY, Name, MinValue, MaxValue); }

YamlObj* DMSSimParserBase::EventHandler_version(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
	if (Obj) { ThrowExceptionWithLineN("Version block must be on the base level", Event); }
	if (!Enter) {
		std::string VersionStr(reinterpret_cast<const char*>(Event->data.scalar.value));
		std::regex VersionRegex("(\\d{1})\\.(\\d{1,2})");
		if (!std::regex_match(VersionStr, VersionRegex)) { ThrowExceptionWithLineN("Invalid version format", Event); }
		const auto Pos = VersionStr.find('.');
		if (Pos == std::string::npos) { ThrowExceptionWithLineN("Version parser - internal error", Event); }
		MajorVersion_ = atoi(VersionStr.c_str());
		MinorVersion_ = atoi(VersionStr.c_str() + Pos + 1);
	}
	return nullptr;
}

namespace DMSSimParserBaseHelpers {

FVector ValuesToVector(const std::vector<float>& Values) {
	assert(Values.size() == 3);
	return FVector{ Values.at(0), Values.at(1), Values.at(2) };
}

FRotator ValuesToRotator(const std::vector<float>& Values) {
	assert(Values.size() == 3);
	return FRotator{ Values.at(1), Values.at(2), Values.at(0) };
}

void ThrowExceptionWithLineN_Internal(const char* const Text, const yaml_mark_t& StartMark) {
	std::string TextStr("Line ");
	TextStr += std::to_string(StartMark.line + 1);
	TextStr += ": ";
	TextStr += Text;
	throw std::exception(TextStr.c_str());
}

void ThrowExceptionWithLineN(const char* const Text, const yaml_event_t* const Event) {
	if (Event) { ThrowExceptionWithLineN_Internal(Text, Event->start_mark); }
	throw std::exception(Text);
}

} // DMSSimParserBaseHelpers namespace