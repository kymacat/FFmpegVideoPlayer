//
//  utils.hpp
//  VideoPlayer
//
//  Created by Vladislav Yandola on 30.06.2023.
//

#ifndef utils_hpp
#define utils_hpp

#include <string>

extern "C"  {
  #include <libavcodec/avcodec.h>
}

void saveFrameAsPPM(const AVFrame *frame, int width, int height, const std::string& filename);

#endif /* utils_hpp */
