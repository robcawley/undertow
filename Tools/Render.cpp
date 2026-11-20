#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <juce_audio_formats/juce_audio_formats.h>
#include <new>
#include <random>
#include <thread>
#include <set>
#include <map>
#include <tuple>
thread_local bool tracking = false;
thread_local size_t allocations = 0;
void *operator new(std::size_t n) {
  if (tracking)
    ++allocations;
  if (auto *p = std::malloc(std::max(n, size_t(1))))
    return p;
  throw std::bad_alloc();
}
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void *operator new(std::size_t n, std::align_val_t a) {
  if (tracking)
    ++allocations;
  void *p = nullptr;
  if (posix_memalign(&p, (size_t)a, std::max(n, size_t(1))) == 0)
    return p;
  throw std::bad_alloc();
}
void *operator new[](std::size_t n, std::align_val_t a) {
  return ::operator new(n, a);
}
void operator delete(void *p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void *p, std::align_val_t) noexcept { std::free(p); }
namespace {
int failures = 0;
void check(bool ok, const char *label) {
  std::printf("%s %s\n", ok ? "PASS" : "FAIL", label);
  if (!ok)
    ++failures;
}
void set(UndertowProcessor &p, int k, float v) {
  auto *param = p.state.getParameter(ut::specs[(size_t)k].id);
  param->setValueNotifyingHost(param->convertTo0to1(v));
}
void prepare(UndertowProcessor &p, double sr = 48000, int bs = 64) {
  p.setPlayConfigDetails(2, 2, sr, bs);
  p.prepareToPlay(sr, bs);
}
void process(UndertowProcessor &p, juce::AudioBuffer<float> &b) {
  juce::MidiBuffer m;
  tracking = true;
  p.processBlock(b, m);
  tracking = false;
}
struct Stats {
  double peak = 0, power = 0, dc = 0, jump = 0, jumpPower = 0;
  uint64_t n = 0, bad = 0;
  float previous[2]{};
  void add(const juce::AudioBuffer<float> &b) {
    for (int c = 0; c < 2; ++c)
      for (int i = 0; i < b.getNumSamples(); ++i) {
        float x = b.getSample(c, i);
        if (!std::isfinite(x)) {
          ++bad;
          continue;
        }
        double d = x - previous[c];
        previous[c] = x;
        peak = std::max(peak, (double)std::abs(x));
        power += (double)x * x;
        dc += x;
        jump = std::max(jump, std::abs(d));
        jumpPower += d * d;
        ++n;
      }
  }
  double rms() const { return std::sqrt(power / std::max(uint64_t(1), n)); }
  bool safe() const { return !bad && peak <= 1; }
  void print(const char *label) const {
    std::printf("%s peak %.9g rms %.9g dc %.9g maxjump %.9g jump/typical %.4g "
                "nonfinite %llu\n",
                label, peak, rms(), dc / std::max(uint64_t(1), n), jump,
                jump / std::max(1.e-15, std::sqrt(jumpPower /
                                                  std::max(uint64_t(1), n))),
                (unsigned long long)bad);
  }
};
void input(juce::AudioBuffer<float> &b, int64_t start, double sr, int kind,
           bool mute = false) {
  for (int i = 0; i < b.getNumSamples(); ++i) {
    double t = (start + i) / sr;
    float x = 0;
    if (!mute) {
      if (kind < 2)
        x = .25f * (float)std::sin(juce::MathConstants<double>::twoPi *
                                   (kind == 0 ? 40 : 80) * t);
      else if (kind == 2) {
        const int notes[]{36, 36, 43, 39, 48, 34, 36, 46};
        const double f =
            juce::MidiMessage::getMidiNoteInHertz(notes[((int)(t * 4)) % 8]);
        x = .15f * (float)(2 * (t * f - std::floor(t * f)) - 1) *
            ((int)(t * 16) % 4 < 3 ? 1.f : 0.f);
      } else if (kind == 3) {
        uint32_t h = (uint32_t)(start + i) * 747796405u + 2891336453u;
        h = ((h >> ((h >> 28) + 4)) ^ h) * 277803737u;
        h = (h >> 22) ^ h;
        x = ((float)(h & 65535) / 32768.f - 1) * .2f *
            (std::fmod(t, .5) < .05 ? 1.f : 0.f);
      } else if (kind == 4)
        x = (start + i) % (int)(sr * .5) == 0 ? .5f : 0;
    }
    for (int c = 0; c < 2; ++c)
      b.setSample(c, i, x);
  }
}
void activatePulse(UndertowProcessor &, int bands);
void nullTest() {
  for (int kind = 0; kind < 2; ++kind)
    for (int channels : {1, 2}) {
      UndertowProcessor p;
      if (kind == 0) {
        set(p, ut::drift_on, 0);
        set(p, ut::echo_on, 0);
        set(p, ut::void_on, 0);
      } else {
        set(p, ut::mix, 0);
        set(p, ut::anchor_on, 0);
      }
      p.setPlayConfigDetails(channels, 2, 48000, 64);
      p.prepareToPlay(48000, 64);
      juce::AudioBuffer<float> b(2, 64), original(2, 64);
      bool same = true;
      for (int k = 0; k < 100; ++k) {
        input(b, k * 64, 48000, 3);
        original.makeCopyOf(b);
        process(p, b);
        for (int c = 0; c < 2; ++c)
          same &= std::memcmp(b.getReadPointer(c), original.getReadPointer(c),
                              64 * sizeof(float)) == 0;
      }
      check(same, "dry path bit-identical, stereo/mono input");
      check(p.getLatencySamples() == 0, "zero latency");
    }
  for (bool zeroMix : {false, true}) {
    UndertowProcessor p;
    prepare(p);
    juce::AudioBuffer<float> b(2, 64), original(2, 64);
    for (int k = 0; k < 100; ++k) {
      input(b, k * 64, 48000, 2);
      process(p, b);
    }
    set(p, ut::in_gain, 12);
    set(p, ut::out_gain, 12);
    if (zeroMix) {
      set(p, ut::anchor_on, 0);
      set(p, ut::mix, 0);
    } else {
      set(p, ut::drift_on, 0);
      set(p, ut::echo_on, 0);
      set(p, ut::void_on, 0);
    }
    bool exact = true;
    for (int k = 0; k < 100; ++k) {
      input(b, k * 64, 48000, 2);
      original.makeCopyOf(b);
      process(p, b);
      if (k >= 20)
        for (int c = 0; c < 2; ++c)
          exact &= std::memcmp(b.getReadPointer(c), original.getReadPointer(c),
                               64 * sizeof(float)) == 0;
    }
    check(exact, "active-to-bypass settles exactly, gain controls excluded");
  }
}
void anchorTest() {
  UndertowProcessor p;
  set(p, ut::mix, 100);
  set(p, ut::drift_depth, 100);
  set(p, ut::echo_feedback, 98);
  set(p, ut::echo_level, 100);
  set(p, ut::void_level, 100);
  set(p, ut::void_decay, 100);
  set(p, ut::anchor_freq, 120);
  prepare(p);
  juce::AudioBuffer<float> b(2, 64);
  double minR = 1, maxR = 0, power = 0;
  int blocks = 0;
  Stats st;
  activatePulse(p, 6);
  set(p, ut::pulse_position, 2);
  p.prepareToPlay(48000, 64);
  const int window = 4800;
  double windowPower = 0;
  int samples = 0;
  for (int64_t n = 0; n < 48000 * 20; n += 64) {
    input(b, n, 48000, 0);
    process(p, b);
    if (n > 48000 * 10) {
      st.add(b);
      for (int i = 0; i < 64; ++i) {
        windowPower += std::pow(b.getSample(0, i), 2);
        if (++samples == window) {
          double r = std::sqrt(windowPower / window);
          minR = std::min(minR, r);
          maxR = std::max(maxR, r);
          power += r * r;
          ++blocks;
          windowPower = 0;
          samples = 0;
        }
      }
    }
  }
  const double gain =
      20 * std::log10(std::sqrt(power / blocks) / (.25 / std::sqrt(2.)));
  const double mod = 20 * std::log10(maxR / minR);
  std::printf("ANCHOR 40 Hz gain %.6f dB, 100ms-window modulation %.6f dB\n",
              gain, mod);
  check(std::abs(gain) <= .2 && mod < .2 && st.safe(),
        "40 Hz bass remains stable");
  set(p, ut::pulse_sub, 1);
  p.prepareToPlay(48000, 64);
  double onMin = 1, onMax = 0, pow = 0;
  int wn = 0;
  for (int64_t pos = 0; pos < 48000 * 10; pos += 64) {
    input(b, pos, 48000, 0);
    process(p, b);
    if (pos > 48000 * 2)
      for (int n = 0; n < 64; ++n) {
        pow += b.getSample(0, n) * b.getSample(0, n);
        if (++wn == 4800) {
          const double r = std::sqrt(pow / wn);
          onMin = std::min(onMin, r);
          onMax = std::max(onMax, r);
          pow = 0;
          wn = 0;
        }
      }
  }
  const double modulation = 20 * std::log10(onMax / onMin);
  std::printf("ANCHOR pulse_sub enabled modulation %.6f dB\n", modulation);
  check(modulation > 6, "pulse_sub makes the protected low band pulse");
}
void stress() {
  std::mt19937 rng(739);
  for (double sr : {44100., 192000.})
    for (float gravity : {-100.f, 100.f}) {
      UndertowProcessor p;
      set(p, ut::anchor_on, 0);
      set(p, ut::mix, 100);
      set(p, ut::echo_feedback, 98);
      set(p, ut::echo_level, 100);
      set(p, ut::echo_sync, 0);
      set(p, ut::echo_time, 10);
      set(p, ut::void_decay, 100);
      set(p, ut::void_gravity, gravity);
      set(p, ut::void_level, 100);
      set(p, ut::drift_depth, 100);
      set(p, ut::echo_drive, 0);
      set(p, ut::echo_tone, 16000);
      set(p, ut::echo_lowcut, 20);
      set(p, ut::echo_age, 100);
      set(p, ut::void_size, 100);
      set(p, ut::void_damp, 0);
      set(p, ut::void_mod, 100);
      set(p, ut::void_diffusion, 100);
      activatePulse(p, 6);
      set(p, ut::pulse_position, 2);
      prepare(p, sr, 4096);
      juce::AudioBuffer<float> b(2, 4096);
      Stats st;
      int64_t pos = 0, end = (int64_t)(sr * 300);
      while (pos < end) {
        int size = std::min((int64_t)(1 + rng() % 4096), end - pos);
        b.setSize(2, size, false, false, true);
        set(p, ut::echo_mode, (int)(pos / (sr * 100)) % 3);
        input(b, pos, sr, 3, pos > sr * 240);
        process(p, b);
        st.add(b);
        pos += size;
      }
      std::printf("STABILITY sr %.0f gravity %.0f 300 seconds blocks 1..4096\n",
                  sr, gravity);
      st.print("extreme");
      check(st.safe(), "finite and <=0 dBFS");
    }
}
void tails() {
  for (int engine : {ut::echo_on, ut::void_on, ut::drift_on}) {
    UndertowProcessor p;
    set(p, ut::drift_on, engine == ut::drift_on);
    set(p, ut::echo_on, engine != ut::void_on);
    set(p, ut::void_on, engine != ut::echo_on);
    set(p, ut::anchor_on, 0);
    set(p, ut::mix, 100);
    set(p, ut::echo_sync, 0);
    set(p, ut::echo_time, 450);
    prepare(p, 48000, 1024);
    juce::AudioBuffer<float> b(2, 1024);
    double last = 0;
    Stats final, all;
    for (int64_t pos = 0; pos < 48000 * 90; pos += 1024) {
      input(b, pos, 48000, 3, pos >= 48000);
      process(p, b);
      all.add(b);
      if (pos > 48000) {
        if (b.getMagnitude(0, b.getNumSamples()) > std::pow(10., -90. / 20))
          last = pos / 48000.;
      }
      if (pos > 48000 * 89)
        final.add(b);
    }
    std::printf("TAIL %s last -90 dBFS crossing %.6f s; bound 90 s\n",
                ut::specs[(size_t)engine].id, last);
    final.print("final second");
    check(all.safe() && final.peak < .000031623, "tail reaches silence");
  }
  UndertowProcessor p;
  set(p, ut::drift_on, 0);
  set(p, ut::echo_on, 0);
  set(p, ut::anchor_on, 0);
  set(p, ut::mix, 100);
  prepare(p, 48000, 1024);
  juce::AudioBuffer<float> b(2, 1024);
  for (int64_t pos = 0; pos < 48000 * 4; pos += 1024) {
    input(b, pos, 48000, 3);
    process(p, b);
  }
  set(p, ut::void_freeze, 1);
  double minR = 1, maxR = 0;
  Stats window, all;
  int64_t wn = 0;
  for (int64_t pos = 0; pos < 48000LL * 610; pos += 1024) {
    b.clear();
    process(p, b);
    all.add(b);
    if (pos >= 48000 * 10) {
      window.add(b);
      wn += 1024;
      if (wn >= 48000 * 10) {
        minR = std::min(minR, window.rms());
        maxR = std::max(maxR, window.rms());
        window = Stats{};
        wn = 0;
      }
    }
  }
  const double driftDb = 20 * std::log10(maxR / minR);
  std::printf(
      "FREEZE 600s ten-second-window range %.6f dB rms min %.9g max %.9g\n",
      driftDb, minR, maxR);
  check(all.safe() && driftDb <= 2 && minR > 1.e-8,
        "frozen level within +/-1 dB");
  set(p, ut::void_freeze, 0);
  Stats released;
  for (int64_t pos = 0; pos < 48000 * 90; pos += 1024) {
    b.clear();
    process(p, b);
    if (pos > 48000 * 89)
      released.add(b);
  }
  released.print("freeze released final second");
  check(released.peak < .000031623, "freeze release decays");
}
void clicks() {
  for (int k = 0; k < ut::count; ++k) {
    UndertowProcessor p;
    activatePulse(p, 6);
    set(p, ut::pulse_position, 2);
    set(p, ut::anchor_on, 0);
    set(p, ut::mix, 100);
    set(p, ut::echo_sync, 0);
    prepare(p);
    juce::AudioBuffer<float> b(2, 64);
    Stats st;
    for (int block = 0; block < 1500; ++block) {
      const float n = block < 750 ? block / 749.f : (1499 - block) / 749.f;
      auto *par = p.state.getParameter(ut::specs[(size_t)k].id);
      par->setValueNotifyingHost(n);
      input(b, block * 64, 48000, 1);
      process(p, b);
      st.add(b);
    }
    st.print(ut::specs[(size_t)k].id);
    const double ratio =
        st.jump /
        std::max(1.e-12, std::sqrt(st.jumpPower / std::max(uint64_t(1), st.n)));
    check(st.safe() && ratio < 12, "parameter sweep safety and jump ratio <12");
  }
  // MUTATE's parameters do nothing with MUTATE off, so sweep them again with it
  // running and every part already in use.
  for (int k = ut::mut_on; k <= ut::macro_mutate; ++k) {
    UndertowProcessor p;
    set(p, ut::mix, 100);
    set(p, ut::mut_on, 1);
    for (int part : {ut::harm_octave, ut::harm_fifth, ut::harm_odd, ut::harm_sub, ut::halo_climb, ut::evolve_amount})
      set(p, part, 40);
    prepare(p);
    juce::AudioBuffer<float> b(2, 64);
    Stats st;
    for (int block = 0; block < 1500; ++block) {
      const float n = block < 750 ? block / 749.f : (1499 - block) / 749.f;
      p.state.getParameter(ut::specs[(size_t)k].id)->setValueNotifyingHost(n);
      input(b, block * 64, 48000, 1);
      process(p, b);
      st.add(b);
    }
    st.print((juce::String(ut::specs[(size_t)k].id) + " (MUTATE on)").toRawUTF8());
    const double ratio = st.jump / std::max(1.e-12, std::sqrt(st.jumpPower / std::max(uint64_t(1), st.n)));
    check(st.safe() && ratio < 12, "MUTATE parameter sweep safety and jump ratio <12");
  }
}
void rt() {
  allocations = 0;
  for (double sr : {44100., 48000., 192000.})
    for (int bs : {1, 64, 4096}) {
      UndertowProcessor p;
      activatePulse(p, 6);
      set(p, ut::pulse_b2_source, 1);
      set(p, ut::bloom, 50);
      set(p, ut::macro_motion, 20);
      set(p, ut::xy_current, 3);
      auto initial = p.extraState();
      initial.motionCount = 1024;
      initial.corners[0][ut::pulse_b1_depth] = .25f;
      for (int i = 0; i < 1024; ++i)
        initial.motion[(size_t)i] = {(float)i / 256, (float)i / 1024,
                                     1 - (float)i / 1024};
      p.setExtraState(initial);
      set(p, ut::pulse_position, 2);
      prepare(p, sr, bs);
      juce::AudioBuffer<float> b(2, bs);
      for (int k = 0; k < 250; ++k) {
        input(b, (int64_t)k * bs, sr, 3);
        if (k % 13 == 0) {
          auto frame = p.extraState();
          frame.patterns[0] ^= 0xaaaa;
          p.setExtraState(frame);
          set(p, ut::anchor_freq, 40.f + (k % 200) * 1.5f);
          set(p, ut::void_freeze, (k / 13) % 2);
          set(p, ut::xy_motion_play, (k / 13) % 2);
        }
        process(p, b);
      }
    }
  std::printf(
      "Callback C++ allocations %zu (malloc/OS calls not intercepted)\n",
      allocations);
  check(allocations == 0, "callback C++ allocation probe");
}
void cpu() {
  for (bool path : {false, true}) {
    UndertowProcessor p;
    activatePulse(p, 6);
    set(p, ut::pulse_position, 2);
    p.copySceneAToB();
    auto frame = p.extraState();
    frame.corners[0][ut::void_decay] = .15f;
    frame.corners[1][ut::echo_feedback] = .1f;
    frame.corners[2][ut::drift_depth] = .1f;
    frame.motionCount = 1024;
    for (int i = 0; i < 1024; ++i) {
      const double phase = juce::MathConstants<double>::twoPi * i / 1024;
      frame.motion[(size_t)i] = {(float)i / 256,
                                 .5f + .4f * (float)std::sin(phase),
                                 .5f + .4f * (float)std::cos(phase)};
    }
    p.setExtraState(frame);
    set(p, ut::bloom, 37);
    set(p, ut::macro_depth, 10);
    set(p, ut::macro_motion, 15);
    set(p, ut::macro_glow, 5);
    set(p, ut::macro_chaos, 10);
    set(p, ut::duck_amount, 50);
    set(p, ut::xy_current, 3);
    set(p, ut::xy_motion_play, path ? 1.f : 0.f);
    prepare(p);
    juce::AudioBuffer<float> b(2, 64);
    double total = 0, worst = 0;
    for (int k = 0; k < 12000; ++k) {
      input(b, k * 64, 48000, 2);
      const auto begin = std::chrono::steady_clock::now();
      process(p, b);
      const double elapsed = std::chrono::duration<double>(
                                 std::chrono::steady_clock::now() - begin)
                                 .count();
      if (k > 1500) {
        total += elapsed;
        worst = std::max(worst, elapsed);
      }
    }
    const double avg = total / 10499 / (64. / 48000) * 100;
    std::printf("CPU %s 48k/64 average %.4f%% worst %.4f%% of real time, 10499 "
                "blocks\n",
                path ? "1024-point-path" : "Current-Wander", avg,
                worst / (64. / 48000) * 100);
    check(avg < 8, "average under 8% (all engines, six bands, forty focus "
                   "resonators, scene/XY/macros)");
  }
}
void stateTest() {
  std::mt19937 random(73981);
  for (int trial = 0; trial < 2; ++trial) {
    UndertowProcessor a, b;
    if (trial)
      a.setCurrentProgram(0);
    for (auto *p : a.getParameters())
      p->setValueNotifyingHost((float)(random() % 10001) / 10000);
    auto extra = a.extraState();
    extra.width = 1600;
    extra.height = 1000;
    extra.edited = true;
    for (int k = 0; k < ut::count; ++k) {
      if (ut::snapshotParameter(k))
        extra.sceneB[(size_t)k] = (float)(random() % 10001) / 10000;
      if (ut::specs[(size_t)k].kind == 0)
        for (int c = 0; c < 4; ++c)
          extra.corners[(size_t)c][(size_t)k] =
              (float)(random() % 20001) / 10000 - 1;
    }
    for (int band = 0; band < 6; ++band)
      extra.patterns[(size_t)band] = (uint16_t)random();
    extra.motionCount = 1024;
    for (int k = 0; k < 1024; ++k)
      extra.motion[(size_t)k] = {(float)k / 64,
                                 (float)(random() % 10001) / 10000,
                                 (float)(random() % 10001) / 10000};
    a.setExtraState(extra);
    juce::MemoryBlock saved;
    a.getStateInformation(saved);
    b.setStateInformation(saved.getData(), (int)saved.getSize());
    bool exact = true;
    for (auto &spec : ut::specs)
      exact &= a.state.getRawParameterValue(spec.id)->load() ==
               b.state.getRawParameterValue(spec.id)->load();
    const auto loaded = b.extraState();
    exact &= extra.sceneB == loaded.sceneB && extra.corners == loaded.corners &&
             extra.patterns == loaded.patterns && extra.width == loaded.width &&
             extra.height == loaded.height && extra.edited == loaded.edited &&
             extra.motionCount == loaded.motionCount;
    for (int k = 0; k < 1024; ++k) {
      const auto &x = extra.motion[(size_t)k];
      const auto &y = loaded.motion[(size_t)k];
      exact &= x.bar == y.bar && x.x == y.x && x.y == y.y;
    }
    check(exact, "all parameter values and auxiliary state round-trip exactly");
    if (trial == 0) {
      // A project saved before 0.6.0 kept RETURN in the interface settings.
      auto xml = juce::AudioProcessor::getXmlFromBinary(saved.getData(), (int)saved.getSize());
      if (auto *ret = xml->getChildByAttribute("id", "xy_return")) xml->removeChildElement(ret, true);
      if (auto *ex = xml->getChildByName("EXTRAS")) ex->setAttribute("return", 1);
      juce::MemoryBlock old;
      juce::AudioProcessor::copyXmlToBinary(*xml, old);
      UndertowProcessor c;
      c.setStateInformation(old.getData(), (int)old.getSize());
      check(c.parameter(ut::xy_return).getValue() > .5f, "an older project's RETURN setting becomes the RETURN parameter");
    }
    prepare(a);
    prepare(b);
    juce::AudioBuffer<float> x(2, 64), y(2, 64);
    bool audio = true;
    for (int n = 0; n < 1500; ++n) {
      input(x, n * 64, 48000, 2);
      y.makeCopyOf(x);
      process(a, x);
      process(b, y);
      for (int c = 0; c < 2; ++c)
        audio &= std::memcmp(x.getReadPointer(c), y.getReadPointer(c),
                             64 * sizeof(float)) == 0;
    }
    check(audio, "restored state produces identical audio");
    auto xml = juce::AudioProcessor::getXmlFromBinary(saved.getData(),
                                                      (int)saved.getSize());
    auto tree = juce::ValueTree::fromXml(*xml);
    tree.getChildWithName("EXTRAS").getChildWithName("SCENE_B").setProperty(
        "unknown_parameter", .8f, nullptr);
    juce::AudioProcessor::copyXmlToBinary(*tree.createXml(), saved);
    b.setStateInformation(saved.getData(), (int)saved.getSize());
    check(b.extraState().sceneB == extra.sceneB, "unknown scene IDs ignored");
  }
  for (int k = 0; k < ut::count; ++k)
    if (ut::specs[(size_t)k].kind == 0) {
      UndertowProcessor p;
      auto *parameter = p.state.getParameter(ut::specs[(size_t)k].id);
      bool valid = true;
      for (float n : {0.f, .25f, .5f, .75f, 1.f}) {
        // Display text is rounded for reading (e.g. "120 Hz", "8.0 s"), so the
        // test is that every displayed string types back to itself.
        const auto text = parameter->getText(n, 128);
        const float parsed = parameter->getValueForText(text);
        valid &= parameter->getText(parsed, 128) == text;
      }
      valid &= parameter->getValueForText("invalid") == parameter->getValue();
      check(valid, ut::specs[(size_t)k].id);
    }
}
// Optional recordings for the bank check and the listening renders. Without
// them, synthetic sources are used:
//   UNDERTOW_TEST_RIFF   a WAV of a bass riff (the bank and space checks)
//   UNDERTOW_TEST_AUDIO  a folder of WAVs (--presets renders each through UNDERTOW)
juce::File testFile(const char *variable) {
  const char *v = std::getenv(variable);
  return v != nullptr && *v != 0 ? juce::File::getCurrentWorkingDirectory().getChildFile(v) : juce::File();
}
#include "CoreTests.inc"
#include "UiTests.inc"
#include "MutateTests.inc"
#include "GoldenTests.inc"
void presets(const juce::File &dir) {
  dir.createDirectory();
  juce::AudioFormatManager formats;
  formats.registerBasicFormats();
  const auto folder = testFile("UNDERTOW_TEST_AUDIO");
  auto files = folder.isDirectory() ? folder.findChildFiles(juce::File::findFiles, false, "*.wav")
                                    : juce::Array<juce::File>();
  for (int kind = 0; kind < 6 + files.size(); ++kind) {
    UndertowProcessor p;
    prepare(p, 48000, 256);
    juce::AudioBuffer<float> out(2, 48000 * 8), b(2, 256), source;
    out.clear();
    std::unique_ptr<juce::AudioFormatReader> r;
    juce::String name =
        kind < 6 ? juce::StringArray{"40Hz",  "80Hz",   "bass-riff",
                                     "noise", "clicks", "silence"}[kind]
                 : files[kind - 6].getFileNameWithoutExtension();
    if (kind >= 6) {
      r.reset(formats.createReaderFor(files[kind - 6]));
      check(r != nullptr, "read source WAV");
      if (!r)
        continue;
      const int sourceLength = (int)std::min(
          r->lengthInSamples, (juce::int64)(r->sampleRate * 8) + 2);
      source.setSize(2, sourceLength);
      source.clear();
      r->read(&source, 0, sourceLength, 0, true, true);
    }
    for (int pos = 0; pos < out.getNumSamples(); pos += 256) {
      const int n = std::min(256, out.getNumSamples() - pos);
      b.setSize(2, n, false, false, true);
      if (r) {
        b.clear();
        for (int ch = 0; ch < 2; ++ch)
          for (int i = 0; i < n; ++i) {
            const double at = (pos + i) * r->sampleRate / 48000;
            const int index = (int)at;
            const float fraction = (float)(at - index);
            if (index + 1 < source.getNumSamples())
              b.setSample(ch, i,
                          source.getSample(ch, index) * (1 - fraction) +
                              source.getSample(ch, index + 1) * fraction);
          }
      } else
        input(b, pos, 48000, kind);
      process(p, b);
      for (int c = 0; c < 2; ++c)
        out.copyFrom(c, pos, b, c, 0, n);
    }
    Stats st;
    st.add(out);
    st.print(name.toRawUTF8());
    check(st.safe(), "render bounded");
    juce::WavAudioFormat wav;
    auto file = dir.getChildFile(name + ".wav");
    file.deleteFile();
    std::unique_ptr<juce::AudioFormatWriter> w(wav.createWriterFor(
        new juce::FileOutputStream(file), 48000, 2, 24, {}, 0));
    if (w)
      w->writeFromAudioSampleBuffer(out, 0, out.getNumSamples());
    else
      check(false, "write WAV");
  }
  std::printf("Read-only source WAV inputs: %d\n", files.size());
}
} // namespace
int main(int argc, char **argv) {
  juce::ScopedJuceInitialiser_GUI gui;
  std::setvbuf(stdout, nullptr, _IOLBF, 0);
  const juce::String mode = argc > 1 ? argv[1] : "--null";
  if (mode == "--null")
    nullTest();
  else if (mode == "--anchor")
    anchorTest();
  else if (mode == "--tails")
    tails();
  else if (mode == "--stability")
    stress();
  else if (mode == "--clicks")
    clicks();
  else if (mode == "--rt")
    rt();
  else if (mode == "--cpu")
    cpu();
  else if (mode == "--state")
    stateTest();
  else if (mode == "--xy")
    xyTest();
  else if (mode == "--sync")
    syncTest();
  else if (mode == "--pulse")
    pulseTest();
  else if (mode == "--ui")
    uiTest(juce::File::getCurrentWorkingDirectory().getChildFile(argc > 2 ? argv[2] : "work/ui"));
  else if (mode == "--gestures")
    gesturesTest();
  else if (mode == "--bank")
    bankTest(juce::File::getCurrentWorkingDirectory().getChildFile(argc > 2 ? argv[2] : "work/bank"));
  else if (mode == "--mutate")
    mutateTest();
  else if (mode == "--mutdiag")
    mutateDiag();
  else if (mode == "--evolve-bank")
    evolveBankTest();
  else if (mode == "--space")
    spaceTest();
  else if (mode == "--text")
    textTest();
  else if (mode == "--golden")
    golden(juce::File::getCurrentWorkingDirectory().getChildFile(argc > 2 ? argv[2] : "work/golden"));
  else if (mode == "--golden-compare" && argc > 3)
    goldenCompare(juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]),
                  juce::File::getCurrentWorkingDirectory().getChildFile(argv[3]));
  else if (mode == "--presets")
    presets(juce::File::getCurrentWorkingDirectory().getChildFile(
        argc > 2 ? argv[2] : "work/default"));
  else {
    std::puts("Unsupported mode");
    return 2;
  }
  std::printf("RESULT %d failures\n", failures);
  return failures ? 1 : 0;
}
