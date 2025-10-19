#include "DMSSimVideoEncoder.h"
#include "DMSSimLog.h"
#include "DMSSimUtils.h"
#include "DMSSimConfig.h"
#include <sstream>

extern "C" {
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

class DMSSimVideoImageEncoderImpl : public DMSSimVideoEncoder {
public:
	DMSSimVideoImageEncoderImpl(const std::wstring& FileName, size_t SrcWidth, size_t SrcHeight, size_t DstWidth, size_t DstHeight, size_t FrameRate, bool Depth16Bit, bool Nir)
		: DMSSimVideoEncoder(FileName, SrcWidth, SrcHeight, DstWidth, DstHeight, FrameRate, Depth16Bit, Nir) {};

	virtual ~DMSSimVideoImageEncoderImpl();
	bool Initialize() override;
	void AddFrame(const TArray<FColor>& PrevImage, const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth, int FrameIdx) override;

private:
	SwsContext*           ScaleContext_ = nullptr;
	AVFormatContext*      FormatContext_ = nullptr;
	AVStream*             Stream_ = nullptr;
	const AVCodec*        Codec_ = nullptr;
	AVCodecContext*       CodecContext_ = nullptr;
	AVFrame*              Frame_ = nullptr;
};	

DMSSimVideoImageEncoderImpl::~DMSSimVideoImageEncoderImpl() {
	av_frame_free(&Frame_);
	avcodec_free_context(&CodecContext_);
	avformat_free_context(FormatContext_);
	sws_freeContext(ScaleContext_);
	Frame_ = nullptr;
	CodecContext_ = nullptr;
	Stream_ = nullptr;
	Codec_ = nullptr;
	FormatContext_ = nullptr;
	ScaleContext_ = nullptr;
}

bool DMSSimVideoImageEncoderImpl::Initialize() {
	Codec_ = avcodec_find_encoder(AV_CODEC_ID_TIFF);
	CodecContext_ = avcodec_alloc_context3(Codec_);
	if (!Codec_ || !CodecContext_) {
		DMSSimLog::Info() << "fail at avcodec_find_encoder or avcodec_alloc_context3" << FL;
		return false;
	}
	CodecContext_->codec_id = Codec_->id;
	CodecContext_->codec_type = AVMEDIA_TYPE_VIDEO;
	CodecContext_->width = DstWidth_;
	CodecContext_->height = DstHeight_;
	CodecContext_->time_base = AVRational{ 1, int(FrameRate_) };
	CodecContext_->pix_fmt = Nir_? (Depth16Bit_ ? AV_PIX_FMT_GRAY16LE : AV_PIX_FMT_GRAY8) : AV_PIX_FMT_RGBA;
	int err_code = avcodec_open2(CodecContext_, Codec_, nullptr);
	if (0 != err_code) {
		DMSSimLog::Info() << "fail at avcodec_open2 err code: " << err_code << FL;
		return false;
	}
	Frame_ = av_frame_alloc();
	if (!Frame_) {
		DMSSimLog::Info() << "fail at av_frame_alloc" << FL;
		return false;
	}
	Frame_->format = CodecContext_->pix_fmt;
	Frame_->width = CodecContext_->width;
	Frame_->height = CodecContext_->height;
	err_code = av_frame_get_buffer(Frame_, 32);
	if (0 != err_code) {
		DMSSimLog::Info() << "fail at av_frame_get_buffer err code: " << err_code << FL;
		return false;
	}
	ScaleContext_ = sws_getContext(SrcWidth_, SrcHeight_, AV_PIX_FMT_BGRA, DstWidth_, DstHeight_, Nir_? (Depth16Bit_ ? AV_PIX_FMT_GRAY16LE : AV_PIX_FMT_GRAY8) : AV_PIX_FMT_RGBA, SWS_BILINEAR, NULL, NULL, NULL);
	if (!ScaleContext_) {
		DMSSimLog::Info() << "fail at sws_getContext" << FL;
		return false;
	}
	return true;
}

void DMSSimVideoImageEncoderImpl::AddFrame(const TArray<FColor>& PrevImage, const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth, int FrameIdx) {
	// Save the png file with zeropadded frame idx.
	std::wstringstream ss;
	ss << FileName_ << L"_" << std::setw(5) << std::setfill(L'0') << FrameIdx << L".png";
	std::wstring FileNameFull = ss.str();
	const auto FileNameFullA = WideToNarrow(FileNameFull.c_str());
	const uint8_t* const ImagePlanes = reinterpret_cast<const uint8_t*>(PrevImage.GetData());
	int inLinesize[] = { SrcWidth_ * sizeof(FColor) };
	sws_scale(ScaleContext_, &ImagePlanes, inLinesize, 0, SrcHeight_, Frame_->data, Frame_->linesize);
	if (0 != avformat_alloc_output_context2(&FormatContext_, nullptr, NULL, FileNameFullA.c_str())) { DMSSimLog::Info() << "fail at avformat_alloc_output_context2" << FL; }
	Stream_ = avformat_new_stream(FormatContext_, Codec_);
	if (!Stream_) { DMSSimLog::Info() << "fail at avformat_alloc_output_context2" << FL; }
	if (0 != avcodec_parameters_from_context(Stream_->codecpar, CodecContext_)) { DMSSimLog::Info() << "fail at avcodec_parameters_from_context" << FL; }
	if (0 != avio_open(&FormatContext_->pb, FileNameFullA.c_str(), AVIO_FLAG_WRITE)) { DMSSimLog::Info() << "fail at avio_open" << FL; }
	if (0 != avformat_write_header(FormatContext_, nullptr)) { DMSSimLog::Info() << "fail at avformat_write_header" << FL; }
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = nullptr;
	pkt.size = 0;
	if (0 != avcodec_send_frame(CodecContext_, Frame_)) { DMSSimLog::Info() << "fail at avcodec_send_frame" << FL; }
	if (0 != avcodec_receive_packet(CodecContext_, &pkt)) { DMSSimLog::Info() << "fail at avcodec_receive_packet" << FL; }
	if (0 != av_interleaved_write_frame(FormatContext_, &pkt)) { DMSSimLog::Info() << "fail at av_interleaved_write_frame" << FL; }
	if (0 != av_write_trailer(FormatContext_)) { DMSSimLog::Info() << "fail at av_write_trailer" << FL; }
	av_packet_unref(&pkt);
}

TUniquePtr<DMSSimVideoEncoder> DMSSimVideoEncoder::CreateVideoImageEncoder(const std::wstring& FileName, size_t SrcWidth, size_t SrcHeight, size_t DstWidth, size_t DstHeight, size_t FrameRate, bool Depth16Bit, bool Nir) {
	TUniquePtr<DMSSimVideoEncoder> Encoder = MakeUnique<DMSSimVideoImageEncoderImpl>(FileName, SrcWidth, SrcHeight, DstWidth, DstHeight, FrameRate, Depth16Bit, Nir);
	if (Encoder && Encoder->Initialize()) { return Encoder; }
	DMSSimLog::Error() << "CreateVideoImageEncoder fail" << FL;
	return nullptr;
}
