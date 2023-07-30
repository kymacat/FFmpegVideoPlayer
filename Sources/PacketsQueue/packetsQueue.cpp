//
//  packetsQueue.cpp
//  VideoPlayer
//
//  Created by Vladislav Yandola on 30.07.2023.
//

#include "packetsQueue.hpp"

void PacketQueue::push(AVPacket *pkt) {
  std::unique_lock<std::mutex> locker(mLock);
  packetsQueue.push(pkt);
  mQueueCheck.notify_one();
}

AVPacket* PacketQueue::pop() {
  std::unique_lock<std::mutex> locker(mLock);
  while (packetsQueue.empty()) {
    mQueueCheck.wait(locker);
  }

  AVPacket *first = packetsQueue.front();
  packetsQueue.pop();
  return first;
}
