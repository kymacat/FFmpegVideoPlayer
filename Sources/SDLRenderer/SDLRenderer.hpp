//
//  SDLRenderer.hpp
//  VideoPlayer
//
//  Created by Vladislav Yandola on 30.06.2023.
//

#ifndef SDLRenderer_hpp
#define SDLRenderer_hpp

#include <string>
#include <SDL.h>

extern "C"  {
  #include <libavcodec/avcodec.h>
}

class SDLRenderer {
public:
  enum class State {
    notInitialized,
    active,
    finished
  };

private:
  std::string mWindowTitle {};
  int mHeight {};
  int mWidth {};
  State mState {};

  SDL_Window *mWindow = nullptr;
  SDL_Renderer *mRenderer = nullptr;
  SDL_Texture *mTexture = nullptr;

public:
  SDLRenderer(const std::string& windowTitle, int height, int width);

  void init();
  void renderFrame(const AVFrame *frame);

  State getState() const;
};

#endif /* SDLRenderer_hpp */
