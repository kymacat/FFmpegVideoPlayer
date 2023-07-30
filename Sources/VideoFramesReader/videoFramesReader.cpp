//
//  VideoFramesReader.cpp
//  VideoPlayer
//
//  Created by Vladislav Yandola on 30.06.2023.
//

#include "videoFramesReader.hpp"

#include <iostream>
#include <functional>
#include <stdexcept>

extern "C"  {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libavutil/imgutils.h>
  #include <libswresample/swresample.h>
}

VideoFramesReader::VideoFramesReader(const char *videoUrl) : mVideoUrl { videoUrl }, mState { State::notOpened } {

}

VideoFramesReader::~VideoFramesReader() {
  // Close the codec
  avcodec_free_context(&mVideoCodecContext);
  avcodec_free_context(&mAudioCodecContext);

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

  // Find the streams
  for (int i = 0; i < mContext->nb_streams; ++i) {
    AVStream* stream = mContext->streams[i];
    if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO && !mVideoStream) {
      mVideoStream = stream;
    }

    if (stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO && !mAudioStream) {
      mAudioStream = stream;
    }
  }

  if (!mVideoStream)
    throw std::runtime_error("Didn't find a video stream");

  mVideoCodecContext = makeCodecContext(mVideoStream->codecpar);

  if (mAudioStream)
    mAudioCodecContext = makeCodecContext(mAudioStream->codecpar);

  mState = State::opened;
}

AVCodecContext* VideoFramesReader::makeCodecContext(const AVCodecParameters *params) const {
  // Find the decoder
  const AVCodec *codec = avcodec_find_decoder(params->codec_id);

  if (!codec)
    throw std::runtime_error("Codec not found");

  // Open codec
  AVCodecContext *codecContext = avcodec_alloc_context3(codec);
  avcodec_parameters_to_context(codecContext, params);

  if (avcodec_open2(codecContext, codec, nullptr) < 0)
    throw std::runtime_error("Could not open codec");

  return codecContext;
}

void VideoFramesReader::startProcessing(AVPixelFormat pixelFormat, std::function<bool(const AVFrame*)> framesProcessor) {
  if (mState != State::opened)
    throw std::runtime_error("You need to call the openVideo function before reading frames");

  // Allocate video frames
  AVFrame *sourceFormatFrame = av_frame_alloc();
  AVFrame *frame = av_frame_alloc();

  int bytesCount = av_image_get_buffer_size(pixelFormat, mVideoCodecContext->width, mVideoCodecContext->height, 1);
  uint8_t *buffer = (uint8_t*)av_malloc(bytesCount);

  // Assign appropriate parts of buffer to image planes
  av_image_fill_arrays(frame->data, frame->linesize, buffer, pixelFormat, mVideoCodecContext->width, mVideoCodecContext->height, 1);

  SwsContext* swsContext = sws_getContext(mVideoCodecContext->width,
                                          mVideoCodecContext->height,
                                          mVideoCodecContext->pix_fmt,
                                          mVideoCodecContext->width,
                                          mVideoCodecContext->height,
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
      res = avcodec_send_packet(mVideoCodecContext, &packet);
      res = avcodec_receive_frame(mVideoCodecContext, sourceFormatFrame);
      // Did we get a video frame?
      if (res == 0) {
        // Convert the image from its native format
        sws_scale(swsContext,
                  sourceFormatFrame->data,
                  sourceFormatFrame->linesize,
                  0,
                  mVideoCodecContext->height,
                  frame->data,
                  frame->linesize);

        if (!framesProcessor(frame))
          isFinishedReadingFrames = true;
      }
    }

    if (packet.stream_index == mAudioStream->index) {
      AVPacket *pkt = av_packet_clone(&packet);
      mAudioPacketsQueue.push(pkt);
    }

    av_packet_unref(&packet);
  }

  av_free(buffer);
  av_free(sourceFormatFrame);
  av_free(frame);

  sws_freeContext(swsContext);

  av_seek_frame(mContext, mVideoStream->index, 0, 0);
  avcodec_flush_buffers(mVideoCodecContext);
}

const AVFrame* VideoFramesReader::getNextAudioFrame(AVSampleFormat sampleFormat) {
  static AVPacket *pkt;
  static AVFrame *frame = av_frame_alloc();
  static AVFrame *outputFrame = av_frame_alloc();
  static SwrContext *resampler = swr_alloc();

  swr_alloc_set_opts2(
                      &resampler,
                      &mAudioCodecContext->ch_layout,
                      sampleFormat,
                      mAudioCodecContext->sample_rate,
                      &mAudioCodecContext->ch_layout,
                      mAudioCodecContext->sample_fmt,
                      mAudioCodecContext->sample_rate,
                      0,
                      NULL
                      );

  if (swr_init(resampler) < 0)
    throw std::runtime_error("Swr context initialization error");

  pkt = mAudioPacketsQueue.pop();

  avcodec_send_packet(mAudioCodecContext, pkt);
  avcodec_receive_frame(mAudioCodecContext, frame);

  outputFrame->ch_layout = mAudioCodecContext->ch_layout;
  outputFrame->sample_rate = mAudioCodecContext->sample_rate;
  outputFrame->format = sampleFormat;

  swr_convert_frame(resampler, outputFrame, frame);
  av_packet_unref(pkt);

  return outputFrame;
}

void VideoFramesReader::printVideoInfo() const {
  if (mState != State::opened)
    throw std::runtime_error("You need to call the openVideo function before printing info");

  // Dump information about file
  av_dump_format(mContext, 0, mVideoUrl, 0);
}

void VideoFramesReader::printAudioStreamInfo() const {
  if (mState != State::opened)
    throw std::runtime_error("You need to call the openVideo function before printing info");

  if (!mAudioStream)
    throw std::runtime_error("Audio stream not exists");

  std::cout << "Audio stream info:\n";
  std::cout << "Stream:         " << mAudioStream->index << '\n';
  std::cout << "Sample format:  " << av_get_sample_fmt_name(mAudioCodecContext->sample_fmt) << '\n';
  std::cout << "Sample rate:    " << mAudioCodecContext->sample_rate << '\n';
  std::cout << "Sample size:    " << av_get_bytes_per_sample(mAudioCodecContext->sample_fmt) << '\n';
  std::cout << "Channels:       " << mAudioCodecContext->ch_layout.nb_channels << '\n';
  std::cout << "Is planar:      " << (av_sample_fmt_is_planar(mAudioCodecContext->sample_fmt) ? "yes" : "no") << '\n';
}

int VideoFramesReader::getWidth() const {
  if (mState != State::opened)
    throw std::runtime_error("You need to call the openVideo function before get width");

  return mVideoCodecContext->width;
}

int VideoFramesReader::getHeight() const {
  if (mState != State::opened)
    throw std::runtime_error("You need to call the openVideo function before get height");

  return mVideoCodecContext->height;
}

VideoFramesReader::AudioSpec VideoFramesReader::getAudioInfo() const {
  if (mState != State::opened)
    throw std::runtime_error("You need to call the openVideo function before get audio info");

  if (!mAudioStream)
    throw std::runtime_error("Audio stream not exists");

  return AudioSpec { mAudioCodecContext->sample_rate, mAudioCodecContext->ch_layout.nb_channels };
}
