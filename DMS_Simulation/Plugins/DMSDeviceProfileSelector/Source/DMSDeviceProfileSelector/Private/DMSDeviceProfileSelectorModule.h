#pragma once

#include "CoreMinimal.h"
#include "IDeviceProfileSelectorModule.h"

class FDMSDeviceProfileSelectorModule
	: public IDeviceProfileSelectorModule
{
public:
	virtual ~FDMSDeviceProfileSelectorModule();

	virtual const FString GetRuntimeDeviceProfileName() override;

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
