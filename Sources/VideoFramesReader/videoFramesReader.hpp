//
//  VideoFramesReader.hpp
//  VideoPlayer
//
//  Created by Vladislav Yandola on 30.06.2023.
//

#ifndef videoFramesReader_hpp
#define videoFramesReader_hpp

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
  
private:
  const char *mVideoUrl;
  State mState;
  AVFormatContext *mContext = nullptr;
  const AVStream *mVideoStream = nullptr;
  AVCodecContext *mCodecContext = nullptr;

public:
  VideoFramesReader(const char *videoUrl);
  ~VideoFramesReader();

  void openVideo();

  void readVideoFrames(AVPixelFormat pixelFormat, std::function<bool(const AVFrame*)> framesProcessor) const;
  void printVideoInfo() const;
  int getWidth() const;
  int getHeight() const;
};

#endif /* VideoFramesReader_hpp */
