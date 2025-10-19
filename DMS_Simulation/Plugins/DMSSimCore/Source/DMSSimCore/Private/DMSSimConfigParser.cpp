#include "DMSSimConfigParser.h"
#include "DMSSimParserBase.h"
#include "DMSSimYamlObj.h"
#include <cassert>
#include <memory>
#include <stack>
#include <string>
#include <vector>
#include <yaml.h>

using namespace DMSSimParserBaseHelpers;

namespace {
	bool IsCarName(const char* const Name) {
		if (!Name) { return false; }
		const size_t Len = strlen(Name);
		if (Len == 0) { return false; }
		for (size_t i = 0; i < Len; ++i) {
			const auto C = Name[i];
			if (!isalpha(C) && !isdigit(C) && C != '_') { return false; }
		}
		return true;
	}

	class DMSSimCoordinateSpaceImpl : public YamlObj, public DMSSimCoordinateSpace {
	public:
		DMSSimCoordinateSpaceImpl() : YamlObj(YamlObjTypeCoordinateSpace) {}
		virtual ~DMSSimCoordinateSpaceImpl() {}
		virtual const char* GetCarModel() const override { return CarModel_.c_str(); };
		virtual FRotator GetRotation() const override { return ValuesToRotator(Rotation_.Values_); };
		virtual FVector  GetTranslation() const override { return ValuesToVector(Translation_.Values_); };
		virtual FVector  GetScale() const override;
		virtual void Validate() const override;

		std::string         CarModel_;
		DMSSimParserVector  Rotation_;
		DMSSimParserVector  Translation_;
		DMSSimParserVector  Scale_;
	};

	FVector DMSSimCoordinateSpaceImpl::GetScale() const {
		auto Scale = ValuesToVector(Scale_.Values_);
		Scale.Y = -Scale.Y;
		return Scale;
	}

	void DMSSimCoordinateSpaceImpl::Validate() const {
		if (Rotation_.Values_.size() != 3) { ThrowExceptionWithLineN_Internal("Invalid Rotation", Rotation_.Mark_); }
		if (Translation_.Values_.size() != 3) { ThrowExceptionWithLineN_Internal("Invalid Translation", Translation_.Mark_); }
		if (Scale_.Values_.size() != 3) { ThrowExceptionWithLineN_Internal("Invalid Scale", Scale_.Mark_); }
	}

	class DMSSimTransformList : public YamlObj {
	public:
		DMSSimTransformList(): YamlObj(YamlObjTypeTransformList) {}
		std::vector<DMSSimCoordinateSpaceImpl> CoordinateSpaces_;
	};

	class DMSSimConfigParserImpl : public DMSSimConfigParser, public DMSSimParserBase {
	public:
		DMSSimConfigParserImpl() {}
		virtual ~DMSSimConfigParserImpl() {}

		bool Initialize(const wchar_t* const FilePath);

		virtual unsigned GetVersionMajor() const override { return MajorVersion_; };
		virtual unsigned GetVersionMinor() const override { return MinorVersion_; };
		virtual const char* GetProject() const override { return Project_.c_str(); };
		virtual size_t GetCoordinateSpaceCount() const override { return TransformList_.CoordinateSpaces_.size(); };
		virtual const DMSSimCoordinateSpace& GetCoordinateSpace(size_t SpaceIndex) const override { return TransformList_.CoordinateSpaces_.at(SpaceIndex); }

		YamlObj* EventHandler_project(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_transform_base_to_project(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_car_coordinate_space(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_rotation(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_translation(const yaml_event_t* Event, YamlObj* Obj, bool Enter);
		YamlObj* EventHandler_scale(const yaml_event_t* Event, YamlObj* Obj, bool Enter);

	protected:
		virtual const char* FindFunction(const yaml_event_t& Event) const override;
		virtual YamlObj* ProcessEvent(const char* FuncName, const yaml_event_t& Event, YamlObj* Obj, bool Enter) override;

	private:
		std::string             Project_;
		DMSSimTransformList     TransformList_;

		void ValidateParameters();
	};

	namespace {
		typedef YamlObj* (DMSSimConfigParserImpl::* ProcessConfigYamlEventFunc)(const yaml_event_t* Event, YamlObj* Obj, bool Enter);

#define DMSSIM_DEFINE_YAML_EVENT_HANDLER(name) { #name, &DMSSimConfigParserImpl::EventHandler_##name },
		struct EventHandlerInfo{
			const char* const          Name;
			ProcessConfigYamlEventFunc Func;
		}
		const ConfigEventHandlers[] = {
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(version)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(project)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(transform_base_to_project)

			DMSSIM_DEFINE_YAML_EVENT_HANDLER(rotation)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(translation)
			DMSSIM_DEFINE_YAML_EVENT_HANDLER(scale)
		};
#undef DMSSIM_DEFINE_YAML_EVENT_HANDLER
	} // anonymous namespace

	bool DMSSimConfigParserImpl::Initialize(const wchar_t* const FilePath) {
		const bool Result = InitializeInternal(FilePath);
		if (Result) { ValidateParameters(); }
		return Result;
	}

	const char* DMSSimConfigParserImpl::FindFunction(const yaml_event_t& Event) const {
		if (!YamlObjStack_.empty() && YamlObjStack_.top()->GetYamlType() == YamlObjTypeTransformList) {
			const auto Name = reinterpret_cast<const char*>(Event.data.scalar.value);
			if (IsCarName(Name)) { return Name; } 
		}
		const auto* const Info = FindEventHandlerEx(Event, ConfigEventHandlers);
		if (Info) { return Info->Name; }
		return nullptr;
	}

	YamlObj* DMSSimConfigParserImpl::ProcessEvent(const char* FuncName, const yaml_event_t& Event, YamlObj* Obj, bool Enter) {
		if (!YamlObjStack_.empty() && YamlObjStack_.top() && YamlObjStack_.top()->GetYamlType() == YamlObjTypeTransformList) {
			if (IsCarName(FuncName)) { return EventHandler_car_coordinate_space(&Event, Obj, Enter); }
		}
		const auto* const Info = FindEventHandler(FuncName, ConfigEventHandlers);
		if (Info) { return (this->*Info->Func)(&Event, Obj, Enter); }
		return nullptr;
	}

	void DMSSimConfigParserImpl::ValidateParameters() {
		for (auto& CoordinateSpace : TransformList_.CoordinateSpaces_) { CoordinateSpace.Validate(); }
	}

	YamlObj* DMSSimConfigParserImpl::EventHandler_project(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) { ThrowExceptionWithLineN("Project block must be on the base level", Event); }
		if (!Enter) {
			Project_.assign(reinterpret_cast<const char*>(Event->data.scalar.value));
			if (Project_.empty()) { ThrowExceptionWithLineN("Empty project name", Event); }
		}
		return nullptr;
	}

	YamlObj* DMSSimConfigParserImpl::EventHandler_transform_base_to_project(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (Obj) { ThrowExceptionWithLineN("transform_base_to_project block must be on the base level", Event); }
		if (Enter) { return &TransformList_; }
		return nullptr;
	}

	YamlObj* DMSSimConfigParserImpl::EventHandler_car_coordinate_space(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeTransformList) { ThrowExceptionWithLineN("noise property belongs to camera block", Event); }
		if (Enter) {
			const auto CarModel = reinterpret_cast<const char*>(Event->data.scalar.value);
			const auto TransformListObj = static_cast<DMSSimTransformList*>(Obj);
			auto& TranformList = TransformListObj->CoordinateSpaces_;
			DMSSimCoordinateSpaceImpl CoordinateSpace;
			CoordinateSpace.CarModel_ = CarModel;
			TranformList.emplace_back(CoordinateSpace);
			return &TranformList.back();
		}
		return nullptr;
	}

	YamlObj* DMSSimConfigParserImpl::EventHandler_rotation(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCoordinateSpace) { ThrowExceptionWithLineN("rotation property belongs to transform_base_to_project", Event); }
		const auto CoordinateSpace = static_cast<DMSSimCoordinateSpaceImpl*>(Obj);
		if (!Enter) {
			auto& Value = CoordinateSpace->Rotation_;
			Value.Mark_ = Event->start_mark;
			Value.Values_.push_back(ParseFloat(Event, Event->data.scalar.value, "rotation"));
		}
		return CoordinateSpace;
	}

	YamlObj* DMSSimConfigParserImpl::EventHandler_translation(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCoordinateSpace) { ThrowExceptionWithLineN("translation property belongs to transform_base_to_project", Event); }
		const auto CoordinateSpace = static_cast<DMSSimCoordinateSpaceImpl*>(Obj);
		if (!Enter) {
			auto& Value = CoordinateSpace->Translation_;
			Value.Mark_ = Event->start_mark;
			Value.Values_.push_back(ParseFloat(Event, Event->data.scalar.value, "translation"));
		}
		return CoordinateSpace;
	}

	YamlObj* DMSSimConfigParserImpl::EventHandler_scale(const yaml_event_t* Event, YamlObj* Obj, bool Enter) {
		if (!Obj || Obj->GetYamlType() != YamlObjTypeCoordinateSpace) { ThrowExceptionWithLineN("scale property belongs to transform_base_to_project", Event); }
		const auto CoordinateSpace = static_cast<DMSSimCoordinateSpaceImpl*>(Obj);
		if (!Enter) {
			auto& Value = CoordinateSpace->Scale_;
			Value.Mark_ = Event->start_mark;
			const auto ValueFloat = ParseFloat(Event, Event->data.scalar.value, "scale");
			if (std::abs(ValueFloat) < DMSSIM_MIN_SCALE_VALUE) { ThrowExceptionWithLineN((std::string("scale modulo values must be greater than ") + std::to_string(DMSSIM_MIN_SCALE_VALUE)).c_str(), Event); }
			Value.Values_.push_back(ValueFloat);
		}
		return CoordinateSpace;
	}
} // anonymous namespace

DMSSimConfigParser* DMSSimConfigParser::Create(const wchar_t* const FilePath) {
	auto Parser = std::make_unique<DMSSimConfigParserImpl>();
	if (Parser->Initialize(FilePath)) { return Parser.release(); }
	return nullptr;
}