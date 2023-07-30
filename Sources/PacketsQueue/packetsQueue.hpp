//
//  packetsQueue.hpp
//  VideoPlayer
//
//  Created by Vladislav Yandola on 30.07.2023.
//

#ifndef packetsQueue_hpp
#define packetsQueue_hpp

#include <mutex>
#include <queue>
#include <condition_variable>

extern "C"  {
  #include <libavcodec/avcodec.h>
}

class PacketQueue {
  std::mutex mLock {};
  std::condition_variable mQueueCheck {};
  std::queue<AVPacket*> packetsQueue {};

public:
  void push(AVPacket *pkt);
  AVPacket* pop();
};

#endif /* packetsQueue_hpp */
