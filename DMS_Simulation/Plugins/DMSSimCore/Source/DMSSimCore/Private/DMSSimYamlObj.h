#pragma once

#include <yaml.h>

enum YamlObjType {
	YamlObjTypeBase,
	YamlObjTypeEnvironment,
	YamlObjTypeCar,
	YamlObjTypeSun,
	YamlObjTypeCamera,
	YamlObjTypeIllumination,
	YamlObjTypeSteeringWheelColumn,
	YamlObjTypeGroundTruthSettings,
	YamlObjTypeChannelsParameters,
	YamlObjTypeDefaultBlendOutParameters,
	YamlObjTypeOccupant,
	YamlObjTypeOccupants,
	YamlObjTypeRandomMovements,
	YamlObjTypeOccupantScenario,
	YamlObjTypeAnimations,
	YamlObjTypeAnimationSequence,
	YamlObjTypeAnimationPoint,
	YamlObjTypeMotion,
	YamlObjTypeAnimationChannel,
	YamlObjTypeScenario,
	YamlObjTypeSeat,
	YamlObjTypeCoordinateSpace,
	YamlObjTypeTransformList,
};

/**
 * Base class for all intermediary objects used during parsing of yaml configuration files.
 */
class YamlObj {
public:
	YamlObj(YamlObjType Type): YamlType_(Type) {}
	virtual ~YamlObj() {}

	YamlObjType GetYamlType() const { return YamlType_; }
	virtual bool IsComplexSequence() const { return false; }
	virtual YamlObj* StartMapping(const yaml_event_t& Event) { return nullptr; }
	virtual void Validate() const {}

	bool          Sequence_ = false;
private:
	YamlObjType YamlType_;

	YamlObj() = delete;
};
