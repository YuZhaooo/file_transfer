#include "DMSSimGazeUtils.h"
#include "Math/UnrealMathUtility.h"
#include "Kismet/KismetMathLibrary.h"

namespace {
	const float DMSSIM_EYE_UP_LIMIT = 30;
	const float DMSSIM_EYE_DOWN_LIMIT = 43;
	const float DMSSIM_EYE_INNER_LIMIT = 42.700001;
	const float DMSSIM_EYE_OUTER_LIMIT = 37.700001;
} // anonymous namespace

void UDMSSimGazeUtilsBlueprint::ComputeGazeParameters(
	const FDMSSimEyeType eye,
	const FVector& ViewTarget,
	const FVector& EyeLocation,
	const FRotator& HeadRotation,
	float& AngleLeft,
	float& AngleRight,
	float& AngleUp,
	float& AngleDown){

	AngleLeft = 0;
	AngleRight = 0;
	AngleUp = 0;
	AngleDown = 0;

	auto ViewDirection = (ViewTarget - EyeLocation);
	ViewDirection.Normalize();
	const auto UpVec = UKismetMathLibrary::GetForwardVector(HeadRotation);
	const auto SideVec = UKismetMathLibrary::GetUpVector(HeadRotation);
	const auto HeadDirection = UKismetMathLibrary::GetRightVector(HeadRotation);
	const auto ViewProj = UKismetMathLibrary::ProjectVectorOnToPlane(ViewDirection, HeadDirection);

	const auto x = FVector::DotProduct(SideVec, ViewProj);
	const auto y = FVector::DotProduct(UpVec, ViewProj);

	const auto xAngle = UKismetMathLibrary::DegAsin(x);
	const auto yAngle = UKismetMathLibrary::DegAsin(y);

	if (yAngle > 0.0f)
	{
		AngleUp = FMath::GetMappedRangeValueClamped(FVector2D(0, DMSSIM_EYE_UP_LIMIT), FVector2D(0, 1), yAngle);
	}
	else
	{
		AngleDown = FMath::GetMappedRangeValueClamped(FVector2D(0, DMSSIM_EYE_DOWN_LIMIT), FVector2D(0, 1), -yAngle);
	}

	auto InnerLimit = DMSSIM_EYE_INNER_LIMIT;
	auto OuterLimit = DMSSIM_EYE_OUTER_LIMIT;
	if (eye == FDMSSimEyeLeft) {
		InnerLimit = DMSSIM_EYE_OUTER_LIMIT;
		OuterLimit = DMSSIM_EYE_INNER_LIMIT;
	}

	if (xAngle > 0.0f)
	{
		AngleRight = FMath::GetMappedRangeValueClamped(FVector2D(0, InnerLimit), FVector2D(0, 1), xAngle);
	}
	else
	{
		AngleLeft = FMath::GetMappedRangeValueClamped(FVector2D(0, OuterLimit), FVector2D(0, 1), -xAngle);
	}
}
