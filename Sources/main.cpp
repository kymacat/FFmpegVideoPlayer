//
//  main.cpp
//  VideoPlayer
//
//  Created by Vladislav Yandola on 28.05.2023.
//

#include "SDLRenderer.hpp"
#include "utils.hpp"
#include "videoFramesReader.hpp"

#include <stdexcept>
#include <string>

extern "C"  {
  #include <libavcodec/avcodec.h>
}

int main(int argc, const char * argv[]) {
  if (!argv[1])
    throw std::runtime_error("You need to pass the path to the video as the first parameter");

  VideoFramesReader videoReader { argv[1] };
  videoReader.openVideo();
  videoReader.printVideoInfo();
  videoReader.printAudioStreamInfo();

  SDLRenderer renderer { "Video player", videoReader.getHeight(), videoReader.getWidth() };
  renderer.init();

  VideoFramesReader::AudioSpec audioSpec = videoReader.getAudioInfo();
  renderer.openAudio(audioSpec.freq, audioSpec.channels, &videoReader, [](void *userdata, Uint8 *stream, int len) {
    auto reader = (VideoFramesReader*)userdata;
    while(len > 0) {
      const AVFrame *frame = reader->getNextAudioFrame(AV_SAMPLE_FMT_S16);
      int audioSize = frame->linesize[0];
      memcpy(stream, frame->data[0], audioSize);
      len -= audioSize;
      stream += audioSize;
    }
  });

  videoReader.startProcessing(AV_PIX_FMT_YUV420P, [&renderer](const AVFrame* frame) {
    renderer.renderFrame(frame->data, frame->linesize);
    return renderer.getState() != SDLRenderer::State::finished;
  });

  videoReader.startProcessing(AV_PIX_FMT_RGB24, [&videoReader](const AVFrame* frame) {
    static int iteration = 0;
    std::string filename { "frame" + std::to_string(iteration) + ".ppm" };
    saveFrameAsPPM(frame, videoReader.getWidth(), videoReader.getHeight(), filename);
    return ++iteration <= 5;
  });

  return 0;
}
