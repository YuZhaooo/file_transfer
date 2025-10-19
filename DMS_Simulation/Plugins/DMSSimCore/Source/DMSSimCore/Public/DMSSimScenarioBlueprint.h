/**
 * The set of Unreal Blueprint functions to fetch information about the car occupants, their animations, the simulation's environment and the camera/car/rendering settings.
 */
#pragma once
#include "CoreMinimal.h"
#include "Camera/CameraComponent.h"
#include "Containers/EnumAsByte.h"
#include "Curves/CurveVector.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DMSSimOccupantType.h"
#include "DMSSimScenarioBlueprint.generated.h"


/**
 * @struct FDMSSimMontage
 * @brief FDMSSimMontage is a block an animation channel consists of.
 * The structure can contain either a pointer to a recorded animation @Montage" or a pointer to a parametric curve @Curve.
 * Recorded animations and parametric animations curves cannot be used simultaneously.
 *
 * @var FDMSSimMontage::Montage         The pointer to a recorded animation
 * @var FDMSSimMontage::Time            The length of the animation
 * @var FDMSSimMontage::BlendIn         The time of overlapping with the previous animation.
 * @var FDMSSimMontage::BlendOut        The time of overlapping with the next animation.
 * @var FDMSSimMontage::Curve           The pointer to a parametric curve
 * @var FDMSSimMontage::Recorded        true if the @Montage pointer is to be used, or false if @Curve is valid
 */
USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSSimMontage
{
	GENERATED_BODY()
public:
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	UAnimMontage* Montage = nullptr;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float Time = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float BlendIn = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float BlendOut = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	UCurveVector* Curve = nullptr;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	bool Recorded = false; // recorded or parametric
};

/**
 * @struct FDMSSimAnimationContainer
 * @brief Contains animations for all animation channels
 *
 * @var FDMSSimMontage::CommonChannel         Common Channel animations, not to be used, left for legacy purposes
 * @var FDMSSimMontage::EyeGazeChannel        Eye Gaze Channel animations
 * @var FDMSSimMontage::EyelidsChannel        Eyelids Channel animations
 * @var FDMSSimMontage::FaceChannel           Face Channel animations
 * @var FDMSSimMontage::HeadChannel           Head Channel animations
 * @var FDMSSimMontage::UpperBodyChannel      Upper Body Channel animations
 * @var FDMSSimMontage::LeftHandChannel       Left Hand Channel animations
 * @var FDMSSimMontage::RightHandChannel      Right Hand Channel animations
 * @var FDMSSimMontage::SteeringWheelChannel  Steering Wheel Channel animations, used only for the steering wheel animation
 */
USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSSimAnimationContainer
{
	GENERATED_BODY()
public:
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	TArray<FDMSSimMontage> CommonChannel;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	TArray<FDMSSimMontage> EyeGazeChannel;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	TArray<FDMSSimMontage> EyelidsChannel;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	TArray<FDMSSimMontage> FaceChannel;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	TArray<FDMSSimMontage> HeadChannel;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	TArray<FDMSSimMontage> UpperBodyChannel;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	TArray<FDMSSimMontage> LeftHandChannel;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	TArray<FDMSSimMontage> RightHandChannel;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	TArray<FDMSSimMontage> SteeringWheelChannel;

	/** Clears all animation channels */
	void Reset()
	{
		CommonChannel.Empty();
		EyeGazeChannel.Empty();
		EyelidsChannel.Empty();
		FaceChannel.Empty();
		HeadChannel.Empty();
		UpperBodyChannel.Empty();
		LeftHandChannel.Empty();
		RightHandChannel.Empty();
		SteeringWheelChannel.Empty();
	}
};

USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSSimSteeringWheelColumn
{
	GENERATED_BODY()
public:
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	bool IsCameraIntegrated = false;
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float PitchAngle = 0.f;
};


/**
 * @struct FDMSSimCustomLight
 * @brief Contains parameters of an additional spot light
 * 
 * @var FDMSSimCustomLight::Intensity         Intensity of light, in candela
 * @var FDMSSimCustomLight::AttenuationRadius Attenuation distance
 * @var FDMSSimCustomLight::SourceRadius	  Radius of the illumination lamp
 * @var FDMSSimCustomLight::InnerConeAngle    Inner Cone Angle of the Spot Light, in degrees
 * @var FDMSSimCustomLight::OuterConeAngle    Outer Cone Angle of the Spot Light, in degrees
 * @var FDMSSimCustomLight::Position          Position of the Spot Light
 * @var FDMSSimCustomLight::Rotation          Direction of the Spot Light
 */
USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSSimCustomLight
{
	GENERATED_BODY()
public:
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float Intensity = 0;
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float AttenuationRadius = 0;
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float SourceRadius = 0;
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float InnerConeAngle = 0;
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float OuterConeAngle = 0;
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	FVector Position = {};
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	FRotator Rotation = {};
};

/**
 * @struct FDMSSimSunLight
 * @brief Contains parameters of sun light
 *
 * @var FDMSSimSunLight::Intensity         Intensity of light, in candela
 * @var FDMSSimSunLight::Temperature       Temperature of light
 * @var FDMSSimSunLight::Rotation          Rotation of the Sun Light
 */
USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSSimSunLight
{
	GENERATED_BODY()
public:
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float Intensity = 0;
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float Temperature = 0;
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FRotator Rotation = {};
};

/**
 * @struct FDMSBoundingBox2D
 * @brief Contains bounding box parameter
 *
 * @var FDMSBoundingBox2D::Center	2D Center of bounding box
 * @var FDMSBoundingBox2D::Width	width of bounding box
 * @var FDMSBoundingBox2D::Height	height of bounding box
 */
USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSBoundingBox2D
{
	GENERATED_BODY()
public:
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FVector2D Center = { 0.f, 0.f };
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float Width = 0.f;
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float Height = 0.f;
};

/**
 * @struct FDMSBoundingBox3D
 * @brief Contains bounding box parameter
 *
 * @var FDMSBoundingBox3D::Center	3D Center of bounding box
 * @var FDMSBoundingBox3D::Width	width of bounding box
 * @var FDMSBoundingBox3D::Height	height of bounding box
 * @var FDMSBoundingBox3D::Length	height of bounding box
 */
USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSBoundingBox3D
{
	GENERATED_BODY()
public:
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FVector Center = { 0.f, 0.f, 0.f };
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float Width = 0.f;
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float Height = 0.f;
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float Length = 0.f;
};


/**
 * @struct FDMSSimGlasses
 * @brief Occupant's glasses information
 *
 * @var FDMSSimGlasses::Model      Index of the glasses' model
 * @var FDMSSimGlasses::Color      The glasses' color
 * @var FDMSSimGlasses::Opacity    The glasses' opacity
 * @var FDMSSimGlasses::Reflective If a glare should be added
 */
USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSSimGlasses
{
	GENERATED_BODY()
public:
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	int32   Model = 0;
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	FVector Color = {};
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float   Opacity = 0;
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	bool   Reflective = 0;
};

/**
 * @struct FDMSSimOccupant
 * @brief Occupant's parameters
 *
 * @var FDMSSimOccupant::Type        Occupant type: Driver, FrontPassenger, etc.
 * @var FDMSSimOccupant::Character   Character model name
 * @var FDMSSimOccupant::Headgear    Headgear index
 * @var FDMSSimOccupant::Glasses     Glasses information
 * @var FDMSSimOccupant::Mask        Mask index
 * @var FDMSSimOccupant::Scarf       Scarf index
 * @var FDMSSimOccupant::Hair        Hair index
 * @var FDMSSimOccupant::Beard       Beard index
 * @var FDMSSimOccupant::Mustache    The glasses' opacity
 * @var FDMSSimOccupant::Pupil       Eye pupil brightness
 * @var FDMSSimOccupant::Sclera      Eye sclera brightness
 * @var FDMSSimOccupant::Height      Occupant's height
 * @var FDMSSimOccupant::SeatOffset  Occupant's seat offset, relative to its standard position
 *
 */
USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSSimOccupant
{
	GENERATED_BODY()
public:
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	FDMSSimOccupantType Type = FDMSSimOccupantType::Driver;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	FString Character;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	FString Uppercloth;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	int32 Headgear = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	FDMSSimGlasses Glasses;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	int32 Mask = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	int32 Scarf = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	int32 Hair = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	int32 Beard = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	int32 Mustache = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float PupilSize = 1.12;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float PupilBrightness = 0.f;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float IrisSize = 0.53f;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float IrisBrightness = 1.f;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float LimbusDarkAmount = 0.5f;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float IrisBorderWidth = 0.04f;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	FString IrisColor;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float ScleraBrightness = 0.5f;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float ScleraVeins = 0.5f;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float SkinWrinkles = 1.f;
	
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float SkinRoughness = 0.5f;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float SkinSpecularity = 0.43f;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float Height = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	FVector SeatOffset = {};
};


/**
 * @struct FDMSGroundTruthSettings
 * @brief Settings for adjusting the ground truth, e.g. Paddings for Bounding Boxes.
 * 
 *
 * @var FDMSGroundTruthSettings::BoundingBoxPaddingFactorFace  Padding factor for face bounding box
 * @var FDMSGroundTruthSettings::EyeBoundingBoxWidthFactor  Enlarges the width of the Eye 3D Bounding Box (Base are the two eye corners)
 * @var FDMSGroundTruthSettings::EyeBoundingBoxHeightFactor  Enlarges the height of the Eye 3D Bounding Box (Base are the upper and lower eye lid center)
 * @var FDMSGroundTruthSettings::EyeBoundingBoxDepth  Defines the depth of the Eye 3D Bounding Box (in cm)
 */
USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSGroundTruthSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float BoundingBoxPaddingFactorFace = 1.0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float EyeBoundingBoxWidthFactor = 1.0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float EyeBoundingBoxHeightFactor = 1.0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	float EyeBoundingBoxDepth = 3.0;
};


/**
 * @struct FDMSCamera
 * @brief Camera configuration for DMS simulation
 *
 * @var FDMSCamera::MinimalViewInfo               Basic view information for the camera
 * @var FDMSCamera::NIR                           Indicates if Near-Infrared (NIR) is used, if not RGB is used
 * @var FDMSCamera::FrameSize                     Size of the camera frame
 * @var FDMSCamera::FrameRate                     Frame rate of the camera
 * @var FDMSCamera::Mirrored                      Indicates if the camera view is mirrored
 * @var	FDMSCamera::GrainIntensity				  Noise grain intensity
 * @var	FDMSCamera::GrainJitter					  Noise grain jitter
 * @var	FDMSCamera::Saturation					  Saturation of the image
 * @var	FDMSCamera::Gamma						  Gamma correction in the image
 * @var	FDMSCamera::Contrast					  Contrast in the image
 * @var	FDMSCamera::BloomIntensity				  Bloom to soften highlights
 * @var	FDMSCamera::FocusOffset					  Offset to intentionally put the character out of focus (focus is on the nose)
 */
USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSCamera
{
	GENERATED_BODY()
public:
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FMinimalViewInfo MinimalViewInfo = {};

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		bool NIR = 0;
	
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FVector2D FrameSize = {};

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		int32 FrameRate = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		bool Mirrored = false;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float GrainIntensity = 0.0f;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float GrainJitter = 0.0f;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float Saturation= 1.0f;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float Gamma= 1.0f;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float Contrast= 1.0f;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float BloomIntensity = 0.0f;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float FocusOffset = 0.0f;
};

/**
 * @struct FDMSBlendoutDefaults
 * @brief Default blendout settings for various body parts in DMS simulation
 *
 * @var FDMSBlendoutDefaults::Common        Common blendout setting
 * @var FDMSBlendoutDefaults::EyeGaze       Blendout setting for eye gaze
 * @var FDMSBlendoutDefaults::Eyelids       Blendout setting for eyelids
 * @var FDMSBlendoutDefaults::Face          Blendout setting for face
 * @var FDMSBlendoutDefaults::Head          Blendout setting for head
 * @var FDMSBlendoutDefaults::UpperBody     Blendout setting for upper body
 * @var FDMSBlendoutDefaults::LeftHand      Blendout setting for left hand
 * @var FDMSBlendoutDefaults::RightHand     Blendout setting for right hand
 * @var FDMSBlendoutDefaults::SteeringWheel Blendout setting for steering wheel
 */
USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSBlendoutDefaults
{
	GENERATED_BODY()
public:
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float Common = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float EyeGaze = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float Eyelids = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float Face = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float Head = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float UpperBody = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float LeftHand = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float RightHand = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float SteeringWheel = 0;

};

USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSRandomMovements
{
	GENERATED_BODY()
public:
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	bool Blinking = false;
	
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	bool Smiling = false;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	bool Head = false;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	bool Body = false;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	bool Gaze = false;
};

/**
 * @struct FDMSScenario
 * @brief Scenario configuration for DMS simulation
 *
 * @var FDMSScenario::ScenarioIdx          Index of the scenario
 * @var FDMSScenario::ScenariosCount       Total number of scenarios available
 * @var FDMSScenario::VersionMajor         Major version of the scenario configuration
 * @var FDMSScenario::VersionMinor         Minor version of the scenario configuration
 * @var FDMSScenario::Description          Description of the scenario
 * @var FDMSScenario::Environment          Environment setting for the scenario
 * @var FDMSScenario::RandomMovements      True if the small movements in postprocessing are enabled
 * @var FDMSScenario::CarModel             Model of the car used in the scenario
 * @var FDMSScenario::CarSpeed             Speed of the car in the scenario
 * @var FDMSScenario::BlendoutDefaults     Default blendout settings for the scenario
 * @var FDMSScenario::Camera               Camera configuration for the scenario
 * @var FDMSScenario::CameraLight          Custom light settings for the camera in the scenario
 * @var FDMSScenario::SunLight             Sunlight settings for the scenario
 */
USTRUCT(BlueprintType)
struct DMSSIMCORE_API FDMSScenario
{
	GENERATED_BODY()
public:
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		int32 ScenarioIdx = 0;
	
	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		int32 ScenariosCount = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		int32 VersionMajor = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		int32 VersionMinor = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FString Description = "";

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FString Environment = "";

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
	FDMSRandomMovements RandomMovements = {};

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FString CarModel = "";

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		float CarSpeed = 0;

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FDMSBlendoutDefaults BlendoutDefaults = {};

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FDMSCamera Camera = {};

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FDMSGroundTruthSettings GroundTruthSettings = {};

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FDMSSimCustomLight CameraLight = {};

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FDMSSimSteeringWheelColumn SteeringWheelColumn = {};

	UPROPERTY(Category = "DmsSimulation", BlueprintReadWrite, EditAnywhere)
		FDMSSimSunLight SunLight = {};
};


UCLASS()
class UDMSSimScenarioBlueprint : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	/**
	 * Utility function, needed for internal logic.
	 * @return true is called in the final DMS application, not in the Unreal Editor
	 */
	UFUNCTION(BlueprintCallable, Category = "DMSSimCore")
	static bool IsDmsExecutable();


	/**
	 * The function parses a scenario and returns car/camera/light/driver information.
	 *
	 * @param[in]  Path           The full path to the scenario
	 * @param[in]  ScenarioIndex  Index of used scenario from the loaded vector of scenarios
	 * @param[out] Occupants      The information about occupants
	 * @param[out] ErrorMessage   Error message, in case of failure during scenario parsing
	 * @param[out] Scenario       All information about the scenario (camera, light, car, description,...)
	 * @param[out] CarSpeed       The car speed, in km/h
	 *
	 * @return true in case of success
	 */
	UFUNCTION(BlueprintCallable, Category = "DMSSimCore")
		static bool LoadDmsScenarioMulti(
			const FString& Path,
			const int32& ScenarioIndex,
			TArray<FDMSSimOccupant>& Occupants,
			FString& ErrorMessage,
			FDMSScenario& Scenario,
			float& CarSpeed);


	/**
	* The function clears the array of scenario parsers 
	 */
	UFUNCTION(BlueprintCallable, Category = "DMSSimCore")
	static void ResetScenarios();


	/**
	 * Return the name of the enviroment - Forest, Urban, etc.
	 *
	 * @param[in]  Path         The full path to the scenario
	 * @param[out] Environment  The environment's name
	 * @param[out] ErrorMessage Error message, in case of failure during scenario parsing
	 *
	 * @return true in case of success
	 */
	UFUNCTION(BlueprintCallable, Category = "DMSSimCore")
	static bool GetDmsEnvironment(
			const FString& Path,
			const int32& ScenarioIndex,
			FString& Environment,
			FString& ErrorMessage);


	/**
	 * Returns the list of animations for the driver
	 *
	 * @param[in] OccupantType   The occupant to fetch animations for
	 * @param[in] ResetAnimations  Clears the animation container for a new scenario.
	 * @param[in] FaceSkeleton   Face Skeleton to bind recorded animations to. Used for EyeGaze Channel, Eyelids Channel, Face Channel.
	 * @param[in] BodySkeleton   Body Skeleton to bind recorded animations to. Used for Head Channel, UpperBody Channel, LeftHand Channel, RightHand Channel
	 * @param[out] Animations     Array of animations for each channel.
	 * @param[out] TotalTime      Total Time of the simulation for the occupant.
	 * @param[out] ErrorMessage   Error message, in case of failure during scenario parsing
	 *
	 * @return true in case of success
	 */
	UFUNCTION(BlueprintCallable, Category = "DmsSimCore")
	static bool GetDmsAnimationsMulti(
		FDMSSimOccupantType OccupantType,
		bool ResetAnimations,
		USkeleton* FaceSkeleton,
		USkeleton* BodySkeleton,
		FDMSSimAnimationContainer& Animations,
		float& TotalTime,
		FString& ErrorMessage);

	/**
	 * Returns information about random movements
	 *
	 * @return true if random movements are enabled
	 */
	UFUNCTION(BlueprintCallable, Category = "DmsSimCore")
	static FDMSRandomMovements GetRandomMovementsStatus();


	/**
	 * Returns random value from range
	 *
	 * @param[in] Min   Minimal value of output
	 * @param[in] Max   Maximal value of output
	 *
	 * @return random value in range [Min, Max]
	 */
	UFUNCTION(BlueprintCallable, Category = "DmsSimCore")
	static float GetRandomFloat(
		const float& Min,
		const float& Max);
};
