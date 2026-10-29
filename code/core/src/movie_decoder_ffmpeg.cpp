// Native device adapter. Container input stays within the original CCFile
// member; neither FFmpeg filenames nor temporary extraction bypass MIX lookup.
#include "api/movie_decoder.hpp"
#include "api/sidebar_movie_backend.hpp"
#include "movie_player.hpp"
#include "sidebar_movie.hpp"
#include "yrpp/CCFileClass.h"
#include "yrpp/Audio.h"
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
}
#include <algorithm>
#include <atomic>
#include <climits>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <memory>
#include <thread>
#include <vector>

namespace game {
namespace {
constexpr unsigned ring_frames = 131072;
struct MovieAudio {
    std::vector<float> pcm = std::vector<float>(2 * ring_frames);
    std::atomic<std::uint64_t> read{0}, written{0};
    std::atomic<bool> paused{false};
};
struct VideoFrame {
    std::vector<std::uint16_t> pixels;
    std::int64_t time = 0;
    unsigned number = 0;
};
}
struct MovieSession {
    unsigned rate = 0;
    bool audio_enabled = true;
    std::atomic<SidebarMovieHandle*> playing{nullptr};
    std::atomic<bool> enabled{true};
    unsigned long long paused_at = 0, paused_time = 0;
    unsigned long long time() const noexcept {
        const auto now = Audio::GetTime();
        return now - paused_time - (enabled.load(std::memory_order_relaxed) ? 0 : now - paused_at);
    }
    std::atomic<MovieAudio*> audio{nullptr};
    std::atomic<unsigned> readers{0};
    std::atomic<float> volume{1.0f};
    std::atomic<std::uint64_t> mixed{0};
    MovieStatistics statistics;
    char error[256]{};
    void fail(const char* text) noexcept {
        ++statistics.failures;
        std::snprintf(error, sizeof(error), "%s", text);
    }
    void detach(MovieAudio* value) noexcept {
        auto* expected = value;
        audio.compare_exchange_strong(expected, nullptr);
        // Readers pin the pointer before loading it. No device callback waits
        // or allocates; reclamation occurs only on the owner thread.
        while (readers.load(std::memory_order_seq_cst))
            std::this_thread::yield();
    }
};
struct SidebarMovieHandle {
    MovieSession& session;
    CCFileClass file;
    int length = 0;
    AVIOContext* io = nullptr;
    AVFormatContext* format = nullptr;
    AVCodecContext* video = nullptr;
    AVCodecContext* sound = nullptr;
    AVPacket* packet = nullptr;
    AVFrame* decoded = nullptr;
    SwsContext* scaler = nullptr;
    SwrContext* resampler = nullptr;
    int video_index = -1, audio_index = -1;
    AVRational fps{0, 1}, time_base{0, 1};
    int width = 0, height = 0;
    unsigned next_number = 0, subtitle_period = 0, total_frames = 0;
    bool bink = false;
    std::int64_t duration = 0;
    unsigned long long started_at = 0, paused_at = 0;
    bool started = false, eof = false, failed = false, paused = false, has_future = false;
    VideoFrame current, future;
    MovieAudio audio;
    std::vector<float> conversion;
    std::uint64_t decoded_audio = 0, discard_audio = 0;
    explicit SidebarMovieHandle(MovieSession& owner, const char* name) : session(owner), file(name) {}
    ~SidebarMovieHandle() {
        auto* expected = this;
        session.playing.compare_exchange_strong(expected, nullptr);
        session.detach(&audio);
        swr_free(&resampler);
        sws_freeContext(scaler);
        av_frame_free(&decoded);
        av_packet_free(&packet);
        avcodec_free_context(&sound);
        avcodec_free_context(&video);
        avformat_close_input(&format);
        if (io) {
            av_freep(&io->buffer);
            avio_context_free(&io);
        }
    }
    static int read(void* opaque, std::uint8_t* bytes, int count) noexcept {
        auto& h = *static_cast<SidebarMovieHandle*>(opaque);
        try {
            const auto position = h.file.Seek(0, FileSeekMode::Current);
            if (position < 0 || position > h.length) return AVERROR(EIO);
            count = std::min(count, h.length - position);
            if (count <= 0) return AVERROR_EOF;
            const auto result = h.file.ReadBytes(bytes, count);
            return result > 0 ? result : AVERROR_EOF;
        } catch (...) {
            return AVERROR(EIO);
        }
    }
    static std::int64_t seek(void* opaque, std::int64_t offset, int mode) noexcept {
        auto& h = *static_cast<SidebarMovieHandle*>(opaque);
        if (mode == AVSEEK_SIZE) return h.length;
        try {
            mode &= ~AVSEEK_FORCE;
            std::int64_t base = 0;
            if (mode == SEEK_CUR)
                base = h.file.Seek(0, FileSeekMode::Current);
            else if (mode == SEEK_END)
                base = h.length;
            else if (mode != SEEK_SET)
                return AVERROR(EINVAL);
            if (base < 0 || offset < -base || offset > h.length - base) return AVERROR(EINVAL);
            const auto target = base + offset;
            return h.file.Seek(int(target), FileSeekMode::Set) == target ? target : AVERROR(EIO);
        } catch (...) {
            return AVERROR(EIO);
        }
    }
    AVCodecContext* codec(int index) {
        const auto* parameters = format->streams[index]->codecpar;
        const auto* implementation = avcodec_find_decoder(parameters->codec_id);
        if (!implementation) return nullptr;
        auto* context = avcodec_alloc_context3(implementation);
        if (!context) return nullptr;
        if (avcodec_parameters_to_context(context, parameters) < 0 ||
            avcodec_open2(context, implementation, nullptr) < 0) {
            avcodec_free_context(&context);
            return nullptr;
        }
        return context;
    }
    bool open() {
        if (!file.Open(FileAccessMode::Read) || (length = file.GetFileSize()) <= 0) return false;
        auto* buffer = static_cast<unsigned char*>(av_malloc(32768));
        if (!buffer) return false;
        io = avio_alloc_context(buffer, 32768, 0, this, read, nullptr, seek);
        if (!io) {
            av_free(buffer);
            return false;
        }
        format = avformat_alloc_context();
        if (!format) return false;
        format->pb = io;
        format->flags |= AVFMT_FLAG_CUSTOM_IO;
        if (avformat_open_input(&format, nullptr, nullptr, nullptr) < 0 ||
            avformat_find_stream_info(format, nullptr) < 0)
            return false;
        for (unsigned i = 0; i < format->nb_streams; ++i) {
            const auto type = format->streams[i]->codecpar->codec_type;
            if (type == AVMEDIA_TYPE_VIDEO && video_index < 0)
                video_index = int(i);
            else if (type == AVMEDIA_TYPE_AUDIO && audio_index < 0)
                audio_index = int(i);
        }
        if (video_index < 0 || !(video = codec(video_index))) return false;
        width = video->width;
        height = video->height;
        if (width <= 0 || height <= 0 || width > 4096 || height > 4096) return false;
        auto* stream = format->streams[video_index];
        fps = av_guess_frame_rate(format, stream, nullptr);
        time_base = stream->time_base;
        if (fps.num <= 0 || fps.den <= 0 || fps.num / fps.den <= 0) return false;
        subtitle_period = 60u / unsigned(fps.num / fps.den);
        bink = std::strcmp(format->iformat->name, "bink") == 0;
        total_frames = unsigned(std::max<std::int64_t>(0, stream->nb_frames));
        if (!total_frames && stream->duration > 0)
            total_frames = unsigned(av_rescale_q(stream->duration, time_base, av_inv_q(fps)));
        duration = av_rescale_q(stream->nb_frames, av_inv_q(fps), AVRational{1, 1000});
        if (duration <= 0 && format->duration > 0) duration = format->duration / 1000;
        if (!session.audio_enabled) audio_index = -1;
        if (audio_index >= 0) {
            if (!(sound = codec(audio_index))) return false;
            AVChannelLayout stereo = AV_CHANNEL_LAYOUT_STEREO;
            const int result =
                swr_alloc_set_opts2(&resampler, &stereo, AV_SAMPLE_FMT_FLT, int(session.rate), &sound->ch_layout,
                                    sound->sample_fmt, sound->sample_rate, 0, nullptr);
            av_channel_layout_uninit(&stereo);
            if (result < 0 || swr_init(resampler) < 0) return false;
        }
        packet = av_packet_alloc();
        decoded = av_frame_alloc();
        return packet && decoded;
    }
    bool decode_audio(AVPacket* input) {
        if (avcodec_send_packet(sound, input) < 0) return false;
        int result;
        while ((result = avcodec_receive_frame(sound, decoded)) >= 0) {
            const int count = int(av_rescale_rnd(swr_get_delay(resampler, sound->sample_rate) + decoded->nb_samples,
                                                 session.rate, sound->sample_rate, AV_ROUND_UP));
            conversion.resize(std::size_t(count) * 2);
            auto* destination = reinterpret_cast<unsigned char*>(conversion.data());
            const int frames =
                swr_convert(resampler, &destination, count, const_cast<const unsigned char**>(decoded->extended_data),
                            decoded->nb_samples);
            if (frames < 0) return false;
            const auto start = decoded_audio;
            decoded_audio += unsigned(frames);
            session.statistics.audio_frames += unsigned(frames);
            const auto skip =
                unsigned(std::min<std::uint64_t>(frames, discard_audio > start ? discard_audio - start : 0));
            const auto remaining = unsigned(frames) - skip;
            if (remaining) {
                const auto written = audio.written.load(std::memory_order_relaxed);
                const auto read = audio.read.load(std::memory_order_acquire);
                if (written + remaining - read > ring_frames) return false;
                for (unsigned i = 0; i < remaining; ++i) {
                    const auto at = (written + i) % ring_frames;
                    audio.pcm[2 * at] = conversion[2 * (i + skip)];
                    audio.pcm[2 * at + 1] = conversion[2 * (i + skip) + 1];
                }
                audio.written.store(written + remaining, std::memory_order_release);
            }
            av_frame_unref(decoded);
        }
        return result == AVERROR(EAGAIN) || result == AVERROR_EOF;
    }
    bool next_video() {
        while (!eof) {
            const int result = av_read_frame(format, packet);
            if (result == AVERROR_EOF) {
                eof = true;
                break;
            }
            if (result < 0) return false;
            bool success = true;
            if (packet->stream_index == audio_index)
                success = decode_audio(packet);
            else if (packet->stream_index == video_index) {
                success = avcodec_send_packet(video, packet) >= 0;
                if (success && avcodec_receive_frame(video, decoded) >= 0) {
                    future.pixels.resize(std::size_t(width) * height);
                    scaler =
                        sws_getCachedContext(scaler, width, height, static_cast<AVPixelFormat>(decoded->format), width,
                                             height, AV_PIX_FMT_RGB565LE, SWS_BILINEAR, nullptr, nullptr, nullptr);
                    if (!scaler)
                        success = false;
                    else {
                        unsigned char* output = reinterpret_cast<unsigned char*>(future.pixels.data());
                        int pitch = width * 2;
                        success =
                            sws_scale(scaler, decoded->data, decoded->linesize, 0, height, &output, &pitch) == height;
                        future.number = ++next_number;
                        future.time = av_rescale_q(std::int64_t(future.number - 1), av_inv_q(fps), AVRational{1, 1000});
                        has_future = success;
                        ++session.statistics.video_frames;
                    }
                    av_frame_unref(decoded);
                }
            }
            av_packet_unref(packet);
            if (!success) return false;
            if (has_future) return true;
        }
        return true;
    }
    void describe(SidebarMovieFrame& frame) noexcept {
        frame = {};
        frame.width = width;
        frame.height = height;
        frame.pitch = width;
        frame.pixels = current.pixels.empty() ? nullptr : current.pixels.data();
        frame.pixel_count = std::uint32_t(current.pixels.size());
        frame.subtitle_time = subtitle_period * current.number;
        frame.frame_period_ms = movie_frame_period(unsigned(fps.num), unsigned(fps.den));
        frame.finished = bink && current.number && total_frames &&
                         movie_frame_finished(current.number + 1, total_frames, current.number);
    }
    SidebarMovieStatus update(SidebarMovieFrame& frame) {
        if (failed) return SidebarMovieStatus::failed;
        if (paused) {
            describe(frame);
            return SidebarMovieStatus::waiting;
        }
        const auto now = session.time();
        session.playing.store(this, std::memory_order_release);
        if (!started) {
            started = true;
            started_at = now;
        }
        const auto elapsed = std::int64_t(now - started_at);
        // A stalled renderer can miss several packet intervals while the
        // audio consumer drains its buffer. Drop expired PCM on the owner
        // thread after detaching readers, rather than overflowing the ring
        // or replaying old speech behind the current video/subtitles.
        const auto target = std::uint64_t(elapsed) * session.rate / 1000;
        if (audio_index >= 0 && target > audio.read.load(std::memory_order_acquire) + session.rate / 4) {
            session.detach(&audio);
            const auto written = audio.written.load(std::memory_order_relaxed);
            audio.read.store(target, std::memory_order_release);
            if (target > written) audio.written.store(target, std::memory_order_release);
            discard_audio = target;
        }
        bool changed = false;
        for (;;) {
            if (!has_future && !eof && !next_video()) {
                failed = true;
                session.fail("Movie packet decode failed");
                return SidebarMovieStatus::failed;
            }
            if (!has_future || future.time > elapsed) break;
            current = std::move(future);
            has_future = false;
            changed = true;
        }
        describe(frame);
        if (audio_index >= 0) session.audio.store(&audio, std::memory_order_seq_cst);
        // Bink NextFrame advances FrameNum before the original completion
        // query. An updated image and completion can occur in the same call.
        if (frame.finished) {
            session.playing.store(nullptr, std::memory_order_release);
            session.detach(&audio);
            return changed ? SidebarMovieStatus::frame : SidebarMovieStatus::finished;
        }
        // Other formats use their demuxer duration; EOF is not a frame update.
        if (eof && !has_future && elapsed >= duration) {
            session.playing.store(nullptr, std::memory_order_release);
            session.detach(&audio);
            frame.finished = true;
            return changed ? SidebarMovieStatus::frame : SidebarMovieStatus::finished;
        }
        return changed ? SidebarMovieStatus::frame : SidebarMovieStatus::waiting;
    }
};
namespace {
MovieSession* installed = nullptr;
SidebarMovieHandle* create(void* opaque, const char* filename) noexcept {
    auto& session = *static_cast<MovieSession*>(opaque);
    try {
        auto handle = std::make_unique<SidebarMovieHandle>(session, filename);
        if (!handle->open()) {
            session.fail("Could not open movie through game file services");
            return nullptr;
        }
        ++session.statistics.created;
        return handle.release();
    } catch (...) {
        session.fail("Could not allocate movie decoder");
        return nullptr;
    }
}
bool describe(void*, SidebarMovieHandle* handle, SidebarMovieFrame& output) noexcept {
    if (!handle) return false;
    handle->describe(output);
    return true;
}
SidebarMovieStatus update(void*, SidebarMovieHandle* handle, SidebarMovieFrame& output) noexcept {
    try {
        return handle->update(output);
    } catch (...) {
        handle->failed = true;
        handle->session.fail("Movie update failed");
        return SidebarMovieStatus::failed;
    }
}
void pause(void*, SidebarMovieHandle* handle, bool value) noexcept {
    if (handle->paused == value) return;
    const auto now = handle->session.time();
    if (value)
        handle->paused_at = now;
    else if (handle->started)
        handle->started_at += now - handle->paused_at;
    handle->paused = value;
    handle->audio.paused.store(value, std::memory_order_release);
}
bool seek(void*, SidebarMovieHandle* handle, int frame) noexcept {
    // Radar playback never seeks; other original handle users require an
    // explicit failure rather than silently using an incompatible frame.
    return frame == int(handle->current.number);
}
void destroy(void* opaque, SidebarMovieHandle* handle) noexcept {
    if (handle) {
        ++static_cast<MovieSession*>(opaque)->statistics.destroyed;
        delete handle;
    }
}
}
bool create_movie_session(unsigned rate, MovieSession*& output) noexcept {
    if (output || installed || (rate && rate < 8000) || rate > 192000) return false;
    try {
        auto session = std::make_unique<MovieSession>();
        session->rate = rate ? rate : 48000;
        session->audio_enabled = rate != 0;
        SidebarMovieBackend backend;
        backend.context = session.get();
        backend.create = create;
        backend.describe = describe;
        backend.update = update;
        backend.pause = pause;
        backend.seek = seek;
        backend.destroy = destroy;
        if (!set_sidebar_movie_backend(backend)) return false;
        output = session.release();
        installed = output;
        return true;
    } catch (...) {
        return false;
    }
}
void destroy_movie_session(MovieSession*& session) noexcept {
    if (!session) return;
    if (installed == session) {
        reset_sidebar_movie_backend();
        installed = nullptr;
    }
    delete session;
    session = nullptr;
}
void mix_movie_session(MovieSession& session, float* output, unsigned frames) noexcept {
    if (!output) return;
    std::fill_n(output, std::size_t(frames) * 2, 0.0f);
    session.readers.fetch_add(1, std::memory_order_seq_cst);
    if (auto* audio = session.audio.load(std::memory_order_seq_cst);
        audio && session.enabled.load(std::memory_order_acquire) && !audio->paused.load(std::memory_order_acquire)) {
        const auto read = audio->read.load(std::memory_order_relaxed);
        const auto written = audio->written.load(std::memory_order_acquire);
        const unsigned count = unsigned(std::min<std::uint64_t>(frames, written - read));
        const float volume = session.volume.load(std::memory_order_relaxed);
        for (unsigned i = 0; i < count; ++i) {
            const auto at = (read + i) % ring_frames;
            output[2 * i] = audio->pcm[2 * at] * volume;
            output[2 * i + 1] = audio->pcm[2 * at + 1] * volume;
        }
        audio->read.store(read + count, std::memory_order_release);
        session.mixed.fetch_add(count, std::memory_order_relaxed);
    }
    session.readers.fetch_sub(1, std::memory_order_seq_cst);
}
bool get_movie_statistics(const MovieSession& session, MovieStatistics& output) noexcept {
    output = session.statistics;
    output.mixed_frames = session.mixed.load(std::memory_order_relaxed);
    output.active = session.playing.load(std::memory_order_acquire) != nullptr;
    return true;
}
const char* movie_session_error(const MovieSession& session) noexcept { return session.error; }
void set_movie_volume(MovieSession& session, float volume) noexcept {
    if (std::isfinite(volume)) session.volume.store(std::clamp(volume, 0.0f, 1.0f), std::memory_order_relaxed);
}
void set_movie_activity(MovieSession& session, bool playing) noexcept {
    if (session.enabled.load(std::memory_order_relaxed) == playing) return;
    const auto now = Audio::GetTime();
    if (playing)
        session.paused_time += now - session.paused_at;
    else
        session.paused_at = now;
    session.enabled.store(playing, std::memory_order_release);
}
void set_native_movie_activity(bool playing) noexcept {
    if (installed) set_movie_activity(*installed, playing);
}
}
