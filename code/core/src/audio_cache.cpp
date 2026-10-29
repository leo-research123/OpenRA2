#include "audio_session.hpp"
namespace game {
void AudioSession::discard_unused_samples() noexcept {
    // Stop commands retain their own PCM references until the callback consumes
    // them. Retired storage must not keep the logical sample cache pinned.
    for (auto it = samples.begin(); it != samples.end();) {
        bool used = false;
        for (const auto& entry : events)
            if (entry.alive && entry.type) {
                for (int i = 0; i < entry.SampleCount; ++i)
                    if (entry.type->SampleIndex[entry.sample_indices[i]] == it->first) {
                        used = true;
                        break;
                    }
                if (used) break;
            }
        if (!used) {
            sample_bytes -= it->second->pcm.size() * sizeof(AudioPCMFrame);
            it = samples.erase(it);
        } else
            ++it;
    }
}
std::shared_ptr<AudioSource> AudioSession::sample(int index_value) {
    if (const auto it = samples.find(index_value); it != samples.end()) return it->second;
    if (!index || !index->OpenSample(index_value)) {
        ++missing;
        return {};
    }
    struct SampleRead {
        AudioIDXData& index;
        ~SampleRead() { index.ClearCurrentSample(); }
    } reading{*index};
    AudioSampleData info;
    if (!index->GetSampleInformation(index_value, &info)) {
        ++missing;
        return {};
    }
    AudioDecoder decoder;
    if (!decoder.open(*index->CurrentSampleFile, info, index->CurrentSampleSize)) {
        ++failures;
        return {};
    }
    auto result = std::make_shared<AudioSource>();
    result->rate = info.SampleRate;
    AudioPCMFrame buffer[2048];
    const std::size_t budget = sample_budget;
    while (!decoder.finished()) {
        unsigned frames = 0;
        if (!decoder.read(buffer, 2048, frames) || !frames) {
            ++failures;
            return {};
        }
        const auto required = (result->pcm.size() + frames) * sizeof(AudioPCMFrame);
        if (sample_bytes + required > budget) {
            collect();
            // Logical cache ownership is independent of the references that
            // keep stopped PCM alive until its consumer fence is crossed.
            discard_unused_samples();
        }
        if (sample_bytes + required > budget) {
            sample_pressure = true;
            fail("Audio sample cache budget exceeded");
            return {};
        }
        result->pcm.insert(result->pcm.end(), buffer, buffer + frames);
    }
    samples.emplace(index_value, result);
    sample_bytes += result->pcm.size() * sizeof(AudioPCMFrame);
    return result;
}
}
