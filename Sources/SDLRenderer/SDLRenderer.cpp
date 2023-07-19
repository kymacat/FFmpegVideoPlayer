//
//  SDLRenderer.cpp
//  VideoPlayer
//
//  Created by Vladislav Yandola on 30.06.2023.
//

#include "SDLRenderer.hpp"

#include <string>
#include <SDL.h>

extern "C"  {
  #include <libavcodec/avcodec.h>
}

SDLRenderer::SDLRenderer(const std::string& windowTitle, int height, int width)
: mWindowTitle { windowTitle }, mHeight { height }, mWidth { width }, mState { State::notInitialized } {
}

void SDLRenderer::init() {
  if (mState != State::notInitialized)
    throw std::runtime_error("The state has already been initialized");

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER))
    throw std::runtime_error(SDL_GetError());

  mWindow = SDL_CreateWindow(mWindowTitle.c_str(),
                             SDL_WINDOWPOS_UNDEFINED,
                             SDL_WINDOWPOS_UNDEFINED,
                             mWidth,
                             mHeight,
                             SDL_WINDOW_RESIZABLE);

  if (!mWindow) {
    throw std::runtime_error("SDL: could not create window");
  }

  mRenderer = SDL_CreateRenderer(mWindow, -1, 0);

  if (!mRenderer) {
    throw std::runtime_error("SDL: could not create renderer");
  }

  mTexture = SDL_CreateTexture(mRenderer,
                               SDL_PIXELFORMAT_YV12,
                               SDL_TEXTUREACCESS_STREAMING,
                               mWidth,
                               mHeight);

  if (!mTexture) {
    throw std::runtime_error("SDL: could not create texture");
  }

  mState = State::active;
}

void SDLRenderer::renderFrame(const AVFrame *frame) {
  if (mState != State::active)
    throw std::runtime_error("You need to call the initialize function before rendering");

  SDL_Event event;

  SDL_UpdateYUVTexture(mTexture,
                       NULL,
                       frame->data[0],
                       frame->linesize[0],
                       frame->data[1],
                       frame->linesize[1],
                       frame->data[2],
                       frame->linesize[2]);


  SDL_RenderClear(mRenderer);
  SDL_RenderCopy(mRenderer, mTexture, NULL, NULL);
  SDL_RenderPresent(mRenderer);

  SDL_PollEvent(&event);
  switch (event.type) {
    case SDL_QUIT:
      mState = State::finished;
      SDL_DestroyTexture(mTexture);
      SDL_DestroyRenderer(mRenderer);
      SDL_DestroyWindow(mWindow);
      SDL_Quit();
      break;
    default:
      break;
  }
  SDL_Delay(10);
}

SDLRenderer::State SDLRenderer::getState() const {
  return mState;
}
