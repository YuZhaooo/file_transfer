#pragma once

#include <Runtime\Core\Public\Math\Color.h>
#include "DMSSimRenderRequest.h"
#include "DMSSimVideoEncoder.h"
#include "DMSSimImageLabeler.h"
#include "DMSSimLog.h"

using ImagePtr = FDMSSimRenderRequest::ImagePtr;

/**
 * @class DMSSimVideoRecordingRunable
 * @brief Asynchronous worker that does actual video recording.
 */
class DMSSimVideoRecordingRunable : public FRunnable
{
public:
    DMSSimVideoRecordingRunable(std::wstring&& FileName, size_t SrcWidth, size_t SrcHeight, size_t DstWidth, size_t DstHeight, size_t FrameRate, bool Depth16Bit, bool Nir);
    virtual ~DMSSimVideoRecordingRunable(){};

    bool Init() override { return true; };
    uint32 Run() override;
    void Stop() override { Active_ = false; };
    void Exit() override { Done_ = true; };

    bool IsDone() const { return Done_; };
    void AddFrame(const ImagePtr& Frame, TSharedPtr<DMSSimGroundTruthFrame> GroundTruth) {
        FrameQueue_.Enqueue(Frame);
        GroundTruthQueue_.Enqueue(GroundTruth);
    };
    void IncPendingFrames() { ++PendingFrames_; };
    int GetNumPendingFrames() { return PendingFrames_; }

private:
    TQueue<ImagePtr>                                FrameQueue_;
    TQueue<TSharedPtr<DMSSimGroundTruthFrame> >     GroundTruthQueue_;
    ImagePtr                                        PrevFrame_ = MakeShareable(new TArray<FColor>);
    TSharedPtr<DMSSimGroundTruthFrame>              PrevGroundTruth_ = nullptr;
    FRunnableThread*                                Thread_ = nullptr;
    const std::wstring                              BaseFileName_;
    TUniquePtr<DMSSimVideoEncoder>                  Encoder_;
    DMSSimImageLabelerImpl                          Labeler_;
    DMSSimImageLabelerOldImpl                       LabelerOld_;
    bool                                            Active_ = true;
    bool                                            Done_ = false;
    int                                             FrameIdx_ = 0;
    int                                             PendingFrames_ = 0;
};
