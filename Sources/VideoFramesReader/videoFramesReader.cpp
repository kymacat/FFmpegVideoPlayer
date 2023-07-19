//
//  VideoFramesReader.cpp
//  VideoPlayer
//
//  Created by Vladislav Yandola on 30.06.2023.
//

#include "videoFramesReader.hpp"

#include <functional>
#include <stdexcept>

extern "C"  {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libavutil/imgutils.h>
}

VideoFramesReader::VideoFramesReader(const char *videoUrl) : mVideoUrl { videoUrl }, mState { State::notOpened } {

}

VideoFramesReader::~VideoFramesReader() {
  // Close the codec
  avcodec_free_context(&mCodecContext);

  // Close the video file
  avformat_close_input(&mContext);
}

void VideoFramesReader::openVideo() {
  if (mState != State::notOpened)
    throw std::runtime_error("The video has already been opened");

  // Open video file
  if (avformat_open_input(&mContext, mVideoUrl, nullptr, nullptr) != 0)
    throw std::runtime_error("Couldn't open file");

  // Retrieve stream information
  if (avformat_find_stream_info(mContext, nullptr) < 0)
    throw std::runtime_error("Couldn't find stream information");

  // Find the first video stream
  for (int i = 0; i < mContext->nb_streams; ++i) {
    AVStream* stream = mContext->streams[i];
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
      mVideoStream = stream;
      break;
    }
  }

  if (!mVideoStream)
    throw std::runtime_error("Didn't find a video stream");

  // Find the decoder for the video stream
  const AVCodec *codec = avcodec_find_decoder(mVideoStream->codecpar->codec_id);

  if (!codec)
    throw std::runtime_error("Codec not found");

  // Open codec
  mCodecContext = avcodec_alloc_context3(codec);
  avcodec_parameters_to_context(mCodecContext, mVideoStream->codecpar);

  if (avcodec_open2(mCodecContext, codec, nullptr) < 0)
    throw std::runtime_error("Could not open codec");

  mState = State::opened;
}

void VideoFramesReader::readVideoFrames(AVPixelFormat pixelFormat, std::function<bool(const AVFrame*)> framesProcessor) const {
  if (mState != State::opened)
    throw std::runtime_error("You need to call the openVideo function before reading frames");

  // Allocate video frames
  AVFrame *sourceFormatFrame = av_frame_alloc();
  AVFrame *frame = av_frame_alloc();

  int bytesCount = av_image_get_buffer_size(pixelFormat, mCodecContext->width, mCodecContext->height, 1);
  uint8_t *buffer = (uint8_t*)av_malloc(bytesCount);

  // Assign appropriate parts of buffer to image planes
  av_image_fill_arrays(frame->data, frame->linesize, buffer, pixelFormat, mCodecContext->width, mCodecContext->height, 1);

  SwsContext* swsContext = sws_getContext(mCodecContext->width,
                                          mCodecContext->height,
                                          mCodecContext->pix_fmt,
                                          mCodecContext->width,
                                          mCodecContext->height,
                                          pixelFormat,
                                          SWS_BILINEAR,
                                          nullptr,
                                          nullptr,
                                          nullptr);

  bool isFinishedReadingFrames = false;

  AVPacket packet;
  while (!isFinishedReadingFrames) {
    int error = av_read_frame(mContext, &packet);

    if (error == AVERROR_EOF) {
      isFinishedReadingFrames = true;
      continue;
    }

    if (error < 0) {
      char err[AV_ERROR_MAX_STRING_SIZE];
      av_make_error_string(err, AV_ERROR_MAX_STRING_SIZE, error);
      throw std::runtime_error(err);
    }

    if (packet.stream_index == mVideoStream->index) {
      int res;
      res = avcodec_send_packet(mCodecContext, &packet);
      res = avcodec_receive_frame(mCodecContext, sourceFormatFrame);
      // Did we get a video frame?
      if (res == 0) {
        // Convert the image from its native format
        sws_scale(swsContext,
                  sourceFormatFrame->data,
                  sourceFormatFrame->linesize,
                  0,
                  mCodecContext->height,
                  frame->data,
                  frame->linesize);

        if (!framesProcessor(frame))
          isFinishedReadingFrames = true;
      }
    }

    av_packet_unref(&packet);
  }

  av_free(buffer);
  av_free(sourceFormatFrame);
  av_free(frame);

  av_seek_frame(mContext, mVideoStream->index, 0, 0);
  avcodec_flush_buffers(mCodecContext);
}

void VideoFramesReader::printVideoInfo() const {
  if (mState != State::opened)
    throw std::runtime_error("You need to call the openVideo function before printing info");

  // Dump information about file
  av_dump_format(mContext, 0, mVideoUrl, 0);
}

int VideoFramesReader::getWidth() const {
  if (mState != State::opened)
    throw std::runtime_error("You need to call the openVideo function before get width");

  return mCodecContext->width;
}

int VideoFramesReader::getHeight() const {
  if (mState != State::opened)
    throw std::runtime_error("You need to call the openVideo function before get height");

  return mCodecContext->height;
}
