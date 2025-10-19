#pragma once
#include "DMSSimConfig.h"

#include <map>
#include <type_traits>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

template<typename Derived>
class DMSSimImageLabeler {
protected:
	~DMSSimImageLabeler() = default;

public:
	DMSSimImageLabeler(const std::wstring& FileName): BaseFileName_(FileName) {};

	Derived& derived() { return static_cast<Derived&>(*this); }
	Derived const& derived() const { return static_cast<Derived const&>(*this); }

	// NOTE: We are saving the json to the previous image frame. That's why we are using mainly PrevGroundTruth.
	// ONLY The occlusion / visibility data is lagging one frame behind, which is why we are using the ground truth from the future (here "GroundTruth")
	void AddFrame(const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth, int FrameIdx) {  derived().AddFrame(PrevGroundTruth, GroundTruth, FrameIdx); }

protected:
	const std::wstring&    BaseFileName_;
};

class DMSSimImageLabelerImpl : public DMSSimImageLabeler<DMSSimImageLabelerImpl> {
public:
	DMSSimImageLabelerImpl(const std::wstring& FileName) : DMSSimImageLabeler<DMSSimImageLabelerImpl>(FileName) {};
	~DMSSimImageLabelerImpl() {};

	void AddFrame(const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth, int FrameIdx);

private:
	rapidjson::Document CreateLabelFile(const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth);
};

class DMSSimImageLabelerOldImpl : public DMSSimImageLabeler<DMSSimImageLabelerOldImpl> {
public:
	DMSSimImageLabelerOldImpl(const std::wstring& FileName) : DMSSimImageLabeler<DMSSimImageLabelerOldImpl>(FileName) {};
	~DMSSimImageLabelerOldImpl() {};

	void AddFrame(const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth, int FrameIdx);

private:
	rapidjson::Document CreateLabelFileOld(const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth);
};

const std::string     LandmarkPupilIrisNames[9] = {
	"out",
	"out up",
	"up",
	"in up",
	"in",
	"in down",
	"down",
	"out down",
	"center"
};

static std::map<FString, float> MaxLeftEyeOpenings = {
	{"Gavin", 1.061462f},
	{"Hana", 1.031548f},
	{"Ada", 1.114868f},
	{"Benedikt", 1.155762f},
	{"Jesse", 1.017761f},
	{"Mylen", 1.075439f},
	{"Bernice", 1.049866f},
	{"Danielle", 1.014038f},
	{"Glenda", 1.276855f},
	{"Keiji", 1.046722f},
	{"Lucian", 1.17749f},
	{"Maria", 0.868713f},
	{"Neema", 1.272217f},
	{"Omar", 1.273193f},
	{"Roux", 1.225342f},
	{"Sook-ja", 0.959106f},
	{"Stephane", 1.20166f},
	{"Taro", 1.111389f},
	{"Trey", 1.105499f}
};

static std::map<FString, float> MaxRightEyeOpenings = {
	{"Gavin", 1.058929f},
	{"Hana", 1.035683f},
	{"Ada", 1.099243f},
	{"Benedikt", 1.184814f},
	{"Jesse", 1.024902f},
	{"Mylen", 1.067383f},
	{"Bernice", 1.040894f},
	{"Danielle", 1.036438f},
	{"Glenda", 1.20459f},
	{"Keiji", 1.092224f},
	{"Lucian", 1.086853f},
	{"Maria", 0.885132f},
	{"Neema", 1.196289f},
	{"Omar", 1.315552f},
	{"Roux", 1.196167f},
	{"Sook-ja", 0.967041f},
	{"Stephane", 1.128174f},
	{"Taro", 1.092712f},
	{"Trey", 1.09491f}
};


template<typename T>
inline rapidjson::Value CreateLabel(std::string LabelName, T LabelValue, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value NewLabel(rapidjson::kObjectType);
	NewLabel.AddMember("name", rapidjson::Value(LabelName.c_str(), allocator).Move(), allocator);
	NewLabel.AddMember("val", LabelValue, allocator);
	return NewLabel;
};

template<>
inline rapidjson::Value CreateLabel<std::string>(std::string LabelName, std::string LabelValue, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value NewLabel(rapidjson::kObjectType);
	NewLabel.AddMember("name", rapidjson::Value(LabelName.c_str(), allocator).Move(), allocator);
	NewLabel.AddMember("val", rapidjson::Value(LabelValue.c_str(), allocator).Move(), allocator);
	return NewLabel;
};

template<>
inline rapidjson::Value CreateLabel<FVector>(std::string LabelName, FVector LabelValue, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value NewLabel(rapidjson::kObjectType);
	NewLabel.AddMember("name", rapidjson::Value(LabelName.c_str(), allocator).Move(), allocator);
	rapidjson::Value ArrayValue(rapidjson::kArrayType);
	ArrayValue.PushBack(LabelValue.X, allocator);
	ArrayValue.PushBack(LabelValue.Y, allocator);
	ArrayValue.PushBack(LabelValue.Z, allocator);
	NewLabel.AddMember("val", ArrayValue.Move(), allocator);
	return NewLabel;
};

template<>
inline rapidjson::Value CreateLabel<FRotator>(std::string LabelName, FRotator LabelValue, rapidjson::Document::AllocatorType& allocator) {
	// roll, pitch, yaw
	rapidjson::Value NewLabel(rapidjson::kObjectType);
	NewLabel.AddMember("name", rapidjson::Value(LabelName.c_str(), allocator).Move(), allocator);
	rapidjson::Value ArrayValue(rapidjson::kArrayType);
	ArrayValue.PushBack(LabelValue.Roll, allocator);
	ArrayValue.PushBack(LabelValue.Pitch, allocator);
	ArrayValue.PushBack(LabelValue.Yaw, allocator);
	NewLabel.AddMember("val", ArrayValue.Move(), allocator);
	return NewLabel;
};

template<>
inline rapidjson::Value CreateLabel<FVector2D>(std::string LabelName, FVector2D LabelValue, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value NewLabel(rapidjson::kObjectType);
	NewLabel.AddMember("name", rapidjson::Value(LabelName.c_str(), allocator).Move(), allocator);
	rapidjson::Value ArrayValue(rapidjson::kArrayType);
	ArrayValue.PushBack(static_cast<int>(LabelValue.X + 0.5), allocator);
	ArrayValue.PushBack(static_cast<int>(LabelValue.Y + 0.5), allocator);
	NewLabel.AddMember("val", ArrayValue, allocator);
	return NewLabel;
};

template<>
inline rapidjson::Value CreateLabel<FDMSBoundingBox2D>(std::string LabelName, FDMSBoundingBox2D LabelValue, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value NewLabel(rapidjson::kObjectType);
	NewLabel.AddMember("name", rapidjson::Value(LabelName.c_str(), allocator).Move(), allocator);
	rapidjson::Value ArrayValue(rapidjson::kArrayType);
	ArrayValue.PushBack(static_cast<int>(LabelValue.Center.X + 0.5), allocator);
	ArrayValue.PushBack(static_cast<int>(LabelValue.Center.Y + 0.5), allocator);
	ArrayValue.PushBack(static_cast<int>(LabelValue.Width + 0.5), allocator);
	ArrayValue.PushBack(static_cast<int>(LabelValue.Height + 0.5), allocator);
	NewLabel.AddMember("val", ArrayValue, allocator);
	return NewLabel;
};

template<typename T>
inline void AddLabel(rapidjson::Value& ParentArray, std::string LabelName, T LabelValue, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value NewLabel = CreateLabel(std::move(LabelName), std::move(LabelValue), allocator);
	ParentArray.PushBack(NewLabel, allocator);
};

template<typename T>
inline void AddLabelWithCoordinates(rapidjson::Value& ParentArray, std::string LabelName, T LabelValue, std::string CoordinateSystem, rapidjson::Document::AllocatorType& allocator) {
	rapidjson::Value NewLabel = CreateLabel(std::move(LabelName), std::move(LabelValue), allocator);
	NewLabel.AddMember("coordinate_system", rapidjson::Value(std::move(CoordinateSystem).c_str(), allocator), allocator);
	ParentArray.PushBack(NewLabel, allocator);
};

template <typename T>
struct IsVectorOfNumbers {
	static constexpr bool value = false;
};

template <typename T>
struct IsVectorOfNumbers<std::vector<T>> {
	static constexpr bool value = std::is_integral<T>::value || std::is_floating_point<T>::value;
};

// Function to get the type category of the second element in a pair
template <typename T>
std::string GetSecondElementType() {
	if (std::is_same<T, bool>::value) { return "boolean"; }
	else if (std::is_integral<T>::value || std::is_floating_point<T>::value) { return "num"; }	
	else if (std::is_same<T, std::string>::value) { return "text"; }
	else if (IsVectorOfNumbers<T>::value || std::is_same<T, FVector>::value) { return "vec"; }
	return "unknown";
};

template<typename A>
inline void AddAttributes(rapidjson::Value& Label, rapidjson::Document::AllocatorType& allocator, A Attrs) {
	rapidjson::Value ArrayOfAttrs(rapidjson::kArrayType);
	for (auto& Attr : Attrs) { AddLabel(ArrayOfAttrs, Attr.first, Attr.second, allocator); }
	std::string NameOfType = GetSecondElementType<typename decltype(Attrs.begin()->second)>();
	Label.AddMember(rapidjson::Value(NameOfType.c_str(), allocator), ArrayOfAttrs, allocator);
};

template<typename A, typename... As>
inline void AddAttributes(rapidjson::Value& Label, rapidjson::Document::AllocatorType& allocator, A Attrs, As... OtherAttrs) {
	rapidjson::Value ArrayOfAttrs(rapidjson::kArrayType);
	for (auto& Attr : Attrs) { AddLabel(ArrayOfAttrs, Attr.first, Attr.second, allocator); }
	auto NameOfType = GetSecondElementType<decltype(Attrs.begin()->second)>();
	Label.AddMember(rapidjson::Value(NameOfType.c_str(), allocator), ArrayOfAttrs, allocator);
	AddAttributes(Label, allocator, OtherAttrs...);
};

template<typename T, typename... As>
inline void AddLabelWithAttributes(rapidjson::Value& ParentArray, std::string LabelName, T LabelValue, rapidjson::Document::AllocatorType& allocator, As... Attrs) {
	rapidjson::Value NewLabel = CreateLabel(std::move(LabelName), LabelValue, allocator);
	rapidjson::Value Attributes(rapidjson::kObjectType);
	AddAttributes(Attributes, allocator, Attrs...);
	NewLabel.AddMember("attributes", Attributes, allocator);
	ParentArray.PushBack(NewLabel, allocator);
};

template<typename T, typename... As>
inline void AddLabelWithCoordinatesAndAttributes(rapidjson::Value& ParentArray, std::string LabelName, T LabelValue, std::string CoordinateSystem, rapidjson::Document::AllocatorType& allocator, As... Attrs) {
	rapidjson::Value NewLabel = CreateLabel(std::move(LabelName), LabelValue, allocator);
	rapidjson::Value Attributes(rapidjson::kObjectType);
	AddAttributes(Attributes, allocator, Attrs...);
	NewLabel.AddMember("coordinate_system", rapidjson::Value(std::move(CoordinateSystem).c_str(), allocator), allocator);
	NewLabel.AddMember("attributes", Attributes, allocator);
	ParentArray.PushBack(NewLabel, allocator);
};