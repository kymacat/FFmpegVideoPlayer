//
//  utils.cpp
//  VideoPlayer
//
//  Created by Vladislav Yandola on 30.06.2023.
//

#include "utils.hpp"

#include <fstream>
#include <string>

extern "C"  {
  #include <libavcodec/avcodec.h>
}

void saveFrameAsPPM(const AVFrame *frame, int width, int height, const std::string& filename) {
  std::ofstream file;
  file.open(filename);

  file << "P6\n" << width << " " << height << '\n' << "255\n";

  for(int y = 0; y < height; y++) {
    uint8_t *line = frame->data[0] + y * frame->linesize[0];
    for (int x = 0; x < frame->linesize[0]; x++) {
      file << line[x];
    }
  }

  file.close();
}
