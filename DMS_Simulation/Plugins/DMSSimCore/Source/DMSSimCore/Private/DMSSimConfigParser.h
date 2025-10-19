#pragma once

#include <Math/Rotator.h>
#include <Math/Vector.h>

constexpr char DMSSIM_DEFAULT_COORDINATE_SPACE_NAME[] = "default";
constexpr float DMSSIM_MIN_SCALE_VALUE = 0.001f;

class DMSSimCoordinateSpace {
protected:
	virtual ~DMSSimCoordinateSpace() {}
public:
	virtual const char* GetCarModel() const = 0;
	virtual FRotator GetRotation() const = 0;
	virtual FVector  GetTranslation() const = 0;
	virtual FVector  GetScale() const = 0;
};

/**
 * @class DMSSimConfigParser
 * @brief DMSSimConfigParser is a tree-like representation of a configuration yaml file.
 * In general, it contains coordinate spaces @DMSSimCoordinateSpace for different car models.
 */
class DMSSimConfigParser {
public:
	virtual ~DMSSimConfigParser() {}

	virtual unsigned GetVersionMajor() const = 0;
	virtual unsigned GetVersionMinor() const = 0;
	virtual const char* GetProject() const = 0;
	virtual size_t GetCoordinateSpaceCount() const = 0;
	virtual const DMSSimCoordinateSpace& GetCoordinateSpace(size_t SpaceIndex) const = 0;

	static DMSSimConfigParser* Create(const wchar_t* const FilePath);
};
