#pragma once
#include "UObject/Object.h"
#include "DMSSimRenderRequest.generated.h"

/**
 * @struct FDMSSimRenderRequest
 * @brief The structure contains one rendered frame. The structures are stored in UDMSSimRenderer::RenderRequestQueue_.
 * On every FCoreDelegates::OnEndFrame the render request queue is checked for ready frames,
 * that, in their turn, are sent to the video encoder DMSSimVideoRecordingRunable.
 *
 * @var FDMSSimRenderRequest::Image       Pixels of the frame that are populated by reading the offscreen surface, where the scene is rendered to.
 * @var FDMSSimRenderRequest::RenderFence Synchronization object. The user must wait for the readiness of the fence before the use of the image.
 */

USTRUCT()
struct FDMSSimRenderRequest {
	GENERATED_BODY()

	using ImagePtr = TSharedPtr<TArray<FColor> >;

	ImagePtr            Image;
	FRenderCommandFence RenderFence;
	FDMSSimRenderRequest()
		: Image(new TArray<FColor>){
	}
};