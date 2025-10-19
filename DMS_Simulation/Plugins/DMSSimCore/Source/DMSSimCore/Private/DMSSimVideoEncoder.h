#pragma once
#include "DMSSimConfig.h"
#include "../Public/DMSSimRenderRequest.h"

/**
 * @class DMSSimVideoEncoder
 * @brief Video recording object. It creates a video stream with specified parameters and has a method to add frames to the stream.
 * The stream is finalized on the DMSSimVideoEncoder object destruction. The video recorder is in fact a wrapper around of FFMPEG library.
 * The sizes of the input frames and of the target video stream don't necessarily match, because the support for the vertical FOV and camera distortion can require frame rescaling.
 * The rescaling is performed by FFMPEG.
 * The video is saved in AVI format with H264 codec.
 */
class DMSSimVideoEncoder{
public:
	DMSSimVideoEncoder(const std::wstring& Filename, size_t SrcWidth, size_t SrcHeight, size_t DstWidth, size_t DstHeight, size_t FrameRate, bool Depth16Bit, bool Nir) :
		FileName_(Filename), SrcWidth_(SrcWidth), SrcHeight_(SrcHeight), DstWidth_(DstWidth), DstHeight_(DstHeight), FrameRate_(FrameRate), Depth16Bit_(Depth16Bit), Nir_(Nir) {};
	virtual ~DMSSimVideoEncoder(){};
	/**
	 * Adds a new frame to the stream.
	 *
	 * @param[in] Frame an array of pixels. The array's size must match the size of the source frame set during creation of the DMSSimVideoEncoder object.
	 */
	virtual void AddFrame(const TArray<FColor>& PrevImage, const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth, int FrameIdx) = 0;
	virtual bool Initialize() = 0;

	/**
	 * Creates a video recording object
	 *
	 * @param[in] FileName  Path to the target output stream.
	 * @param[in] SrcWidth  Width of the source frames
	 * @param[in] SrcHeight Height of the source frames
	 * @param[in] DstWidth  Width of the output video stream
	 * @param[in] DstHeight Height of the output video stream
	 * @param[in] FrameRate Frame Rate of the output video stream
	 *
	 * @return Video Encoder object, has to be deleted at the end of recording.
	 */
	static TUniquePtr<DMSSimVideoEncoder> CreateVideoEncoder(const std::wstring& FileName, size_t SrcWidth, size_t SrcHeight, size_t DstWidth, size_t DstHeight, size_t FrameRate, bool Depth16Bit, bool Nir);
	static TUniquePtr<DMSSimVideoEncoder> CreateVideoImageEncoder(const std::wstring& FileName, size_t SrcWidth, size_t SrcHeight, size_t DstWidth, size_t DstHeight, size_t FrameRate, bool Depth16Bit, bool Nir);

protected:	
	const std::wstring&    FileName_;
	size_t                SrcWidth_ = 0;
	size_t                SrcHeight_ = 0;
	size_t                DstWidth_ = 0;
	size_t                DstHeight_ = 0;
	size_t                FrameRate_ = 0;
	bool				  Depth16Bit_ = false;
	bool                  Nir_ = true;
};
