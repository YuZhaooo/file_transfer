#pragma once

#include "GroomBindingAsset.h"

namespace DMSSimGroomBindingBuilder
{
	/**
	 * By default, groom binding resource is supported only in the Unreal Editor.
	 * To be able to generate groom binding assets at runtime, a workaround (BuildBinding_CPU) is used.
	 * It basically, in most part, copies the code from the Unreal Engine.
	 */
	bool BuildBinding_CPU(UGroomBindingAsset* BindingAsset, bool bInitResources);
}
