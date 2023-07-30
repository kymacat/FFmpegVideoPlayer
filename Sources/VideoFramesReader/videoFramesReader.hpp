//
//  VideoFramesReader.hpp
//  VideoPlayer
//
//  Created by Vladislav Yandola on 30.06.2023.
//

#ifndef videoFramesReader_hpp
#define videoFramesReader_hpp

#include "packetsQueue.hpp"

#include <functional>

extern "C"  {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
}

class VideoFramesReader {
public:
  enum class State {
    notOpened,
    opened
  };

  struct AudioSpec {
    int freq;        /**< DSP frequency -- samples per second */
    int channels;    /**< Number of channels: 1 mono, 2 stereo */
  };
  
private:
  const char *mVideoUrl;
  State mState;
  PacketQueue mAudioPacketsQueue {};

  AVFormatContext *mContext = nullptr;
  const AVStream *mVideoStream = nullptr;
  const AVStream *mAudioStream = nullptr;
  AVCodecContext *mVideoCodecContext = nullptr;
  AVCodecContext *mAudioCodecContext = nullptr;

  AVCodecContext* makeCodecContext(const AVCodecParameters *params) const;

public:
  VideoFramesReader(const char *videoUrl);
  ~VideoFramesReader();

  void openVideo();

  void startProcessing(AVPixelFormat pixelFormat, std::function<bool(const AVFrame*)> framesProcessor);
  const AVFrame* getNextAudioFrame(AVSampleFormat sampleFormat);

  void printVideoInfo() const;
  void printAudioStreamInfo() const;

  int getWidth() const;
  int getHeight() const;
  AudioSpec getAudioInfo() const;
};

#endif /* VideoFramesReader_hpp */
