// RIOT PAIRS: plays each NEON RIOT patch into its UNDERTOW partner, both as
// installed Audio Units, and measures how much the result evolves - how far each
// pass of a repeating phrase differs from the pass before - against the dry
// patch and against MUTATE / Living Freeze on the same source.
#include <cstdio>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>

namespace {
constexpr double sr = 48000, bpm = 120;
constexpr int block = 256, loopBars = 4, loops = 10;
constexpr int loopLen = (int)(sr * 60 / bpm * 4 * loopBars); // 8 s

struct Head : juce::AudioPlayHead {
  int64_t pos = 0;
  juce::Optional<PositionInfo> getPosition() const override {
    PositionInfo p;
    p.setBpm(bpm);
    p.setTimeSignature(TimeSignature{4, 4});
    p.setTimeInSamples(pos);
    p.setTimeInSeconds(pos / sr);
    const double ppq = pos / sr * bpm / 60;
    p.setPpqPosition(ppq);
    p.setPpqPositionOfLastBarStart(std::floor(ppq / 4) * 4);
    p.setIsPlaying(true);
    return p;
  }
};

// phrase kinds: 0 bass riff, 1 lead line, 2 low held chord, 3 mid held chord
juce::MidiBuffer phrase(int kind) {
  juce::MidiBuffer m;
  const double beat = sr * 60 / bpm;
  auto note = [&](int n, double onBeat, double lenBeats, int vel) {
    m.addEvent(juce::MidiMessage::noteOn(1, n, (juce::uint8)vel), (int)(onBeat * beat));
    m.addEvent(juce::MidiMessage::noteOff(1, n), std::min(loopLen - 1, (int)((onBeat + lenBeats) * beat)));
  };
  if (kind == 0) {
    const int riff[]{36, 36, 48, 39, 36, 43, 46, 34, 36, 36, 48, 39, 41, 43, 46, 31};
    for (int k = 0; k < 16; ++k) note(riff[k], k * .5, k % 4 == 2 ? .6 : .3, k % 4 == 0 ? 120 : 85);
    note(36, 8, 7, 100);
  } else if (kind == 1) {
    const int line[]{60, 63, 65, 67, 70, 67};
    const double at[]{0, 1.5, 3, 4, 6, 7};
    for (int k = 0; k < 6; ++k) note(line[k], at[k], k == 5 ? 6.5 : 1.3, 100);
    note(58, 14, 1.8, 90);
  } else if (kind == 2) {
    for (int n : {36, 43, 50}) note(n, 0, 14, 100);
  } else {
    for (int n : {48, 51, 55, 58}) note(n, 0, 14, 95);
  }
  return m;
}

std::unique_ptr<juce::AudioPluginInstance> load(juce::AudioPluginFormatManager &f, const char *id) {
  juce::OwnedArray<juce::PluginDescription> d;
  for (int i = 0; i < f.getNumFormats(); ++i) f.getFormat(i)->findAllTypesForFile(d, id);
  if (d.isEmpty()) return nullptr;
  juce::String err;
  return f.createPluginInstance(*d[0], sr, block, err);
}
bool program(juce::AudioPluginInstance &p, const juce::String &name) {
  for (int i = 0; i < p.getNumPrograms(); ++i)
    if (p.getProgramName(i) == name) { p.setCurrentProgram(i); return true; }
  return false;
}

// 24 log bands, 60 Hz - 12 kHz, energy per half-second window
using Frames = std::vector<std::array<double, 24>>;
Frames bands(const juce::AudioBuffer<float> &b) {
  constexpr int order = 12, n = 1 << order;
  juce::dsp::FFT fft(order);
  juce::dsp::WindowingFunction<float> win(n, juce::dsp::WindowingFunction<float>::hann);
  std::vector<float> w(2 * n);
  const int hop = (int)(sr / 2);
  Frames out;
  for (int at = 0; at + hop <= b.getNumSamples(); at += hop) {
    std::array<double, 24> e{};
    for (int f = at; f + n <= at + hop + n / 2 && f + n <= b.getNumSamples(); f += n / 2) {
      std::fill(w.begin(), w.end(), 0.f);
      for (int i = 0; i < n; ++i) w[(size_t)i] = .5f * (b.getSample(0, f + i) + b.getSample(1, f + i));
      win.multiplyWithWindowingTable(w.data(), n);
      fft.performFrequencyOnlyForwardTransform(w.data());
      for (int k = 1; k < n / 2; ++k) {
        const double hz = k * sr / n;
        if (hz < 60 || hz >= 12000) continue;
        const int band = (int)(24 * std::log(hz / 60) / std::log(200.0));
        e[(size_t)band] += (double)w[(size_t)k] * w[(size_t)k];
      }
    }
    out.push_back(e);
  }
  return out;
}
// Mean over windows of the RMS (across audible bands) of the dB change from the
// same moment one pass earlier. Loops 3..10, so the space has filled.
double evolution(const Frames &f) {
  const int per = loopLen / (int)(sr / 2);
  double sum = 0;
  int count = 0;
  for (int w = per * 2; w < (int)f.size(); ++w) {
    double s = 0;
    int used = 0;
    double top = 0;
    for (double v : f[(size_t)w]) top = std::max(top, v);
    for (size_t k = 0; k < 24; ++k) {
      const double a = f[(size_t)w][k], b = f[(size_t)(w - per)][k];
      if (std::max(a, b) < top * 1e-5) continue; // within 50 dB of the loudest band
      const double d = 10 * std::log10((a + top * 1e-6) / (b + top * 1e-6));
      s += d * d;
      ++used;
    }
    if (used > 0) { sum += std::sqrt(s / used); ++count; }
  }
  return count ? sum / count : 0;
}
double rms(const juce::AudioBuffer<float> &b, int from) {
  double s = 0;
  for (int c = 0; c < 2; ++c) for (int i = from; i < b.getNumSamples(); ++i) s += (double)b.getSample(c, i) * b.getSample(c, i);
  return std::sqrt(s / (2.0 * (b.getNumSamples() - from)));
}
double db(double v) { return 20 * std::log10(std::max(1e-9, v)); }

// ---- --drums: ROB KIT plays a 4-bar groove into each PERCUSSION preset.
// Punch: the first 30 ms of every kick and snare against the dry kit. Space:
// the preset's space alone (MIX 100 %) against the dry kit. Level and peak as above.
struct Hit { int at; bool snare; };
juce::MidiBuffer groove(std::vector<Hit> &hits) {
  juce::MidiBuffer m;
  const double beat = sr * 60 / bpm;
  auto hit = [&](int note, double b, int vel, bool punch) {
    const int at = (int)(b * beat);
    m.addEvent(juce::MidiMessage::noteOn(10, note, (juce::uint8)vel), at);
    m.addEvent(juce::MidiMessage::noteOff(10, note), std::min(loopLen - 1, at + (int)(beat * .4)));
    if (punch) hits.push_back({at, note == 38});
  };
  for (int bar = 0; bar < 4; ++bar) {
    const double o = bar * 4;
    hit(36, o, 115, true);
    hit(36, o + 2.5, 95, true);
    if (bar != 3) hit(36, o + 3.0, 105, true);
    hit(38, o + 1, 110, true);
    hit(38, o + 3, 112, true);
    for (int e = 0; e < 8; ++e)
      if (!(bar == 3 && e >= 6)) hit(e == 7 ? 46 : 42, o + e * .5, e % 2 ? 70 : 90, false);
  }
  hit(49, 0, 110, false);
  for (int k = 0; k < 4; ++k) hit(k < 2 ? 48 : 45, 15 + k * .25, 100, false); // tom fill into the loop
  return m;
}
int drumsTest(juce::AudioPluginFormatManager &formats, const juce::File &outDir) {
  outDir.createDirectory();
  const char *presets[]{"Gated Room", "Tight Plate", "Big Kit Hall", "Dub Snare", "Reverse Swell", "Tom Cavern",
                        "Shimmer Cymbals", "Stutter Grid", "Ping Pong Hats", "Trash Room", "Drum Drone", "Breathing Room"};
  std::vector<Hit> hits;
  const auto midi = groove(hits);
  auto render = [&](const juce::String &fxName, int &latency, bool spaceAlone = false) {
    auto kit = load(formats, "AudioUnit:Synths/aumu,Rkit,Rcwl");
    auto fx = load(formats, "AudioUnit:Effects/aufx,Utow,Rcwl");
    juce::AudioBuffer<float> out;
    if (!kit || !fx) return out;
    Head head;
    kit->setPlayHead(&head);
    fx->setPlayHead(&head);
    if (fxName.isNotEmpty() && !program(*fx, fxName)) return out;
    if (spaceAlone)
      for (auto *v : fx->getParameters())
        if (v->getName(64) == "Mix") v->setValueNotifyingHost(1.f);
    kit->setPlayConfigDetails(0, 2, sr, block);
    fx->setPlayConfigDetails(2, 2, sr, block);
    kit->prepareToPlay(sr, block);
    fx->prepareToPlay(sr, block);
    latency = kit->getLatencySamples();
    const int passes = 6;
    out.setSize(2, loopLen * passes);
    juce::AudioBuffer<float> b(2, block);
    for (int at = 0; at < out.getNumSamples(); at += block) {
      head.pos = at;
      const int inLoop = at % loopLen;
      juce::MidiBuffer mb;
      for (const auto m : midi)
        if (m.samplePosition >= inLoop && m.samplePosition < inLoop + block)
          mb.addEvent(m.getMessage(), m.samplePosition - inLoop);
      b.clear();
      kit->processBlock(b, mb);
      if (fxName.isNotEmpty()) { juce::MidiBuffer none; fx->processBlock(b, none); }
      for (int ch = 0; ch < 2; ++ch) out.copyFrom(ch, at, b, ch, 0, block);
    }
    return out;
  };
  int lat = 0;
  const auto dry = render({}, lat);
  if (dry.getNumSamples() == 0) { std::puts("FAIL: ROB KIT or UNDERTOW is not installed"); return 1; }
  auto peakIn = [](const juce::AudioBuffer<float> &b, int from, int len) {
    float m = 0;
    for (int ch = 0; ch < 2; ++ch)
      for (int i = from; i < std::min(b.getNumSamples(), from + len); ++i) m = std::max(m, std::abs(b.getSample(ch, i)));
    return m;
  };
  auto rmsIn = [](const juce::AudioBuffer<float> &b, int from, int len) {
    double s = 0;
    for (int ch = 0; ch < 2; ++ch)
      for (int i = from; i < from + len; ++i) s += (double)b.getSample(ch, i) * b.getSample(ch, i);
    return std::sqrt(s / (2.0 * len));
  };
  auto punchOf = [&](const juce::AudioBuffer<float> &o) {
    double sum = 0; int n = 0;
    for (int pass = 2; pass < 6; ++pass)
      for (auto h : hits) {
        const int at = pass * loopLen + h.at + lat;
        const float d = peakIn(dry, at, (int)(sr * .03));
        if (d > 1.e-3f) { sum += db(peakIn(o, at, (int)(sr * .03)) / d); ++n; }
      }
    return n ? sum / n : 0;
  };
  // above 300 Hz: with MIX at 100 % the ANCHOR still passes the dry lows, so the
  // space alone is measured where only the space is
  auto upperRms = [&](const juce::AudioBuffer<float> &b) {
    double s = 0;
    for (int ch = 0; ch < 2; ++ch) {
      juce::IIRFilter f1, f2;
      f1.setCoefficients(juce::IIRCoefficients::makeHighPass(sr, 300));
      f2.setCoefficients(juce::IIRCoefficients::makeHighPass(sr, 300));
      for (int i = 0; i < b.getNumSamples(); ++i) {
        const float y = f2.processSingleSampleRaw(f1.processSingleSampleRaw(b.getSample(ch, i)));
        if (i >= loopLen * 2) s += (double)y * y;
      }
    }
    return std::sqrt(s / (2.0 * (b.getNumSamples() - loopLen * 2)));
  };
  const double dryPeak = peakIn(dry, 0, dry.getNumSamples()), dryLevel = rms(dry, loopLen * 2), dryUpper = upperRms(dry);
  std::printf("ROB KIT alone: peak %.2f, level %.1f dB, latency %d samples\n", dryPeak, db(dryLevel), lat);
  std::printf("%-18s %9s %9s %9s %7s\n", "preset", "punch", "space", "level", "peak");
  int fails = 0, index = 0;
  for (auto *name : presets) {
    ++index;
    int l = 0;
    const auto out = render(juce::String("PERCUSSION / ") + name, l);
    if (out.getNumSamples() == 0) { std::printf("FAIL: no PERCUSSION / %s\n", name); ++fails; continue; }
    const auto wet = render(juce::String("PERCUSSION / ") + name, l, true);
    const double punch = punchOf(out), space = db(upperRms(wet) / dryUpper), lev = db(rms(out, loopLen * 2) / dryLevel);
    const float peak = out.getMagnitude(0, out.getNumSamples());
    const bool ok = std::isfinite(peak) && peak < std::max(.98, dryPeak * 1.05) && punch >= -2.5 && space >= -12 && std::abs(lev) <= 3;
    fails += !ok;
    std::printf("%-18s %+6.1f dB %+6.1f dB %+6.1f dB %7.2f %s\n", name, punch, space, lev, peak, ok ? "PASS" : "FAIL");
    juce::WavAudioFormat wav;
    const auto file = outDir.getChildFile(juce::String(index).paddedLeft('0', 2) + " " + name + ".wav");
    file.deleteFile();
    std::unique_ptr<juce::AudioFormatWriter> w(wav.createWriterFor(new juce::FileOutputStream(file), sr, 2, 24, {}, 0));
    if (w) w->writeFromAudioSampleBuffer(out, loopLen, loopLen * 3);
  }
  {
    juce::WavAudioFormat wav;
    const auto file = outDir.getChildFile("00 ROB KIT dry.wav");
    file.deleteFile();
    std::unique_ptr<juce::AudioFormatWriter> w(wav.createWriterFor(new juce::FileOutputStream(file), sr, 2, 24, {}, 0));
    if (w) w->writeFromAudioSampleBuffer(dry, loopLen, loopLen * 3);
  }
  std::puts("(punch: the first 30 ms of each kick and snare against the dry kit; space: the preset's space alone (MIX 100 %) "
            "against the dry kit, both above 300 Hz. A preset passes when the hits lose no more than 2.5 dB, the space is no quieter than -12 dB, the "
            "level stays within 3 dB of the dry kit and nothing clips)");
  std::printf("DRUMS RESULT %d failures\n", fails);
  return fails ? 1 : 0;
}
} // namespace

int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  juce::AudioPluginFormatManager formats;
  formats.addDefaultFormats();
  if (argc > 1 && juce::String(argv[1]) == "--drums")
    return drumsTest(formats, juce::File::getCurrentWorkingDirectory().getChildFile(argc > 2 ? argv[2] : "work/drums"));
  const juce::File outDir = juce::File::getCurrentWorkingDirectory().getChildFile(argc > 1 ? argv[1] : "work/pairs");
  outDir.createDirectory();
  struct Pair { const char *name; int kind; };
  const Pair pairs[]{{"Snarl Bass", 0}, {"Rust Drone", 2}, {"Low Orbit", 0}, {"Glass Moths", 3},
                     {"Abandoned Chamber", 3}, {"Black Reservoir", 2}, {"Hammer-On Lead", 1}, {"Bowed Cable Drone", 2},
                     {"Sodium Horizon", 3}, {"Swarm Choir", 3}, {"Mantle Bloom", 3}, {"Tidal Armature", 2},
                     {"Relay Cathedral", 3}, {"Nightwell Vapour", 2}, {"Faultline Halo", 3}, {"Ghosts in the Busbar", 2}};
  int fails = 0, index = 0;
  std::printf("%-22s %9s %9s %9s %8s %9s %9s %11s\n", "pair", "patch", "LF space", "space", "ratio", "in mix", "level", "peak (dry)");
  const char *only = std::getenv("PAIRS_ONLY");
  for (auto &pr : pairs) {
    ++index;
    if (only && *only && juce::String(pr.name) != only) continue;
    // 0 the patch alone, 1 Living Freeze's space alone (MIX 100 %), 2 the partner's
    // space alone, 3 the partner as voiced
    double evo[4]{}, level[4]{}, peak = 0, dryPeak = 0;
    juce::AudioBuffer<float> keep;
    for (int run = 0; run < 4; ++run) {
      auto synth = load(formats, "AudioUnit:Synths/aumu,NRio,Rcwl");
      auto fx = load(formats, "AudioUnit:Effects/aufx,Utow,Rcwl");
      if (!synth || !fx) { std::puts("FAIL: NEON RIOT or UNDERTOW is not installed"); return 1; }
      Head head;
      synth->setPlayHead(&head);
      fx->setPlayHead(&head);
      if (!program(*synth, pr.name)) { std::printf("FAIL: NEON RIOT has no '%s'\n", pr.name); ++fails; break; }
      const juce::String fxName = run == 1 ? "MUTATE / Living Freeze" : juce::String("RIOT PAIRS / ") + pr.name;
      if (run > 0 && !program(*fx, fxName)) { std::printf("FAIL: UNDERTOW has no '%s'\n", fxName.toRawUTF8()); ++fails; break; }
      if (run == 1 || run == 2)
        for (auto *v : fx->getParameters())
          if (v->getName(64) == "Mix") v->setValueNotifyingHost(1.f);
      synth->setPlayConfigDetails(0, 2, sr, block);
      fx->setPlayConfigDetails(2, 2, sr, block);
      synth->prepareToPlay(sr, block);
      fx->prepareToPlay(sr, block);
      const auto midi = phrase(pr.kind);
      juce::AudioBuffer<float> out(2, loopLen * loops), b(2, block);
      for (int at = 0; at < out.getNumSamples(); at += block) {
        head.pos = at;
        const int inLoop = at % loopLen;
        juce::MidiBuffer mb;
        for (const auto m : midi)
          if (m.samplePosition >= inLoop && m.samplePosition < inLoop + block)
            mb.addEvent(m.getMessage(), m.samplePosition - inLoop);
        b.clear();
        synth->processBlock(b, mb);
        if (run > 0) { juce::MidiBuffer none; fx->processBlock(b, none); }
        for (int c = 0; c < 2; ++c) out.copyFrom(c, at, b, c, 0, block);
      }
      synth->releaseResources();
      fx->releaseResources();
      evo[run] = evolution(bands(out));
      level[run] = rms(out, loopLen * 2);
      if (run == 0) dryPeak = out.getMagnitude(0, out.getNumSamples());
      if (run == 3) { peak = out.getMagnitude(0, out.getNumSamples()); keep = out; }
      if (std::getenv("PAIRS_DEBUG") && run >= 2) {
        int where = 0; float m = 0;
        for (int c = 0; c < 2; ++c) for (int i = 0; i < out.getNumSamples(); ++i)
          if (std::abs(out.getSample(c, i)) > m) { m = std::abs(out.getSample(c, i)); where = i; }
        int over = 0;
        for (int i = 0; i < out.getNumSamples(); ++i) over += std::abs(out.getSample(0, i)) > .9f;
        std::printf("   run %d peak %.2f at %.3f s (loop %d, %.3f s in), %d samples over 0.9\n", run, m, where / sr, where / loopLen,
                    (where % loopLen) / sr, over);
      }
    }
    const double ratio = evo[1] > 0 ? evo[2] / evo[1] : 0, lev = db(level[3] / level[0]);
    const bool ok = std::isfinite(peak) && peak < std::max(.98, dryPeak * 1.05) && ratio >= .9 && std::abs(lev) <= 2;
    fails += !ok;
    std::printf("%-22s %6.1f dB %6.1f dB %6.1f dB %8.2f %6.1f dB %+6.1f dB %5.2f (%4.2f) %s\n", pr.name, evo[0], evo[1], evo[2], ratio,
                evo[3], lev, peak, dryPeak, ok ? "PASS" : "FAIL");
    // the first six passes as an audition file
    juce::WavAudioFormat wav;
    const auto file = outDir.getChildFile(juce::String(index).paddedLeft('0', 2) + " " + pr.name + ".wav");
    file.deleteFile();
    std::unique_ptr<juce::AudioFormatWriter> w(wav.createWriterFor(new juce::FileOutputStream(file), sr, 2, 24, {}, 0));
    if (w && keep.getNumSamples() > 0) w->writeFromAudioSampleBuffer(keep, 0, loopLen * 6);
  }
  std::puts("(evolution: average change in dB, across the spectrum, from one pass of the phrase to the next; "
            "'space' is UNDERTOW alone at MIX 100 %; a pair passes when its space moves at least 0.9 x as much as Living Freeze's on the "
            "same source, it sits within 2 dB of the patch alone, and it peaks no higher than the patch or 0.98)");
  std::printf("PAIRS RESULT %d failures\n", fails);
  return fails ? 1 : 0;
}
