#include "DMSSimVideoRecordingRunable.h"
#include "DMSSimLog.h"
#include "DMSSimConfig.h"

DMSSimVideoRecordingRunable::DMSSimVideoRecordingRunable(std::wstring&& FileName, size_t SrcWidth, size_t SrcHeight, size_t DstWidth, size_t DstHeight, size_t FrameRate, bool Depth16Bit, bool Nir) :
    BaseFileName_(std::move(FileName)),
    Encoder_(DMSSimConfig::GetCurrentScenarioParser()->GetCamera().GetVideoOut() ?
        DMSSimVideoEncoder::CreateVideoEncoder(BaseFileName_, SrcWidth, SrcHeight, DstWidth, DstHeight, FrameRate, Depth16Bit, Nir) :
        DMSSimVideoEncoder::CreateVideoImageEncoder(BaseFileName_, SrcWidth, SrcHeight, DstWidth, DstHeight, FrameRate, Depth16Bit, Nir)),
    Labeler_(BaseFileName_),
    LabelerOld_(BaseFileName_)
{
    if (Encoder_) { Thread_ = FRunnableThread::Create(this, TEXT("DMS Sim Video Recording Thread")); }
}

uint32 DMSSimVideoRecordingRunable::Run() {
    DMSSimLog::Info() << "DMSSimVideoRecordingRunable  -- " << "Run" << FL;
    while (Active_ || PendingFrames_) {
        if (FrameQueue_.IsEmpty() || GroundTruthQueue_.IsEmpty()) {
            FPlatformProcess::Sleep(0.01f);
            continue;
        }
        while (!FrameQueue_.IsEmpty() && !GroundTruthQueue_.IsEmpty()) {
            ImagePtr Frame;
            TSharedPtr<DMSSimGroundTruthFrame> GroundTruth;
            FrameQueue_.Peek(Frame);
            GroundTruthQueue_.Peek(GroundTruth);

            if (Frame && GroundTruth) {
                if (PrevFrame_ && PrevGroundTruth_ && FrameIdx_ > 1) {
                    // Because the occlusion of facial landmarks is lagging one frame behind, 
                    // We need to wait for the second frame and apply its occlusion groundtruth to the previous frame 
                    Labeler_.AddFrame(PrevGroundTruth_, GroundTruth, FrameIdx_);
                    LabelerOld_.AddFrame(PrevGroundTruth_, GroundTruth, FrameIdx_);
                    Encoder_->AddFrame(*PrevFrame_, PrevGroundTruth_, GroundTruth, FrameIdx_);
                }

                PrevFrame_ = Frame;
                PrevGroundTruth_ = GroundTruth;
                FrameQueue_.Pop();
                GroundTruthQueue_.Pop();
                FrameIdx_++;
                PendingFrames_--;
            }
        }
    }
    Encoder_.Reset();
    DMSSimLog::Info() << "DMSSimVideoRecordingRunable  -- " << "Exit " << FL;
    return 0;
}
