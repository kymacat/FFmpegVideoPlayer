//
//  SDLRenderer.cpp
//  VideoPlayer
//
//  Created by Vladislav Yandola on 30.06.2023.
//

#include "SDLRenderer.hpp"

#include <string>
#include <SDL.h>

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

void SDLRenderer::openAudio(
                            const int freq,
                            const int channels,
                            void* userdata,
                            const SDL_AudioCallback& callback
                            ) const {
  SDL_AudioSpec wantedSpec, spec;

  wantedSpec.freq = freq;
  wantedSpec.format = AUDIO_S16SYS;
  wantedSpec.channels = channels;
  wantedSpec.silence = 0;
  wantedSpec.samples = 1024;
  wantedSpec.callback = callback;
  wantedSpec.userdata = userdata;

  SDL_AudioDeviceID id = SDL_OpenAudioDevice(NULL, 0, &wantedSpec, &spec, SDL_AUDIO_ALLOW_ANY_CHANGE);
  SDL_PauseAudioDevice(id, 0);
}

void SDLRenderer::renderFrame(const uint8_t *const *data, const int *linesize) {
  if (mState != State::active)
    throw std::runtime_error("You need to call the initialize function before rendering");

  SDL_Event event;

  SDL_UpdateYUVTexture(mTexture,
                       NULL,
                       data[0],
                       linesize[0],
                       data[1],
                       linesize[1],
                       data[2],
                       linesize[2]);


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
}

SDLRenderer::State SDLRenderer::getState() const {
  return mState;
}
