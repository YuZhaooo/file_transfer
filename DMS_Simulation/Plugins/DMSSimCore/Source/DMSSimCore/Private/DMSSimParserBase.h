#pragma once

#include <stack>
#include <vector>
#include <Math/Rotator.h>
#include <Math/Vector.h>

#include "DMSSimYamlObj.h"

namespace DMSSimParserBaseHelpers {

	FVector ValuesToVector(const std::vector<float>& Values);
	FRotator ValuesToRotator(const std::vector<float>& Values);
	void ThrowExceptionWithLineN_Internal(const char* Text, const yaml_mark_t& StartMark);
	void ThrowExceptionWithLineN(const char* Text, const yaml_event_t* Event);

} // DMSSimParserBaseHelpers namespace

struct DMSSimParserVector {
	std::vector<float> Values_;
	yaml_mark_t        Mark_ = {};
};

/**
 * @class DMSSimParserBase
 * @brief Parent class for YAML parsers
 */
class DMSSimParserBase {
public:
	DMSSimParserBase(){}
	virtual ~DMSSimParserBase(){}

	YamlObj* EventHandler_version(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
protected:
	std::stack<YamlObj*> YamlObjStack_;
	unsigned             MajorVersion_ = 0;
	unsigned             MinorVersion_ = 0;

	static float ParseFloat(const yaml_event_t* Event, const yaml_char_t* StrY, const char* Name);
	static float ParseFloatEx(const yaml_event_t* Event, const yaml_char_t* StrY, const char* Name, float MinValue, float MaxValue);
	static int ParseIntEx(const yaml_event_t* Event, const yaml_char_t* StrY, const char* Name, int MinValue, int MaxValue);

	bool InitializeInternal(const wchar_t* const FilePath);
	virtual const char* FindFunction(const yaml_event_t& Event) const = 0;
	virtual YamlObj* ProcessEvent(const char* FuncName, const yaml_event_t& Event, YamlObj* Obj, bool Enter) = 0;

	template <class TEventHandlerInfo, int Count>
	static TEventHandlerInfo* FindEventHandler(const char* const EventName, TEventHandlerInfo (&Handlers)[Count]) {
		for (const auto& Info : Handlers) {
			if (strcmp(Info.Name, EventName) == 0) { return &Info; }
		}
		return nullptr;
	}

	template <class TEventHandlerInfo, int Count>
	static TEventHandlerInfo* FindEventHandlerEx(const yaml_event_t& Event, TEventHandlerInfo(&Handlers)[Count]) {
		const char* const EventName = reinterpret_cast<const char*>(Event.data.scalar.value);
		const auto* Handler = FindEventHandler(EventName, Handlers);
		if (Handler) { return Handler; }
		DMSSimParserBaseHelpers::ThrowExceptionWithLineN((std::string("Invalid property [") + EventName + "]").c_str(), &Event);
		return nullptr;
	}
};