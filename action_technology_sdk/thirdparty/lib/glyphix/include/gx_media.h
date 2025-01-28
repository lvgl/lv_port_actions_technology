/*
 * This file is part of Glyphix.
 * Copyright (c) 2023-2024, Glyphix Development Team.
 */
#pragma once

#include "gx_signals.h"
#include "gx_bytearray.h"

namespace gx {
class MediaPlayer {
public:
    enum Notify {
        OnPlay,
        OnPause,
        OnStop,
        OnEnded,
        OnTimeUpdate,
        OnPrevious,
        OnNext,
        OnLoadedData,
        OnInterrupt,
        OnError
    };

    enum MediaError {
        PlayError,
        PauseError,
        StopError,
    };

    virtual ~MediaPlayer() GX_DEFAULT;
    virtual int play(const String &url) = 0;
    virtual void pause() = 0;
    virtual void stop() = 0;
    virtual double duration() const = 0;
    virtual double position() const = 0;
    virtual void setPosition(double position) = 0;

    Signal<Notify, int> notify;
};

class MediaRecorder {
public:
    enum RecordNotify {
        OnStart,
        OnStop,
        OnRelease,
        OnAvailable,
        OnError
    };

    enum RecordError {
        ErrNone,
        ErrFileOpen,
        ErrFileClose,
        ErrDataRead,
        ErrDataWrite,
        ErrStart,
        ErrStop,
        ErrRelease,
    };

    virtual ~MediaRecorder() GX_DEFAULT;
    virtual int start(const String &path, int sample, int format, int channel) = 0;
    virtual int stop() = 0;
    virtual int release() = 0;
    virtual ByteArray read() = 0;
    Signal<RecordNotify, int> notify;
};

class MediaWrapper {
public:
    virtual ~MediaWrapper() GX_DEFAULT;
    virtual MediaPlayer *createPlayer() = 0;
    virtual MediaRecorder *createRecorder() = 0;
    virtual void setVolume(float volume) = 0;
    virtual float volume() = 0;
    static MediaWrapper *create();
};
} // namespace gx
