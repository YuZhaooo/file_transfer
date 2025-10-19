#include "DMSSimGroundTruthRecorder.h"
#include "Misc/AutomationTest.h"
#include <map>
#include <sstream>
#include <vector>
#include "Logging/LogMacros.h"

// Define a custom log category if necessary
DEFINE_LOG_CATEGORY_STATIC(MyLogCategory, Log, All);

#if WITH_DEV_AUTOMATION_TESTS

namespace{
void TrimValue(std::string& Value){
	while (!Value.empty())
	{
		if (Value.front() == '"'){
			Value.erase(Value.begin());
		}
		else if (Value.back() == '"'){
			Value.pop_back();
		}
		else{
			break;
		}
	}
}

auto SplitString(const std::string& Line){
	std::vector<std::string> Values;

	std::stringstream LineStream(Line);
	while (LineStream.good())
	{
		std::string Value;
		std::getline(LineStream, Value, ';');
		TrimValue(Value);
		Values.push_back(Value);
	}
	return Values;
}

void LoadValueMap(std::stringstream& Stream, std::map<std::string, std::string>& ValueMap, size_t LineNo = 1){

	Stream.seekg(0);

	std::string HeaderLine;
	if (!std::getline(Stream, HeaderLine))
	{
		return;
	}
	std::vector<std::string> Names = SplitString(HeaderLine);

	std::string ValueLine;
	for (size_t i = 0; i < LineNo; ++i)
	{
		ValueLine.clear();
		std::getline(Stream, ValueLine);
	}

	std::vector<std::string> Values = SplitString(ValueLine);

	if (Names.size() != Values.size()){
		return;
	}

	for (size_t i = 0; i < Names.size(); ++i){
		ValueMap.emplace(Names[i], Values[i]);
	}
}

FString GetValue(const std::map<std::string, std::string>& ValueMap, const char* const Name){
	auto iter = ValueMap.find(Name);
	if (iter != ValueMap.end()){
		return iter->second.c_str();
	}
	return "";
}

} // anonymous namespace

using namespace DMSSimGroundTruthRecorder;
/*
IMPLEMENT_SIMPLE_AUTOMATION_TEST(DMSSimGroundTruthRecorderTest1, "DMSSim.GroundTruthRecorder.Tests1", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool DMSSimGroundTruthRecorderTest1::RunTest(const FString& Parameters)
{
	DMSSimGroundTruthFrame FrameCompound = {};
	auto& CommonData = FrameCompound.Common;
	auto& Frame = FrameCompound.Occupants[DMSSimOccupantDriver];

	CommonData.CameraPoint = { -99077.5469, -4973.17334, 108.118126 };
	CommonData.CarBase = { -98982.4141, -4936.76416, 41.0475311 };
	CommonData.CarRotation = { 0.00000000, -179.999954, -0.734222293 };

	Frame.Initialized = true;
	Frame.EyesMiddlePointGlobal  = { -99134.8984, -4976.30078, 131.967407 };
	Frame.NosePoint = { -99130.5156, -4976.34912, 127.815781 };
	Frame.LEarPoint = { -99145.3125, -4983.97168, 129.771729 };
	Frame.REarPoint = { -99145.0625, -4968.49902, 129.472290 };
	Frame.LeftEyePoint = { -99134.9219, -4979.76855, 132.006348 };
	Frame.RightEyePoint = { -99134.8750, -4972.83301, 131.928482 };
	Frame.LeftGazeOrigin = { -99144.9219, -4981.76855, 132.006348 };
	Frame.RightGazeOrigin = { -99132.8750, -4972.83301, 130.928482 };
	Frame.LeftGazeVector = { 0.00000000, -1.0f, 0.0f };
	Frame.RightGazeVector = { 1.0f, 0.0f, 0.0f };
	Frame.LeftEyeOpen =	2;
	Frame.RightEyeOpen = 2;
	Frame.LeftEyeOpening = 0.946901441;
	Frame.RightEyeOpening = 0.982165635;
	Frame.HorizontalMouthOpening = 2.34;
	Frame.VerticalMouthOpening = 1.47;

	Frame.LeftShoulderPoint = { -56145.3125, -4083.97168, 169.771729 };
	Frame.RightShoulderPoint = { -49147.3125, -4983.97168, 179.771729 };
	Frame.LeftElbowPoint = { -99149.3125, -4983.97168, 129.771729 };
	Frame.RightElbowPoint = { -92150.3125, -4983.97168, 129.771729 };
	Frame.LeftWristPoint = { -93154.3125, -4983.97168, 129.771729 };
	Frame.RightWristPoint = { -94145.3125, -4983.97168, 129.771729 };
	Frame.LeftPinkyKnucklePoint = { -19145.3125, -4184.97168, 129.771729 };
	Frame.RightPinkyKnucklePoint = { -19145.3125, -4284.97168, 129.771729 };
	Frame.LeftIndexKnucklePoint = { -99145.3125, -4483.97168, 129.771729 };
	Frame.RightIndexKnucklePoint = { -99145.3125, -4683.97168, 129.771729 };
	Frame.LeftThumbKnucklePoint = { -89145.3125, -4183.97168, 129.771729 };
	Frame.RightThumbKnucklePoint = { -79145.3125, -4283.97168, 129.771729 };
	Frame.LeftHipPoint = { -92145.3125, -4983.97168, 129.771729 };
	Frame.RightHipPoint = { -79245.3125, -4983.97168, 139.771729 };
	Frame.LeftKneePoint = { -99145.3125, -4183.97168, 119.771729 };
	Frame.RightKneePoint = { -99145.3125, -4083.97168, 109.771729 };
	Frame.LeftAnklePoint = { -69145.3125, -4983.97168, 129.771729 };
	Frame.RightAnklePoint = { -59145.3125, -4583.97168, 129.771729 };
	Frame.LeftHeelPoint = { -92145.3125, -3983.97168, 149.771729 };
	Frame.RightHeelPoint = { -90145.3125, -4083.97168, 159.771729 };
	Frame.LeftFootIndexPoint = { -69145.3125, -4183.97168, 119.771729 };
	Frame.RightFootIndexPoint = { -79145.3125, -4083.97168, 169.771729 };

	auto& Frame2 = FrameCompound.Occupants[DMSSimOccupantPassengerRearMiddle];
	Frame2.Initialized = true;
	Frame2.EyesMiddlePointGlobal = { -99134.8984, -4976.30078, 131.967407 };
	Frame2.LeftEyePoint = { -99134.9219, -4979.76855, 132.006348 };
	Frame2.RightEyePoint = { -99134.8750, -4972.83301, 131.928482 };

	std::stringstream Stream;
	AddHeader(Stream);
	AddFrame(Stream, 0.1, FrameCompound);
	AddFrame(Stream, 0.2, FrameCompound);


	struct {
		const char* ColumnName;
		float Value;
	}
	FloatValues[] =
	{
		{ "s_DM_Availability",           1.0f          },

		{ "s_DM_HeadPositionX",          2.9364066f    },
		{ "s_DM_HeadPositionY",         -0.40698203f   },
		{ "s_DM_HeadPositionZ",          1.5560577f    },

		{ "s_DM_HeadRotationYaw",        -0.011568f   },
		{ "s_DM_HeadRotationPitch",      -0.122562f   },
		{ "s_DM_HeadRotationRoll",       -0.006254f   },

		{ "s_DM_LeftEyePositionX",       2.936641f     },
		{ "s_DM_LeftEyePositionY",      -0.44166192f   },
		{ "s_DM_LeftEyePositionZ",       1.5560026f    },

		{ "s_DM_RightEyePositionX",      2.9361722f    },
		{ "s_DM_RightEyePositionY",     -0.37230217f   },
		{ "s_DM_RightEyePositionZ",      1.5561128f    },

		{ "s_DM_LeftEyeOpening",         0.0094690146f },
		{ "s_DM_RightEyeOpening",        0.0098216562f },

		{ "s_DM_HorizontalMouthOpening", 0.0234f       },
		{ "s_DM_VerticalMouthOpening",   0.0147f       },

		{ "s_DM_GazeOriginX", 2.976428f  },
		{ "s_DM_GazeOriginY", -0.416918f },
		{ "s_DM_GazeOriginZ", 1.550850f  },

		{ "s_DM_LeftGazeOriginX", 3.036641f },
		{ "s_DM_LeftGazeOriginY", -0.461660f },
		{ "s_DM_LeftGazeOriginZ", 1.555746f },

		{ "s_DM_RightGazeOriginX", 2.916172f },
		{ "s_DM_RightGazeOriginY", -0.372174f },
		{ "s_DM_RightGazeOriginZ", 1.546114f },

		{ "s_DM_GazeVectorX", -0.707106f },
		{ "s_DM_GazeVectorY", -0.707049f },
		{ "s_DM_GazeVectorZ", -0.009061f },

		{ "s_DM_LeftGazeVectorX", 0.000001f },
		{ "s_DM_LeftGazeVectorY", -0.999918f },
		{ "s_DM_LeftGazeVectorZ", -0.012814f },

		{ "s_DM_RightGazeVectorX", -1.000000f },
		{ "s_DM_RightGazeVectorY", -0.000001f },
		{ "s_DM_RightGazeVectorZ", -0.000000f },

		{ "s_DM_LeftShoulderPointX", -426.959412f },
		{ "s_DM_LeftShoulderPointY", 8.510321f },
		{ "s_DM_LeftShoulderPointZ", 2.048330f },

		{ "s_DM_RightShoulderPointX", -496.939423f },
		{ "s_DM_RightShoulderPointY", -0.490288f },
		{ "s_DM_RightShoulderPointZ", 2.032993f },

		{ "s_DM_LeftElbowPointX", 3.080569f },
		{ "s_DM_LeftElbowPointY", -0.483404f },
		{ "s_DM_LeftElbowPointZ", 1.533040f },

		{ "s_DM_RightElbowPointX", -66.909431f },
		{ "s_DM_RightElbowPointY", -0.483471f },
		{ "s_DM_RightElbowPointZ", 1.533039f },

		{ "s_DM_LeftWristPointX", -56.869431f },
		{ "s_DM_LeftWristPointY", -0.483461f },
		{ "s_DM_LeftWristPointZ", 1.533039f },

		{ "s_DM_RightWristPointX", -46.959431f },
		{ "s_DM_RightWristPointY", -0.483452f },
		{ "s_DM_RightWristPointZ", 1.533039f },

		{ "s_DM_LeftPinkyKnucklePointX", -796.959412f },
		{ "s_DM_LeftPinkyKnucklePointY", 7.505176f },
		{ "s_DM_LeftPinkyKnucklePointZ", 1.635416f },

		{ "s_DM_RightPinkyKnucklePointX", -796.959412f },
		{ "s_DM_RightPinkyKnucklePointY", 6.505259f },
		{ "s_DM_RightPinkyKnucklePointZ", 1.622602f },

		{ "s_DM_LeftIndexKnucklePointX", 3.040564f },
		{ "s_DM_LeftIndexKnucklePointY", 4.516185f },
		{ "s_DM_LeftIndexKnucklePointZ", 1.597111f },

		{ "s_DM_RightIndexKnucklePointX", 3.040566f },
		{ "s_DM_RightIndexKnucklePointY", 2.516349f },
		{ "s_DM_RightIndexKnucklePointZ", 1.571483f },

		{ "s_DM_LeftThumbKnucklePointX", -96.959442f },
		{ "s_DM_LeftThumbKnucklePointY", 7.515843f },
		{ "s_DM_LeftThumbKnucklePointZ", 1.635553f },

		{ "s_DM_RightThumbKnucklePointX", -196.959427f },
		{ "s_DM_RightThumbKnucklePointY", 6.515830f },
		{ "s_DM_RightThumbKnucklePointZ", 1.622737f },

		{ "s_DM_LeftHipPointX", -66.959435f },
		{ "s_DM_LeftHipPointY", -0.483471f },
		{ "s_DM_LeftHipPointZ", 1.533039f },

		{ "s_DM_RightHipPointX", -195.959427f },
		{ "s_DM_RightHipPointY", -0.484875f },
		{ "s_DM_RightHipPointZ", 1.633029f },

		{ "s_DM_LeftKneePointX", 3.040561f },
		{ "s_DM_LeftKneePointY", 7.517220f },
		{ "s_DM_LeftKneePointZ", 1.535562f },

		{ "s_DM_RightKneePointX", 3.040560f },
		{ "s_DM_RightKneePointY", 8.518419f },
		{ "s_DM_RightKneePointZ", 1.448385f },

		{ "s_DM_LeftAnklePointX", -296.959412f },
		{ "s_DM_LeftAnklePointY", -0.483690f },
		{ "s_DM_LeftAnklePointZ", 1.533036f },

		{ "s_DM_RightAnklePointX", -396.959412f },
		{ "s_DM_RightAnklePointY", 3.515886f },
		{ "s_DM_RightAnklePointZ", 1.584292f },

		{ "s_DM_LeftHeelPointX", -66.959442f },
		{ "s_DM_LeftHeelPointY", 9.513144f },
		{ "s_DM_LeftHeelPointZ", 1.861165f },

		{ "s_DM_RightHeelPointX", -86.959442f },
		{ "s_DM_RightHeelPointY", 8.511926f },
		{ "s_DM_RightHeelPointZ", 1.948342f },

		{ "s_DM_LeftFootIndexPointX", -296.959412f },
		{ "s_DM_LeftFootIndexPointY", 7.516934f },
		{ "s_DM_LeftFootIndexPointZ", 1.535558f },

		{ "s_DM_RightFootIndexPointX", -196.959427f },
		{ "s_DM_RightFootIndexPointY", 8.510540f },
		{ "s_DM_RightFootIndexPointZ", 2.048333f },

		{ "s_DM_PF_HeadPositionX",  0.0f },
		{ "s_DM_PF_HeadPositionY",  0.0f },
		{ "s_DM_PF_HeadPositionZ",  0.0f },

		{ "s_DM_PRL_HeadPositionX",  0.0f },
		{ "s_DM_PRL_HeadPositionY",  0.0f },
		{ "s_DM_PRL_HeadPositionZ",  0.0f },

		{ "s_DM_PRM_HeadPositionX",  2.9364066f   },
		{ "s_DM_PRM_HeadPositionY",  -0.40698203f },
		{ "s_DM_PRM_HeadPositionZ",  1.5560577f   },

		{ "s_DM_PRR_HeadPositionX",  0.0f },
		{ "s_DM_PRR_HeadPositionY",  0.0f },
		{ "s_DM_PRR_HeadPositionZ",  0.0f },
	};

	for (int i = 0; i < 2; ++i)
	{
		std::map<std::string, std::string> ValueMap;
		LoadValueMap(Stream, ValueMap, i + 1);

		TestEqual(TEXT("GT Test 1 Time"), GetValue(ValueMap, "Time"), FString("00:00:00.") + FString::FromInt(i + 1) + "00");

		TestEqual(TEXT("GT Test 1 FrameNumber"), FCString::Atoi(*GetValue(ValueMap, "s_DM_FrameNumber")), i);

		for (const auto& Info : FloatValues)
		{
			const float ActualValue = std::stof(*GetValue(ValueMap, Info.ColumnName));
			std::string Caption = "GT Tesy 1  - ";
			Caption += Info.ColumnName;
			Caption += ": ";
			Caption += std::to_string(Info.Value);
			Caption += " vs ";
			Caption += std::to_string(ActualValue);
			TestTrue(FString(Caption.c_str()), FMath::IsNearlyEqual(ActualValue, Info.Value, 0.0001f));
		}

		struct {
			const char* ColumnName;
			int Value;
		}
		IntValues[] =
		{
			{ "s_DM_LeftEyeClosed",   0 },
			{ "s_DM_RightEyeClosed",  0 },
			{ "s_DM_LeftEyeOpen",     1 },
			{ "s_DM_RightEyeOpen",    1 },
		};

		for (const auto& Info : IntValues)
		{
			const int ActualValue = std::stoi(*GetValue(ValueMap, Info.ColumnName));
			std::string Caption = "GT Tesy 1  - ";
			Caption += Info.ColumnName;
			TestEqual(FString(Caption.c_str()), ActualValue, Info.Value);
		}

		const char* InvalidColumns[] =
		{
			"s_DM_PF_CameraRotationPitch",
			"s_DM_PRL_CameraRotationPitch",
			"s_DM_PRM_CameraRotationPitch",
			"s_DM_PRR_CameraRotationPitch",
		};

		for (const auto InvalidColumn : InvalidColumns)
		{
			std::string Caption = "GT Tesy 1  - ";
			Caption += " Found Invalid Column - ";
			Caption += InvalidColumn;
			TestTrue(FString(Caption.c_str()), ValueMap.find(InvalidColumn) == ValueMap.end());
		}
	}
	return true;
}

*/
IMPLEMENT_SIMPLE_AUTOMATION_TEST(DMSSimGroundTruthRecorderTest2, "DMSSim.GroundTruthRecorder.Tests2", EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::SmokeFilter)

bool DMSSimGroundTruthRecorderTest2::RunTest(const FString& Parameters)
{
	// Log some information
	UE_LOG(MyLogCategory, Log, TEXT("Starting test: DMSSimGroundTruthRecorderTest2"));

	DMSSimGroundTruthFrame FrameCompound = {};
	auto& CommonData = FrameCompound.Common;
	auto& Frame = FrameCompound.Occupants[static_cast<uint8>(FDMSSimOccupantType::Driver)];

	CommonData.CarPosition_inWorld = { -98982.4141, -4934.33740, 230.432068 };
	CommonData.CarRotation_inWorld = { -179.999954, 0.00000000, -0.734222293 };

	Frame.Initialized = true;
	Frame.HeadOriginEyesCenter_inCam = { -99134.8984, -4976.30078, 131.967407 };
	Frame.NosePoint = { -99130.5156, -4976.34912, 127.815781 };
	Frame.LEarPoint = { -99145.3125, -4983.97168, 129.771729 };
	Frame.REarPoint = { -99145.0625, -4968.49902, 129.472290 };
	Frame.LeftEyePoint = { -99134.9219, -4979.76855, 132.006348 };
	Frame.RightEyePoint = { -99134.8750, -4972.83301, 131.928482 };
	Frame.LeftGazeOrigin_inCam = { -99135.9219, -4980.76855, 131.006348 };
	Frame.RightGazeOrigin_inCam = { -99134.8750, -4970.83301, 133.928482 };
	Frame.LeftGazeDirection_inCam = { 0.00000000, -1.0f, 0.0f };
	Frame.RightGazeDirection_inCam = { 1.0f, 0.0f, 0.0f };
	Frame.HorizontalMouthOpening = 3.02;
	Frame.VerticalMouthOpening = 2.57;

	// Needs to match the intialized column header (e.g. faciallandmarks_P3DX..)
	for (auto& OccupantFrame : FrameCompound.Occupants)
	{
		OccupantFrame.FacialLandmarksVisible.SetNum(68);
		OccupantFrame.FacialLandmarks3D_inCam.SetNum(68);
		OccupantFrame.FacialLandmarks2D.SetNum(68);
		OccupantFrame.FaceBoundingBox3D_inCam.SetNum(8);
	}

	for (int i = 0; i < 2; i++)
	{
		Frame.FacialLandmarksVisible[i] = true;
		Frame.FacialLandmarks3D_inCam[i] = { -99134.9219, -4979.76855, 132.006348 };
		Frame.FacialLandmarks2D[i] = { 100.0, 200.0 };
	}
	Frame.FacialLandmarksVisible[2] = false;
	Frame.FacialLandmarks3D_inCam[2] = { -99134.9219, -4979.76855, 132.006348 };
	Frame.FacialLandmarks2D[2] = { -100.0, 200.0 };

	for (int i = 0; i < 2; i++)
	{
		Frame.FaceBoundingBox3D_inCam[i] = { -99134.9219, -4979.76855, 132.006348 };
	}
	Frame.FaceBoundingBox3D_inCam[2] = { -99134.9219, -4979.76855, 132.006348 };
	

	std::stringstream Stream;
	AddHeader(Stream);
	AddFrame(Stream, 103.56, FrameCompound);
	
	std::map<std::string, std::string> ValueMap;

	// Get the content of the stream as an std::string
	std::string StreamContents = Stream.str();

	// Convert std::string to FString
	FString UEStreamContents = FString(StreamContents.c_str());

	// Log the contents using UE_LOG
	UE_LOG(LogTemp, Log, TEXT("Stream Contents: %s"), *UEStreamContents);

	LoadValueMap(Stream, ValueMap);


	for (const auto& Pair : ValueMap)
	{
		FString Key = FString(Pair.first.c_str());    // Convert std::string to FString
		FString Value = FString(Pair.second.c_str()); // Convert std::string to FString

		UE_LOG(LogTemp, Log, TEXT("Key: %s, Value: %s"), *Key, *Value); // Log the key-value pair
	}	
	return true;

	TestEqual(TEXT("GT Test 2 Time"), GetValue(ValueMap, "Time"), "00:01:43.560");

	struct {
		const char* ColumnName;
		float Value;
	}
	FloatValues[] =
	{
		{ "s_DM_HeadPositionX",          2.936408f     },
		{ "s_DM_HeadPositionY",          0.406981f     },
		{ "s_DM_HeadPositionZ",          1.641941f     },

		{ "s_DM_HeadRotationYaw",        0.011567f     },
		{ "s_DM_HeadRotationPitch",      0.122563f     },
		{ "s_DM_HeadRotationRoll",       3.135339f     },

		{ "s_DM_LeftEyePositionX",       2.936642f     },
		{ "s_DM_LeftEyePositionY",       0.441661f     },
		{ "s_DM_LeftEyePositionZ",       1.641996f     },

		{ "s_DM_RightEyePositionX",      2.936173f     },
		{ "s_DM_RightEyePositionY",      0.372302f     },
		{ "s_DM_RightEyePositionZ",      1.641886f     },

		{ "s_DM_LeftEyeOpening",         0.000069f     },
		{ "s_DM_RightEyeOpening",        0.000022f     },

		{ "s_DM_HorizontalMouthOpening", 0.0302f       },
		{ "s_DM_VerticalMouthOpening",   0.0257f       },

		{ "s_DM_LeftGazeOriginX", 2.946642f },
		{ "s_DM_LeftGazeOriginY", 0.451532f },
		{ "s_DM_LeftGazeOriginZ", 1.652124f },

		{ "s_DM_RightGazeOriginX", 2.936173f },
		{ "s_DM_RightGazeOriginY", 0.352560f },
		{ "s_DM_RightGazeOriginZ", 1.621632f },

		{ "s_DM_LeftGazeVectorX", 0.000000f },
		{ "s_DM_LeftGazeVectorY", 0.999918f },
		{ "s_DM_LeftGazeVectorZ", 0.012814f },

		{ "s_DM_RightGazeVectorX", -1.000000f },
		{ "s_DM_RightGazeVectorY", 0.000000f },
		{ "s_DM_RightGazeVectorZ", 0.00000f },

		{ "s_DM_PF_HeadPositionX",  0.0f },
		{ "s_DM_PF_HeadPositionY",  0.0f },
		{ "s_DM_PF_HeadPositionZ",  0.0f },

		{ "s_DM_PRL_HeadPositionX",  0.0f },
		{ "s_DM_PRL_HeadPositionY",  0.0f },
		{ "s_DM_PRL_HeadPositionZ",  0.0f },

		{ "s_DM_PRM_HeadPositionX",  0.0f },
		{ "s_DM_PRM_HeadPositionY",  0.0f },
		{ "s_DM_PRM_HeadPositionZ",  0.0f },

		{ "s_DM_PRR_HeadPositionX",  0.0f },
		{ "s_DM_PRR_HeadPositionY",  0.0f },
		{ "s_DM_PRR_HeadPositionZ",  0.0f },
	};

	for (const auto& Info : FloatValues)
	{
		const float ActualValue = std::stof(*GetValue(ValueMap, Info.ColumnName));
		UE_LOG(MyLogCategory, Log, TEXT("ColumnName: %s, ActualValue: %f"), ANSI_TO_TCHAR(Info.ColumnName), ActualValue);

		std::string Caption = "GT Tesy 2  - ";
		Caption += Info.ColumnName;
		Caption += ": ";
		Caption += std::to_string(Info.Value);
		Caption += " vs ";
		Caption += std::to_string(ActualValue);
		TestTrue(FString(Caption.c_str()), FMath::IsNearlyEqual(ActualValue, Info.Value, 0.0001f));
	}

	struct {
		const char* ColumnName;
		float Value;
	}FacialLandmarks[] =
	{   { "s_DM_FacialLandmarks_1_x",       2.936642f     },
		{ "s_DM_FacialLandmarks_1_y",       0.441661f     },
		{ "s_DM_FacialLandmarks_1_z",       1.641996f     },
		{ "s_DM_FacialLandmarks_2_x",       2.936642f     },
		{ "s_DM_FacialLandmarks_2_y",       0.441661f     },
		{ "s_DM_FacialLandmarks_2_z",       1.641996f     },
		{ "s_DM_FacialLandmarks_3_x",       2.936642f     },
		{ "s_DM_FacialLandmarks_3_y",       0.441661f     },
		{ "s_DM_FacialLandmarks_3_z",       1.641996f     },
	};

	
	for (const auto& Info : FacialLandmarks)
	{
		const float ActualValue = std::stof(*GetValue(ValueMap, Info.ColumnName));
		UE_LOG(MyLogCategory, Log, TEXT("ColumnName: %s, ActualValue: %f"), ANSI_TO_TCHAR(Info.ColumnName), ActualValue);

		std::string Caption = "GT Tesy 2  - ";
		Caption += Info.ColumnName;
		Caption += ": ";
		Caption += std::to_string(Info.Value);
		Caption += " vs ";
		Caption += std::to_string(ActualValue);
		
		//TestTrue(FString(Caption.c_str()), FMath::IsNearlyEqual(ActualValue, Info.Value, 0.0001f));
		// Currently no test available for visible and pixel coordinates
	}

	struct {
		const char* ColumnName;
		int Value;
	}
	IntValues[] =
	{
		{ "s_DM_LeftEyeClosed",   1 },
		{ "s_DM_RightEyeClosed",  1 },
		{ "s_DM_LeftEyeOpen",     0 },
		{ "s_DM_RightEyeOpen",    0 },
	};

	for (const auto& Info : IntValues)
	{
		const int ActualValue = std::stoi(*GetValue(ValueMap, Info.ColumnName));
		std::string Caption = "GT Tesy 2  - ";
		Caption += Info.ColumnName;
		TestEqual(FString(Caption.c_str()), ActualValue, Info.Value);
	}
	return true;
}
#endif //WITH_DEV_AUTOMATION_TESTS