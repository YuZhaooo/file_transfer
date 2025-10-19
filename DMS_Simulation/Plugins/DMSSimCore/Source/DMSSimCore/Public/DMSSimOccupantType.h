#pragma once

/**
 * @enum FDMSSimOccupantType
 * @brief Occupant types used in the Unreal Editor
 */
UENUM(BlueprintType)
enum class FDMSSimOccupantType: uint8
{
	Driver = 0 UMETA(DisplayName = "Driver"), // the Unreal preprocessor requires at least one item (default) to be set to 0.
	PassengerFront = 1 UMETA(DisplayName = "PassengerFront"),
	PassengerRearLeft = 2 UMETA(DisplayName = "PassengerRearLeft"),
	PassengerRearMiddle = 3 UMETA(DisplayName = "PassengerRearMiddle"),
	PassengerRearRight = 4 UMETA(DisplayName = "PassengerRearRight"),
	PassengerCount = 5 UMETA(DisplayName = "PassengerCount")
};
