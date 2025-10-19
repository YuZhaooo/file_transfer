#include "DMSSimVideoEncoder.h"
#include "DMSSimUtils.h"
#include "DMSSimConfig.h"
#include "DMSSimLog.h"

extern "C" {
#include <libswscale/swscale.h>
#include <libavcodec/avcodec.h>
#include <libavutil/mathematics.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}

namespace {
class DMSSimVideoEncoderImpl: public DMSSimVideoEncoder{
public:
	DMSSimVideoEncoderImpl(const std::wstring& FileName, size_t SrcWidth, size_t SrcHeight, size_t DstWidth, size_t DstHeight, size_t FrameRate, bool Depth16Bit, bool Nir)
		: DMSSimVideoEncoder(FileName, SrcWidth, SrcHeight, DstWidth, DstHeight, FrameRate, Depth16Bit, Nir) {};
	virtual ~DMSSimVideoEncoderImpl();
	bool Initialize() override;
	void AddFrame(const TArray<FColor>& PrevImage, const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth, int FrameIdx) override;

private:
	void Close();

	SwsContext*           ScaleContext_ = nullptr;
	const AVOutputFormat* Format_ = nullptr;
	AVFormatContext*      FormatContext_ = nullptr;
	AVStream*             Stream_ = nullptr;
	const AVCodec*        Codec_ = nullptr;
	AVCodecContext*       CodecContext_ = nullptr;
	AVFrame*	          Frame_ = nullptr;
};

DMSSimVideoEncoderImpl::~DMSSimVideoEncoderImpl() { Close(); }

bool DMSSimVideoEncoderImpl::Initialize() {
	DMSSimLog::Info() << "Initialize DMSSimVideoEncoderImpl" << FL;
	int err_code = 0;
	char error_buffer[256] = {};
	if (FileName_.empty()) {
		DMSSimLog::Error() << "Initialize DMSSimVideoEncoderImpl: Filename empty" << FL;
		return false;
	}
	const std::wstring FileNameFull = FileName_ + L".avi";
	const auto FileNameFullA = WideToNarrow(FileNameFull.c_str());
	const size_t BitRate = 0.2 * FrameRate_ * DstWidth_ * DstHeight_;
	ScaleContext_ = sws_getContext(SrcWidth_, SrcHeight_, AV_PIX_FMT_BGRA, DstWidth_, DstHeight_, AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL, NULL, NULL);
	Format_ = av_guess_format(NULL, FileNameFullA.c_str(), NULL);
	err_code = avformat_alloc_output_context2(&FormatContext_, Format_, NULL, FileNameFullA.c_str());
	if (err_code < 0) {
		DMSSimLog::Error() << "Initialize DMSSimVideoEncoderImpl: avformat_alloc_output_context2 failed." << FL;
		av_strerror(err_code, error_buffer, sizeof(error_buffer) - 1);
		return false;
	}
	
	Codec_ = avcodec_find_encoder(AV_CODEC_ID_H264);
	Stream_ = avformat_new_stream(FormatContext_, Codec_);

	AVDictionary *dict = nullptr;
	av_dict_set(&dict, "preset", "slow", 0);
	av_dict_set_int(&dict, "crf", 10, 0);

	CodecContext_ = avcodec_alloc_context3(Codec_);
	CodecContext_->bit_rate = BitRate;
	CodecContext_->width = DstWidth_;
	CodecContext_->height = DstHeight_;
	CodecContext_->time_base = AVRational{ 1, int(FrameRate_) };
	CodecContext_->framerate = AVRational{ int(FrameRate_), 1 };
	CodecContext_->gop_size = 10;
	CodecContext_->max_b_frames = 1;
	CodecContext_->pix_fmt = AV_PIX_FMT_YUV420P;

	if (FormatContext_->oformat->flags & AVFMT_GLOBALHEADER) // Some formats require a global header.
	{ CodecContext_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER; }

	Stream_->time_base = AVRational{ 1, int(FrameRate_) };
	Stream_->codecpar->codec_id = CodecContext_->codec_id;
	Stream_->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
	Stream_->codecpar->width = DstWidth_;
	Stream_->codecpar->height = DstHeight_;
	Stream_->codecpar->format = AV_PIX_FMT_YUV420P;
	Stream_->codecpar->bit_rate = BitRate;
	err_code = avcodec_open2(CodecContext_, Codec_, &dict);
	av_dict_free(&dict);
	if (err_code < 0) {
		DMSSimLog::Error() << "Initialize DMSSimVideoEncoderImpl: avcodec_open2 err code: " << err_code << FL;
		av_strerror(err_code, error_buffer, sizeof(error_buffer) - 1);
		return false;
	}

	err_code = avio_open(&FormatContext_->pb, FileNameFullA.c_str(), AVIO_FLAG_WRITE);
	if (err_code < 0) {
		DMSSimLog::Error() << "Initialize DMSSimVideoEncoderImpl: avio_open err code: " << err_code << FL;
		av_strerror(err_code, error_buffer, sizeof(error_buffer) - 1);
		return false;
	}

	err_code = avformat_write_header(FormatContext_, nullptr);
	if (err_code < 0) {
		DMSSimLog::Error() << "Initialize DMSSimVideoEncoderImpl: avformat_write_header err code: " << err_code << FL;
		av_strerror(err_code, error_buffer, sizeof(error_buffer) - 1);
		return false;
	}

	Frame_ = av_frame_alloc();
	Frame_->format = CodecContext_->pix_fmt;
	Frame_->width = DstWidth_;
	Frame_->height = DstHeight_;
	err_code = av_frame_get_buffer(Frame_, 1);
	if (err_code < 0) {
		DMSSimLog::Error() << "Initialize DMSSimVideoEncoderImpl: av_frame_get_buffer err code: " << err_code << FL;
		av_strerror(err_code, error_buffer, sizeof(error_buffer) - 1);
		return false;
	}

	avcodec_parameters_to_context(CodecContext_, Stream_->codecpar);
	return true;
}

void DMSSimVideoEncoderImpl::AddFrame(const TArray<FColor>& PrevImage, const TSharedPtr<DMSSimGroundTruthFrame>& PrevGroundTruth, const TSharedPtr<DMSSimGroundTruthFrame>& GroundTruth, int FrameIdx) {
	int res = 0;
	char error_buffer[256] = {};

	res = av_frame_make_writable(Frame_);
	if (res < 0) { return; }

	const uint8_t* const ImagePlanes = reinterpret_cast<const uint8_t*>(PrevImage.GetData());
	int inLinesize[] = { SrcWidth_ * sizeof(FColor) };
	// From BGRA to YUV
	sws_scale(ScaleContext_, &ImagePlanes, inLinesize, 0, SrcHeight_, Frame_->data, Frame_->linesize);

	Frame_->pts = FrameIdx;

	res = avcodec_send_frame(CodecContext_,  Frame_);
	if (res < 0) {
		av_strerror(res, error_buffer, sizeof(error_buffer) - 1);
		return;
	}

	AV_TIME_BASE;
	AVPacket pkt;
	av_init_packet(&pkt);
	pkt.data = NULL;
	pkt.size = 0;
	pkt.flags |= AV_PKT_FLAG_KEY;
	if (avcodec_receive_packet(CodecContext_, &pkt) == 0) {
		res = av_interleaved_write_frame(FormatContext_, &pkt);
		av_packet_unref(&pkt);
	}
	return;
}

void DMSSimVideoEncoderImpl::Close() {
	if (FormatContext_ && CodecContext_) {
		AVPacket pkt;
		av_init_packet(&pkt);
		pkt.data = NULL;
		pkt.size = 0;

		for (;;) {
			avcodec_send_frame(CodecContext_, NULL);
			if (avcodec_receive_packet(CodecContext_, &pkt) == 0) {
				av_interleaved_write_frame(FormatContext_, &pkt);
				av_packet_unref(&pkt);
			}
			else { 	break; }
		}

		av_write_trailer(FormatContext_);
		if (!(Format_->flags & AVFMT_NOFILE)) {
			int err = avio_close(FormatContext_->pb);
			if (err < 0) {
				//std::cout << "Failed to close file" << err << std::endl;
			}
		}
	}

	av_frame_free(&Frame_);
	Frame_ = nullptr;
	avcodec_free_context(&CodecContext_);
	CodecContext_ = nullptr;
	avformat_free_context(FormatContext_);
	FormatContext_ = nullptr;

	Format_ = nullptr;
	Stream_ = nullptr;
	Codec_ = nullptr;

	sws_freeContext(ScaleContext_);
	ScaleContext_ = nullptr;
}
} // anonymous namespace

TUniquePtr<DMSSimVideoEncoder> DMSSimVideoEncoder::CreateVideoEncoder(const std::wstring& FileName, size_t SrcWidth, size_t SrcHeight, size_t DstWidth, size_t DstHeight, size_t FrameRate, bool Depth16Bit, bool Nir) {
	TUniquePtr<DMSSimVideoEncoder> Encoder = MakeUnique<DMSSimVideoEncoderImpl>(FileName, SrcWidth, SrcHeight, DstWidth, DstHeight, FrameRate, Depth16Bit, Nir);
	if (Encoder && Encoder->Initialize()) { return Encoder; }
	DMSSimLog::Error() << "CreateVideoEncoder fail" << FL;
	return nullptr;
}
