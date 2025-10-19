#include "DMSSimCameraBlueprint.h"
#include "DMSSimConfig.h"
#include "Camera/CameraActor.h"

void UDMSSimCamera::SetDmsCamera(UCameraComponent* const Camera)
{
	DMSSimConfig::SetCameraComponent(Camera);
}