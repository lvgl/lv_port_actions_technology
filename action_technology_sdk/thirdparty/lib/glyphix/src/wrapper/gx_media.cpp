#include "gx_coreapplication.h"
#include "gx_dir.h"
#include "gx_file.h"
#include "gx_uri.h"
#include "pm/pm.h"
#include "shell/shell.h"
#include "sys/printk.h"
#include <cstddef>
#include <dirent.h>
#include <fcntl.h>
#include <shell/shell.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "gx_application.h"
#include "gx_file.h"
#include "gx_media.h"
#include "gx_lockguard.h"
#include "gx_mediastream.h"

#include <app_manager.h>
#include <audio_system.h>
#include <gx_math.h>
#include <media_player.h>
#include <os_common_api.h>
#include <soc.h>
#include <srv_manager.h>
#include <stdio.h>
#include <stream.h>
#include <string.h>
#include <sys_manager.h>
#include <tts_manager.h>

#define GX_LOG_TAG   "acts.media"
#define GX_LOG_LEVEL GX_LOG_DEBUG
#include "gx_logger.h"

#ifdef CONFIG_BT_TRANSMIT
extern "C" {
	void bt_transmit_catpure_start(io_stream_t input_stream, uint16_t sample_rate, uint8_t channels);
	void bt_transmit_catpure_stop(void);
	void bt_transmit_catpure_pre_start(void);
    int bt_transmit_sync_vol_to_remote(void);
};
#endif
// using namespace gx

namespace gx {
static MediaPlayer *runningPlayer = nullptr;

class MediaMutex : public Mutex {
public:
    struct LockGuard : gx::LockGuard<MediaMutex> {
        LockGuard() : gx::LockGuard<MediaMutex>(instance()) {}
    };
    static MediaMutex &instance() {
        static MediaMutex *mutex = nullptr;
        if (!mutex)
            mutex = new MediaMutex();
        return *mutex;
    }
};

void gx_media_stop_cb(void *handle) {
    MediaMutex::LockGuard lock;
    if (runningPlayer) {
        runningPlayer->notify(MediaPlayer::OnInterrupt, 2);
        runningPlayer->stop();
    }
}

extern "C" void gx_media_stop() {
    MediaMutex::LockGuard lock;
    if (runningPlayer) {
        runningPlayer->notify(MediaPlayer::OnInterrupt, 2);
        runningPlayer->stop();
    }
}

extern "C" bool gx_media_player_is_play() {
    if (runningPlayer == nullptr)
        return false;
    return true;
}

static void _actsmusicplayer_event_callback(u32_t event, void *data, u32_t len, void *user_data);

class ActsMediaStream : public MediaStream {
public:
    ActsMediaStream()
        : MediaStream(), m_cachebuff(nullptr), cache_offset(0), cache_length(0), is_mp3(false) {}
    ~ActsMediaStream() {
        if (m_cachebuff != nullptr) {
            delete[] m_cachebuff;
            m_cachebuff = nullptr;
            LogInfo() << "~ActsMediaStream finished" << m_cachebuff;
        }
    }
    uint8_t *m_cachebuff;
    uint32_t cache_offset;
    uint32_t cache_length;
    bool is_mp3;
};

static int url_stream_init(io_stream_t handle, void *param) {
    int rc = 0;
    const String *url = static_cast<const String *>(param);
    String::const_iterator index = url->find(".mp3");

    ActsMediaStream *aMediaStream = new ActsMediaStream();
    if (!aMediaStream) {
        return -ENOMEM;
    }

    aMediaStream->is_mp3 = (index != url->end());
    LogInfo() << "url_stream_init is_mp3 " << aMediaStream->is_mp3 << index;
    rc = aMediaStream->open(*url);
    if (rc < 0) {
        delete aMediaStream;
        return -EPIPE;
    }
    handle->total_size = aMediaStream->size();

    handle->wofs = handle->total_size;
    handle->data = aMediaStream;

    return 0;
}

static int url_stream_open(io_stream_t handle, stream_mode mode) {
    handle->mode = mode;

    return 0;
}

static int url_stream_read(io_stream_t handle, unsigned char *buf, int len) {
    int ret = 0;
    int read_cnt = 0;
    int read_len = 0;

    ActsMediaStream *aMediaStream = static_cast<ActsMediaStream *>(handle->data);

    if (aMediaStream->cache_length && handle->rofs == 0 && aMediaStream->is_mp3) {
        if (len < (int)aMediaStream->cache_length) {
            read_len = len;
        } else {
            read_len = aMediaStream->cache_length;
        }
        memcpy(buf, &aMediaStream->m_cachebuff[handle->rofs], read_len);
        handle->rofs += read_len;
        len = len - read_len;
        if (len <= 0) {
            return read_len;
        }
    }

TRY_AGAIN:
    ret = aMediaStream->read(buf, len);
    read_cnt++;

    if (ret < 0 && read_cnt < 3) {
        goto TRY_AGAIN;
    }

    if (handle->rofs == 0 && aMediaStream->m_cachebuff == nullptr && ret > 0 &&
        aMediaStream->is_mp3) {
        aMediaStream->m_cachebuff = new uint8_t[ret];
        memcpy(aMediaStream->m_cachebuff, buf, ret);
        aMediaStream->cache_length = ret;
        aMediaStream->cache_offset = handle->rofs;
    }

    handle->rofs += ret;
    return read_len + ret;
}

static int url_stream_seek(io_stream_t handle, int offset, seek_dir origin) {
    int ret = 0;
    ActsMediaStream *aMediaStream = static_cast<ActsMediaStream *>(handle->data);

    if ((offset == (int)handle->rofs && origin == SEEK_DIR_BEG) && aMediaStream->is_mp3) {
        LogInfo() << "url_stream_seek skip1 " << offset << origin << ret << aMediaStream->is_mp3;
        return 0;
    }
#if 0
	if (handle->rofs == aMediaStream->cache_length && origin == SEEK_DIR_BEG && aMediaStream->is_mp3) {
                                LogInfo() << "url_stream_seek skip2 " << handle->rofs << aMediaStream->cache_length << ret << aMediaStream->is_mp3;
                                return 0;
                }
#endif
    ret = aMediaStream->seekByWhence(offset, origin);
    LogInfo() << "url_stream_seek || " << offset << origin << ret << handle->rofs;
    handle->rofs = offset;
    aMediaStream->cache_length = 0;
    return 0;
}

static int url_stream_tell(io_stream_t handle) {
    ActsMediaStream *aMediaStream = static_cast<ActsMediaStream *>(handle->data);

    int ret = aMediaStream->tell();

    LogInfo() << "url_stream_tell" << ret;
    return ret;
}

static int url_stream_get_length(io_stream_t handle) {
    ActsMediaStream *aMediaStream = static_cast<ActsMediaStream *>(handle->data);

    return aMediaStream->size();
}

static int url_stream_close(io_stream_t handle) {
    handle->wofs = 0;
    handle->rofs = 0;
    handle->total_size = 0;
    handle->state = STATE_CLOSE;
    return 0;
}

static int url_stream_destroy(io_stream_t handle) {
    ActsMediaStream *aMediaStream = static_cast<ActsMediaStream *>(handle->data);
    if (aMediaStream) {
        delete aMediaStream;
        handle->data = NULL;
    }
    return 0;
}

const stream_ops_t url_stream_ops = {
    .init = url_stream_init,
    .open = url_stream_open,
    .read = url_stream_read,
    .seek = url_stream_seek,
    .tell = url_stream_tell,
    .get_length = url_stream_get_length,
    .close = url_stream_close,
    .destroy = url_stream_destroy,

};

io_stream_t url_stream_create(const String &url) {
    void *param = const_cast<void *>(reinterpret_cast<const void *>(&url));
    LogInfo() << "url_stream_create" << url << param;
    return stream_create(&url_stream_ops, param);
}

static io_stream_t open_url_stream(stream_mode mode, const String &url) {
    io_stream_t stream = url_stream_create(url);

    if (!stream)
        return NULL;

    if (stream_open(stream, mode)) {
        stream_destroy(stream);
        return NULL;
    }

    return stream;
}
static int ActsMusicPlayerGetFormat(const String &url) {
    int format = MP3_TYPE;

    if (url.find(".wav") != url.end()) {
        format = WAV_TYPE;
    } else if (url.find(".ape") != url.end()) {
        format = APE_TYPE;
    } else if (url.find(".wma") != url.end()) {
        format = WMA_TYPE;
    } else if (url.find(".m4a") != url.end()) {
        format = M4A_TYPE;
    } else if (url.find(".m4a") != url.end() || url.find(".aac") != url.end()) {
        format = M4A_TYPE;
    }
    LogInfo() << "format" << format;
    return format;
}

// enter s3 flag when player was paused
static Atomic<bool> enter_s3{false};
static Atomic<bool> player_paused{false};

media_player_t *ActsMusicPlayerCreate(io_stream_t pInStream, int format)
{
    media_init_param_t init_param;
    media_player_t *player = NULL;

    memset(&init_param, 0, sizeof(init_param));

#ifdef CONFIG_PLAYTTS
	tts_manager_wait_finished(false);
#endif

	media_player_force_stop(false);

    init_param.type = MEDIA_SRV_TYPE_PARSER_AND_PLAYBACK;
    init_param.stream_type = AUDIO_STREAM_LOCAL_MUSIC;
    init_param.format = format;
    init_param.input_stream = pInStream;
    init_param.output_stream = NULL;
    init_param.event_notify_handle = _actsmusicplayer_event_callback;
    init_param.dsp_output = 1;

#ifdef CONFIG_BT_TRANSMIT
	bt_transmit_catpure_pre_start();
#endif

    player = media_player_open(&init_param);
    if (!player) {
        // SYS_LOG_ERR("media_player_open failed");
        return nullptr;
    }
	media_player_set_force_stop_cb(player, NULL, gx_media_stop_cb);
#ifdef CONFIG_BT_TRANSMIT
	/*makesure bt transmit STEREO mode*/
	media_player_set_effect_output_mode(player, MEDIA_EFFECT_OUTPUT_DEFAULT);
	bt_transmit_catpure_start(init_param.output_stream, init_param.sample_rate, init_param.channels);
#endif


    return player;
}

int ActsMusicPlayerStart(media_player_t *player) {
    if (!player)
        return -EFAULT;

    LogInfo() << "ActsMusicPlayerStart";

	media_player_fade_in(player, 30);

    return media_player_play(player);
}

int ActsMusicPlayerPaused(media_player_t *player) {
    if (!player)
        return -EFAULT;

    LogInfo() << "ActsMusicPlayerPaused";

    return media_player_pause(player);
}

int ActsMusicPlayerResume(media_player_t *player) {
    if (!player)
        return -EFAULT;

    LogInfo() << "ActsMusicPlayerResume";

    return media_player_resume(player);
}

int ActsMusicPlayerStop(media_player_t *player) {
    if (!player)
        return -1;
    LogInfo() << "ActsMusicPlayerStop";

	media_player_fade_out(player, 30);
	/** reserve time to fade out*/
	os_sleep(60);

#ifdef CONFIG_BT_TRANSMIT
	bt_transmit_catpure_stop();
#endif
    media_player_stop(player);
    return 0;
}

int ActsMusicPlayerDestory(media_player_t *player, io_stream_t pInStream) {
    if (!player)
        return -1;

    LogInfo() << "acts_PlayMusic_destory" << player;

    media_player_close(player);
    if (pInStream) {
        stream_close(pInStream);
        stream_destroy(pInStream);
    }
    return 0;
}

static void _sys_standby_entry_sleep(enum pm_state state) {
    if(player_paused)
        enter_s3 = true;
}

static void _media_pm_notifier_register() {
    static bool already_registered = false;
    static struct pm_notifier notifier = {
	.state_entry = _sys_standby_entry_sleep,
	.state_exit = nullptr,
    };
    if (!already_registered) {
        pm_notifier_register(&notifier);
        already_registered = true;
    }
}

class ACTSMediaPlayer : public MediaPlayer {
public:
    enum PlayStatus { PlayerPlay = 0, PlayerPause, PlayerStop };

    ACTSMediaPlayer() : m_playStatus(), m_music() {}

    ~ACTSMediaPlayer() override {
        if (m_music) {
            ActsMusicPlayerStop(m_music);
            ActsMusicPlayerDestory(m_music, m_inStream);
            m_music = nullptr;
            m_inStream = nullptr;
            runningPlayer = nullptr;
        }
        if (runningPlayer == this)
            runningPlayer = nullptr;
    }

    media_player_t *get_mplayer() { return m_music; }

    void player_notify_event_end() {
        LogInfo() << "OnEnded";
        notify(OnEnded, 0);
    }

    int play(const String &url) override {
        MediaMutex::LockGuard lock;
        _media_pm_notifier_register();
        player_paused = false;
        if (runningPlayer) {
            if (url != m_url || runningPlayer != this || (runningPlayer == this && m_playStatus == PlayerPlay)) {
                runningPlayer->notify(OnInterrupt, 2);
                runningPlayer->stop();
                ActsMusicPlayerDestory(
                    (static_cast<ACTSMediaPlayer *>(runningPlayer))->get_mplayer(), m_inStream);
                m_music = nullptr;
                m_inStream = nullptr;
            }
        }

        runningPlayer = this;

        if (url != m_url || m_music == nullptr) { // play a new music.
            m_url = url;
            m_inStream = open_url_stream(MODE_IN, url);
            if (!m_inStream) {
                notify(OnError, 0);
                return 15001;
            }
            m_music = ActsMusicPlayerCreate(m_inStream, ActsMusicPlayerGetFormat(url));
            ActsMusicPlayerStart(m_music);
            m_playStatus = PlayerPlay;
        } else if (m_playStatus == PlayerPause) {
            if(enter_s3) {
                // need reopen the player when player was paused and enter S3 sleep
                stop();
                // reopen the stream and seek
                {
                    m_url = url;
                    m_inStream = open_url_stream(MODE_IN, url);
                    if (!m_inStream) {
                        notify(OnError, 0);
                        return 15001;
                    }
                    runningPlayer = this;
                    m_music = ActsMusicPlayerCreate(m_inStream, ActsMusicPlayerGetFormat(url));

                    media_seek_info_t info;
                    info.whence = AP_SEEK_SET;
                    info.time_offset = media_bp_info.time_offset;
                    info.file_offset = media_bp_info.file_offset;
                    media_player_seek(m_music, &info);
                    ActsMusicPlayerStart(m_music);
                }
                m_playStatus = PlayerPlay;
                enter_s3 = false;
                player_paused = false;
            } else {
                ActsMusicPlayerResume(m_music);
                m_playStatus = PlayerPlay;
                notify(OnPlay, 0);
                return 0;
            }
        } else if (m_playStatus == PlayerStop) {
            ActsMusicPlayerStop(m_music);
            ActsMusicPlayerDestory(m_music, m_inStream);
            m_music = nullptr;
            m_url = url;
            m_inStream = open_url_stream(MODE_IN, url);
            if (!m_inStream) {
                notify(OnError, 0);
                return 15001;
            }
            m_music = ActsMusicPlayerCreate(m_inStream, ActsMusicPlayerGetFormat(url));
            ActsMusicPlayerStart(m_music);
            m_playStatus = PlayerPlay;
        }

        if (!m_music) {
            LogError() << "null player error:" << m_url;
            notify(OnError, 0);
            return 15001;
        }

        notify(OnPlay, 0);

        return 0;
    }

    void pause() override {
        MediaMutex::LockGuard lock;
        player_paused = true;
        media_player_get_breakpoint(m_music, &media_bp_info);
        if (m_playStatus != PlayerPlay) {
            LogInfo() << "pause: player status not playing " << m_playStatus;
            return;
        }
        ActsMusicPlayerPaused(m_music);
        if (m_playStatus != PlayerPause)
            notify(OnPause, 0);
        m_playStatus = PlayerPause;
    }

    void stop() override {
        MediaMutex::LockGuard lock;
        if (m_playStatus == PlayerStop) {
            LogInfo() << "stop: player status already stop " << m_playStatus;
            return;
        }
        ActsMusicPlayerStop(m_music);
        ActsMusicPlayerDestory(m_music, m_inStream);
        m_music = nullptr;
        m_inStream = nullptr;
        if (runningPlayer == this)
            runningPlayer = nullptr;
        if (m_playStatus != PlayerStop) {
            if(!enter_s3)
                notify(OnStop, 0);
        }
        m_playStatus = PlayerStop;
    }

    double position() const override {
        MediaMutex::LockGuard lock;
        media_breakpoint_info_t bp_info;
        if (!runningPlayer || !m_music) {
            LogError() << "player is null";
            return 0;
        }

        if (media_player_get_breakpoint(m_music, &bp_info) == 0) {
            return bp_info.time_offset / 1000;
        }

        return 0;
    }
    double duration() const override {
        MediaMutex::LockGuard lock;
        media_info_t info;

        if (!runningPlayer || !m_music) {
            LogError() << "player is null";
            return 15002;
        }

        if (media_player_get_mediainfo(m_music, &info) == 0) {
            return info.total_time / 1000;
        }
        return 0;
    }
    void setPosition(double position) override {
        MediaMutex::LockGuard lock;
        media_seek_info_t info = {
            .file_offset = -1,
        };
        int seek_time = (int)position;

        if (!runningPlayer || !m_music) {
            LogError() << "player is null";
            return;
        }
        info.whence = AP_SEEK_SET;
        info.time_offset = (int)seek_time * 1000;

        if (media_player_seek(m_music, &info)) {
            LogError() << "(%d)s failed" << seek_time;
            return;
        }
    }

private:
    io_stream_t m_inStream{};
    int m_playStatus;
    String m_url;
    media_player_t *m_music;
    media_breakpoint_info_t media_bp_info;
};

static void _actsmusicplayer_event_callback(u32_t event, void *data, u32_t len, void *user_data) {
    LogInfo() << "ActsMusicPlayer callback" << event;

    if (!runningPlayer)
        return;

    switch (event) {
    case PLAYBACK_EVENT_STOP_ERROR: break;
    case PLAYBACK_EVENT_STOP_COMPLETE:
        // case PARSER_EVENT_STOP_COMPLETE:

        (static_cast<ACTSMediaPlayer *>(runningPlayer))->player_notify_event_end();
        break;
    default: break;
    }
}

#define RECORDER_DEFAULT_SAMPLE  (16)
#define RECORDER_DEFAULT_FORMAT  (16)
#define RECORDER_DEFAULT_CHANNEL (1)
#define RECORDER_DEFAULT_TYPE    (MP3_TYPE)

static void recorder_listener(io_stream_t handle, uint8_t *buff, int size);

static int recorder_stream_init(io_stream_t handle, void *param) {
    handle->data = param;
    return 0;
}

static int recorder_stream_open(io_stream_t handle, stream_mode mode) {
    handle->mode = mode;
    handle->total_size = INT_MAX;
    handle->rofs = 0;
    handle->wofs = 0;
    return 0;
}

static int recorder_stream_read(io_stream_t handle, unsigned char *buf, int len) {
    handle->rofs += len;
    return 0;
}

static int recorder_stream_write(io_stream_t handle, unsigned char *buf, int len) {
    LogInfo() << "recorder write data, length: " << len;
    recorder_listener(handle, buf, len);
    handle->wofs += len;
    return len;
}

static int recorder_stream_get_length(io_stream_t handle) { return handle->wofs - handle->rofs; }

static int recorder_stream_get_space(io_stream_t handle) { return INT_MAX; }

static int recorder_stream_close(io_stream_t handle) { return 0; }

static const stream_ops_t recorder_stream_ops = {
    .init = recorder_stream_init,
    .open = recorder_stream_open,
    .read = recorder_stream_read,
    .get_length = recorder_stream_get_length,
    .get_space = recorder_stream_get_space,
    .write = recorder_stream_write,
    .close = recorder_stream_close,
};

static io_stream_t recorder_open_stream(stream_mode mode, void *parent) {
    io_stream_t stream = stream_create(&recorder_stream_ops, parent);
    if (!stream) {
        LogError() << "recorder stream create faield.";
        return nullptr;
    }
    int result = stream_open(stream, mode);
    if (result != 0) {
        stream_destroy(stream);
        LogError() << "recorder stream open faield.";
        return nullptr;
    }
    return stream;
}

static void recorder_close_stream(io_stream_t stream) {
    if (stream) {
        stream_close(stream);
        stream_destroy(stream);
    }
}

static media_type recorder_get_type(const String &path) {
    media_type type = UNSUPPORT_TYPE;
    if (path.endsWith(".mp3", String::CaseInsensitive)) {
        type = MP3_TYPE;
    } else if (path.endsWith(".opus", String::CaseInsensitive)) {
        type = OPUS_TYPE;
    } else if (path.endsWith(".pcm", String::CaseInsensitive)) {
        type = PCM_TYPE;
    } else {
        LogError() << "recorder type error, only support <mp3,opus,pcm>";
        type = RECORDER_DEFAULT_TYPE;
    }
    return type;
}

static int recorder_get_sample(int sample) {
    int result = 0;
    switch (sample) {
    case 8000:
    case 16000:
    case 48000:
    case 96000:
    case 192000:
    case 384000: result = (sample / 1000); break;
    default: result = RECORDER_DEFAULT_SAMPLE; break;
    }
    return result;
}

static int recorder_get_format(int format) {
    int result = 0;
    switch (format) {
    case 8:
    case 16:
    case 32: result = format; break;
    default: result = RECORDER_DEFAULT_FORMAT; break;
    }
    return result;
}

static int recorder_get_channel(int channel) {
    int result = 0;
    switch (channel) {
    case 1:
    case 2: result = channel; break;
    default: result = RECORDER_DEFAULT_CHANNEL; break;
    }
    return result;
}

class ACTSMediaRecorder : public MediaRecorder {
public:
    enum RecordStatus { RecordStart = 0, RecordStop, RecordRelease };
    ACTSMediaRecorder() : m_state(RecordStop), m_recorder(nullptr), m_streamer(nullptr) {}
    ~ACTSMediaRecorder() override { release(); }
    int start(const String &path, int sample, int format, int channel) override {
        if (m_state == RecordStart) {
            return ErrNone;
        }
        if (ioOpen(path, sample, format, channel) != ErrNone) {
            return ErrFileOpen;
        }
        notify(OnStart, 0);
        m_state = RecordStart;
        return ErrNone;
    }
    int stop() override {
        if (m_state == RecordStop) {
            return ErrNone;
        }
        if (ioClose() != ErrNone) {
            return ErrStop;
        }
        notify(OnStop, 0);
        m_state = RecordStop;
        return ErrNone;
    }
    int release() override {
        if (m_state == RecordRelease) {
            return ErrNone;
        }
        if (stop() != ErrNone) {
            return ErrRelease;
        }
        notify(OnRelease, 0);
        m_state = RecordRelease;
        return ErrNone;
    }
    ByteArray read() override { return ioRead(); }
    void callback(const uint8_t *buffer, int length) {
        RecordError result = ErrNone;
        if (m_state == RecordStart) {
            result = ioWrite(buffer, length);
        }
        if ((m_state == RecordStart) && (result != ErrNone)) {
            notify(OnError, ErrDataWrite);
        }
        if ((m_state == RecordStart) && (result == ErrNone)) {
            notify(OnAvailable, length);
        }
        delete[] buffer;
    }

private:
    RecordError ioOpen(const String &path, int sample, int format, int channel) {
        if (m_ioOpend == true) {
            LogError() << "recorder is already stated.";
            return ErrStart;
        }
        if (recorder_get_type(path) == UNSUPPORT_TYPE) {
            LogError() << "recorder parameter error, start failed.";
            return ErrStart;
        }
        Dir(Dir(path).directoryName()).mkpath();
        m_wFile.setUri(path);
        m_rFile.setUri(path);
        if (m_wFile.isOpened()) {
            m_wFile.close();
        }
        if (m_rFile.isOpened()) {
            m_rFile.close();
        }
        if (m_wFile.open(File::Truncate) <= 0) {
            LogError() << "recorder write file open failed.";
            return ErrFileOpen;
        }
        m_wFile.seek(0, 0);
        if (m_rFile.open(File::ReadOnly) <= 0) {
            LogWarn() << "recorder read file open failed.";
        }
        m_rFile.seek(0, 0);
        memset(&m_init_param, 0, sizeof(m_init_param));
        m_streamer = recorder_open_stream(MODE_IN_OUT, this);
        if (m_streamer == nullptr) {
            LogError() << "recorder stream open error, start failed.";
            return ErrStart;
        }
        if (runningPlayer) {
            runningPlayer->notify(ACTSMediaPlayer::OnInterrupt, 2);
            runningPlayer->stop();
        }
        m_init_param.type = MEDIA_SRV_TYPE_CAPTURE;
        m_init_param.stream_type = AUDIO_STREAM_AI;
        m_init_param.capture_format = recorder_get_type(path);
        m_init_param.capture_sample_rate_input = recorder_get_sample(sample);
        m_init_param.capture_sample_rate_output = recorder_get_sample(sample);
        m_init_param.capture_sample_bits = recorder_get_format(format);
        m_init_param.capture_channels_input = recorder_get_channel(channel);
        m_init_param.capture_channels_output = recorder_get_channel(channel);
        m_init_param.capture_bit_rate = recorder_get_sample(sample);
        m_init_param.capture_input_stream = NULL;
        m_init_param.capture_output_stream = m_streamer;
        m_recorder = media_player_open(&m_init_param);
        if (m_recorder == nullptr) {
            LogError() << "recorder media open error, start failed.";
            ioClose();
            return ErrStart;
        }
        int result = media_player_play(m_recorder);
        if (result != 0) {
            LogError() << "recorder media play error:" << result;
            ioClose();
            return ErrStart;
        }
        m_ioOpend = true;
        LogInfo() << "recorder media play success.";
        return ErrNone;
    }
    RecordError ioWrite(const uint8_t *buffer, int length) {
        int writen = 0;
        if (m_ioOpend == false) {
            LogError() << "recorder io not open, write data failed.";
            return ErrDataWrite;
        }
        if (m_wFile.isOpened()) {
            writen = m_wFile.write(buffer, length);
        }
        if (writen < length) {
            LogError() << "recorder write error, length(" << length << ")!= writen(" << writen
                       << ")";
            return ErrDataWrite;
        }
        return ErrNone;
    }
    ByteArray ioRead() {
        if (m_ioOpend == false) {
            LogWarn() << "recorder io not open, read data failed.";
            return ByteArray(0);
        }
        if (m_rFile.isOpened()) {
            return m_rFile.read();
        }
        LogWarn() << "recorder read file not open, read data failed.";
        return ByteArray(0);
    }
    RecordError ioClose() {
        if (m_recorder != nullptr) {
            media_player_stop(m_recorder);
            media_player_close(m_recorder);
            m_recorder = nullptr;
        }
        if (m_streamer != nullptr) {
            recorder_close_stream(m_streamer);
            m_streamer = nullptr;
        }
        if (m_wFile.isOpened()) {
            m_wFile.close();
        }
        if (m_rFile.isOpened()) {
            m_rFile.close();
        }
        m_ioOpend = false;
        return ErrNone;
    }

private:
    RecordStatus m_state{};
    File m_wFile{};
    File m_rFile{};
    media_player_t *m_recorder{};
    io_stream_t m_streamer{};
    media_init_param_t m_init_param{};
    bool m_ioOpend = false;
};

static void recorder_listener(io_stream_t handle, uint8_t *buff, int size) {
    ACTSMediaRecorder *recorder = (ACTSMediaRecorder *)(handle->data);
    if ((recorder == nullptr) || (buff == nullptr) || (size <= 0) || (App()->isQuited())) {
        LogError() << "recorder listener callback error, recorder is null.";
        return;
    }
    uint8_t *data = new uint8_t[size];
    if (data == nullptr) {
        LogError() << "recorder listener callback error, malloc(" << size << ") failed.";
        return;
    }
    memcpy(data, buff, size);
    App()->postTask([=] {
        if (recorder) {
            recorder->callback(data, size);
        }
    });
}

class ACTSMediaWrapper : public MediaWrapper {
public:
    ACTSMediaWrapper() : MediaWrapper() {}
    virtual ~ACTSMediaWrapper() {}
    virtual MediaPlayer *createPlayer() { return new ACTSMediaPlayer; }
    virtual MediaRecorder *createRecorder() { return new ACTSMediaRecorder; }
    virtual void setVolume(float volume) {
        audio_system_set_stream_volume(AUDIO_STREAM_LOCAL_MUSIC, (int)(volume * 16));
	#ifdef CONFIG_BT_TRANSMIT
		bt_transmit_sync_vol_to_remote();
	#endif
    }
    virtual float volume() {
        return audio_system_get_current_volume(AUDIO_STREAM_LOCAL_MUSIC) * 1.0 / 15;
    }
};

MediaWrapper *MediaWrapper::create() { return new ACTSMediaWrapper(); }

} // namespace gx
