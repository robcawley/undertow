// MUTATE (0.4): harmonics, halo, evolve. Included by Render.cpp.
namespace mut {
double goertzel(const juce::AudioBuffer<float> &b, int from, int to, double hz, double sr = 48000) {
  const double w = juce::MathConstants<double>::twoPi * hz / sr, c = 2 * std::cos(w);
  double s1 = 0, s2 = 0;
  for (int i = from; i < to; ++i) {
    const double win = .5 - .5 * std::cos(juce::MathConstants<double>::twoPi * (i - from) / (to - from));
    const double s0 = win * .5 * (b.getSample(0, i) + b.getSample(1, i)) + c * s1 - s2;
    s2 = s1; s1 = s0;
  }
  return std::sqrt(std::max(0.0, s1 * s1 + s2 * s2 - c * s1 * s2)) / ((to - from) * .25);
}
// energy in a band, as a sum of Goertzel bins 2 Hz apart (a shifted partial
// spreads into a few nearby lines)
double band(const juce::AudioBuffer<float> &b, int from, int to, double lo, double hi) {
  double e = 0;
  for (double hz = lo; hz <= hi; hz += 2) { const double m = goertzel(b, from, to, hz); e += m * m; }
  return std::sqrt(e);
}
double db(double v) { return 20 * std::log10(std::max(1e-12, v)); }
double rmsOf(const juce::AudioBuffer<float> &b, int from, int to) {
  double s = 0;
  for (int c = 0; c < 2; ++c) for (int i = from; i < to; ++i) s += (double)b.getSample(c, i) * b.getSample(c, i);
  return std::sqrt(s / (2.0 * std::max(1, to - from)));
}
double centroid(const juce::AudioBuffer<float> &b, int from, int to) {
  double num = 0, den = 0;
  for (double hz = 100; hz < 8000; hz *= 1.12) { const double m = goertzel(b, from, to, hz); num += m * hz; den += m; }
  return den > 0 ? num / den : 0;
}
// Render `seconds` of input `kind` (0 sine at hz, 1 noise burst at start, 2 steady noise, 3 bass riff)
juce::AudioBuffer<float> render(UndertowProcessor &p, double seconds, int kind, double hz = 440,
                                std::function<void(int)> perBlock = nullptr) {
  p.setPlayConfigDetails(2, 2, 48000, 256);
  p.prepareToPlay(48000, 256);
  juce::AudioBuffer<float> out(2, (int)(48000 * seconds)), b(2, 256);
  juce::Random r(3);
  float lp = 0;
  int64_t pos = 0;
  for (int at = 0; at < out.getNumSamples(); at += 256) {
    const int n = std::min(256, out.getNumSamples() - at);
    b.setSize(2, n, false, false, true);
    if (kind == 3) input(b, pos, 48000, 2);
    else
      for (int i = 0; i < n; ++i) {
        float x = 0;
        const double t = (at + i) / 48000.0;
        if (kind == 0 || (kind == 4 && t < 1)) x = .25f * (float)std::sin(juce::MathConstants<double>::twoPi * hz * t);
        if (kind == 1 && t < .2) { lp += .3f * ((r.nextFloat() * 2 - 1) - lp); x = lp * .6f; }
        if (kind == 2) { lp += .3f * ((r.nextFloat() * 2 - 1) - lp); x = lp * .3f; }
        b.setSample(0, i, x); b.setSample(1, i, x);
      }
    if (perBlock) perBlock(at);
    process(p, b);
    for (int c = 0; c < 2; ++c) out.copyFrom(c, at, b, c, 0, n);
    pos += n;
  }
  return out;
}
void only(UndertowProcessor &p) { // mutate alone at full wet: no drift, echo or reverb
  set(p, ut::drift_on, 0); set(p, ut::echo_on, 0); set(p, ut::void_on, 0);
  set(p, ut::mix, 100); set(p, ut::mut_on, 1);
}
} // namespace mut

// Where the level of each MUTATE preset comes from: the riff (or a steady
// noise bed), with one part switched off at a time.
void mutateDiag() {
  for (int variant = 0; variant < (std::getenv("DIAG_PRESETS") ? 0 : 8); ++variant) {
    UndertowProcessor p;
    set(p, ut::drift_on, 0); set(p, ut::echo_on, 0); set(p, ut::mix, 100); set(p, ut::mut_on, 1);
    set(p, ut::evolve_amount, 100); set(p, ut::evolve_size, 100); set(p, ut::evolve_tone, 100); set(p, ut::evolve_formant, 100);
    set(p, ut::halo_climb, 100);
    const char *names[] {"all", "formant 0", "size 0", "tone 0", "evolve 0", "formant only", "halo 0", "no input"};
    if (variant == 5) { set(p, ut::evolve_size, 0); set(p, ut::evolve_tone, 0); }
    if (variant == 6) set(p, ut::halo_climb, 0);
    if (variant == 1) set(p, ut::evolve_formant, 0);
    if (variant == 2) set(p, ut::evolve_size, 0);
    if (variant == 3) set(p, ut::evolve_tone, 0);
    if (variant == 4) set(p, ut::evolve_amount, 0);
    const auto out = mut::render(p, 60, variant == 7 ? 1 : 2, 0, [&](int at) { if (at <= 48000 * 3 && at + 256 > 48000 * 3) set(p, ut::void_freeze, 1); });
    std::printf("frozen %-10s", names[variant]);
    for (int s = 4; s < 60; s += 3) std::printf(" %.0f", mut::db(mut::rmsOf(out, 48000 * s, 48000 * (s + 1))));
    std::printf("\n");
  }
  struct Off { const char *name; std::vector<std::pair<int, float>> v; };
  const std::vector<Off> offs{{"full", {}},
                              {"no HALO", {{ut::halo_climb, 0}}},
                              {"no formant", {{ut::evolve_formant, 0}}},
                              {"no harmonics", {{ut::harm_octave, 0}, {ut::harm_fifth, 0}, {ut::harm_odd, 0}, {ut::harm_sub, 0}}},
                              {"no EVOLVE", {{ut::evolve_amount, 0}}},
                              {"MUTATE off", {{ut::mut_on, 0}}}};
  const char *range = std::getenv("DIAG_PRESETS");
  std::vector<int> list;
  if (range) for (auto &t : juce::StringArray::fromTokens(range, ",", "")) list.push_back(t.getIntValue());
  else for (int i = 11; i < 16; ++i) list.push_back(i);
  auto all = offs;
  all.push_back({"no CURRENT", {{ut::xy_amount, 0}}});
  all.push_back({"no PULSE", {{ut::pulse_on, 0}}});
  all.push_back({"no gravity", {{ut::void_gravity, 0}}});
  for (int preset : list)
    for (auto &o : all) {
      UndertowProcessor q;
      q.setPlayConfigDetails(2, 2, 48000, 256);
      q.prepareToPlay(48000, 256);
      q.setCurrentProgram(preset);
      for (auto [k, v] : o.v) {
        set(q, k, v);
        auto e = q.extraState();
        e.sceneB[(size_t)k] = q.parameter(k).getValue();
        q.setExtraState(e);
      }
      // a held, bright three-note chord (saws), retriggered every 8 s, for 64 s
      juce::AudioBuffer<float> out(2, 48000 * 64), b(2, 256);
      double ph[3]{};
      const double hz[3]{65.4, 98.0, 146.8};
      for (int at = 0; at < out.getNumSamples(); at += 256) {
        for (int i = 0; i < 256; ++i) {
          const int t = (at + i) % (48000 * 8);
          const float env = t < 48000 * 7 ? std::min(1.f, t / 4800.f) : std::max(0.f, 1.f - (t - 48000 * 7) / 4800.f);
          float x = 0;
          for (int n = 0; n < 3; ++n) { ph[n] += hz[n] / 48000; ph[n] -= std::floor(ph[n]); x += (float)(2 * ph[n] - 1); }
          x *= .1f * env;
          b.setSample(0, i, x); b.setSample(1, i, x);
        }
        process(q, b);
        for (int ch = 0; ch < 2; ++ch) out.copyFrom(ch, at, b, ch, 0, 256);
      }
      int over = 0;
      for (int i = 0; i < out.getNumSamples(); ++i) over += std::abs(out.getSample(0, i)) > .9f;
      std::printf("%-34s %-13s rms %6.1f dB  peak %6.1f dB  over 0.9: %d\n", q.getProgramName(preset).toRawUTF8(), o.name,
                  mut::db(mut::rmsOf(out, 48000 * 8, 48000 * 64)), mut::db(out.getMagnitude(0, out.getNumSamples())), over);
    }
}

void mutateTest() {
  using namespace mut;
  // The shifter on its own: a clean octave and fifth from a sine.
  for (double ratio : {2.0, 1.4983070768766815, .5}) {
    ut::Shifter sh;
    sh.prepare(48000, .08);
    juce::AudioBuffer<float> o(2, 48000 * 2);
    for (int i = 0; i < o.getNumSamples(); ++i) {
      const float y = sh.process(.5f * (float)std::sin(juce::MathConstants<double>::twoPi * 220 * i / 48000), ratio);
      o.setSample(0, i, y); o.setSample(1, i, y);
    }
    const double want = band(o, 48000, 96000, 220 * ratio * .96, 220 * ratio * 1.04), total = rmsOf(o, 48000, 96000) * std::sqrt(2.0);
    std::printf("  Shifter x%.3f: wanted partial %.1f dB re input, everything %.1f dB\n", ratio, db(want / .5), db(total / .5));
    check(db(want / .5) > -4, "the shifter puts the shifted tone at close to full level (within a semitone band)");
  }
  // A. Off means off: every MUTATE amount at full with the switch off changes nothing.
  {
    UndertowProcessor a, b;
    a.setCurrentProgram(1); b.setCurrentProgram(1);
    for (int k : {ut::harm_octave, ut::harm_fifth, ut::harm_odd, ut::harm_sub, ut::halo_climb, ut::evolve_amount, ut::evolve_formant})
      set(b, k, 100);
    const auto x = render(a, 6, 3), y = render(b, 6, 3);
    bool same = true;
    for (int c = 0; c < 2; ++c) same &= std::memcmp(x.getReadPointer(c), y.getReadPointer(c), sizeof(float) * (size_t)x.getNumSamples()) == 0;
    check(same, "MUTATE off: every amount at full changes nothing (bit-identical)");
  }
  // B. Each harmonic adds its own partial to a 440 Hz tone.
  struct H { int param; double hz; const char *name; };
  for (const H &h : {H{ut::harm_octave, 880, "OCTAVE adds 880 Hz"}, H{ut::harm_fifth, 659.26, "FIFTH adds 659 Hz"},
                     H{ut::harm_odd, 1320, "ODD adds 1320 Hz"}, H{ut::harm_sub, 220, "SUB adds 220 Hz"}}) {
    UndertowProcessor p;
    only(p);
    set(p, h.param, 100);
    const auto out = render(p, 2, 0, 440);
    const double target = goertzel(out, 48000, 96000, h.hz), input = .25;
    std::printf("  %-22s %.1f dB re input\n", h.name, db(target / input));
    check(db(target / input) > -20, h.name);
  }
  // C. HALO climbs, and stays bounded at the extreme.
  {
    // a 220 Hz tone for one second, then the tail: how much of 440 and 880 Hz
    // (one and two octaves up) the tail holds, with and without HALO
    double up1[2], up2[2], base[2];
    for (int on = 0; on < 2; ++on) {
      UndertowProcessor p;
      set(p, ut::drift_on, 0); set(p, ut::echo_on, 0); set(p, ut::mix, 100); set(p, ut::anchor_on, 0);
      set(p, ut::void_decay, 8); set(p, ut::void_lowcut, 20); set(p, ut::mut_on, 1);
      set(p, ut::halo_climb, on ? 100.f : 0.f);
      set(p, ut::halo_spread, 0);
      auto out = render(p, 5, 4, 220);
      base[on] = band(out, 48000 * 2, 48000 * 4, 205, 235);
      up1[on] = band(out, 48000 * 2, 48000 * 4, 410, 470);
      up2[on] = band(out, 48000 * 2, 48000 * 4, 820, 940);
      std::printf("  [halo %d] tail RMS %.1f dB; bands around 220 %.1f, 440 %.1f, 880 %.1f, 1760 %.1f dB\n", on,
                  db(rmsOf(out, 48000 * 2, 48000 * 4)), db(base[on]), db(up1[on]), db(up2[on]),
                  db(band(out, 48000 * 2, 48000 * 4, 1640, 1880)));
    }
    std::printf("  HALO octave up, tail 1-3 s after, re the 220 Hz band: 440 band %+.1f dB, 880 band %+.1f dB with HALO; %+.1f / %+.1f dB without\n",
                db(up1[1] / base[1]), db(up2[1] / base[1]), db(up1[0] / base[0]), db(up2[0] / base[0]));
    check(db(up1[1] / base[1]) > -10 && db(up1[1] / base[1]) - db(up1[0] / base[0]) > 20, "HALO puts strong octaves into the tail");
    UndertowProcessor p;
    set(p, ut::drift_on, 0); set(p, ut::echo_on, 0); set(p, ut::mix, 100); set(p, ut::mut_on, 1);
    set(p, ut::void_decay, 100); set(p, ut::void_size, 100); set(p, ut::halo_climb, 100); set(p, ut::halo_interval, 3);
    set(p, ut::evolve_amount, 100);
    const auto out = render(p, 60, 2);
    Stats st; st.add(out); st.print("halo extreme 60 s");
    const double early = rmsOf(out, 48000 * 20, 48000 * 30), late = rmsOf(out, 48000 * 50, 48000 * 60);
    std::printf("  HALO extreme: RMS 20-30 s %.1f dB, 50-60 s %.1f dB\n", db(early), db(late));
    check(st.safe() && late < early * 2, "HALO at its extreme stays bounded");
  }
  // D. EVOLVE moves the sound, repeats exactly, and moves the formants.
  {
    auto evolving = [](UndertowProcessor &p) {
      set(p, ut::drift_on, 0); set(p, ut::echo_on, 0); set(p, ut::mix, 100); set(p, ut::mut_on, 1);
      set(p, ut::void_decay, 10); set(p, ut::evolve_amount, 100); set(p, ut::evolve_formant, 100);
      set(p, ut::evolve_cycle, 0); set(p, ut::evolve_size, 100); set(p, ut::evolve_tone, 100);
    };
    UndertowProcessor a, b, c;
    evolving(a); evolving(b);
    set(c, ut::drift_on, 0); set(c, ut::echo_on, 0); set(c, ut::mix, 100); set(c, ut::void_decay, 10);
    float lo = 1e9f, hi = 0;
    const auto x = render(a, 10, 2, 0, [&](int) { const float f = a.engine.formantHz[1].load(); if (f > 0) { lo = std::min(lo, f); hi = std::max(hi, f); } });
    const auto y = render(b, 10, 2), z = render(c, 10, 2);
    bool same = true;
    for (int ch = 0; ch < 2; ++ch) same &= std::memcmp(x.getReadPointer(ch), y.getReadPointer(ch), sizeof(float) * (size_t)x.getNumSamples()) == 0;
    check(same, "EVOLVE repeats exactly for the same song position");
    juce::AudioBuffer<float> d(x);
    for (int ch = 0; ch < 2; ++ch) d.addFrom(ch, 0, z, ch, 0, z.getNumSamples(), -1.f);
    std::printf("  EVOLVE changes the space by %.1f dB re the plain reverb; second formant moved %.0f-%.0f Hz\n",
                db(rmsOf(d, 48000 * 2, 48000 * 10) / rmsOf(z, 48000 * 2, 48000 * 10)), lo, hi);
    check(rmsOf(d, 48000 * 2, 48000 * 10) > rmsOf(z, 48000 * 2, 48000 * 10) * .1 && hi - lo > 200, "EVOLVE audibly moves size, tone and formants");
  }
  // E. A frozen tail keeps mutating without growing or dying.
  {
    UndertowProcessor p;
    set(p, ut::drift_on, 0); set(p, ut::echo_on, 0); set(p, ut::mix, 100); set(p, ut::mut_on, 1);
    set(p, ut::evolve_amount, 100); set(p, ut::evolve_size, 100); set(p, ut::evolve_tone, 100); set(p, ut::evolve_formant, 100);
    set(p, ut::halo_climb, 100);
    const auto out = render(p, 150, 2, 0, [&](int at) { if (at <= 48000 * 3 && at + 256 > 48000 * 3) set(p, ut::void_freeze, 1); });
    double lo = 1e9, hi = 0;
    for (int s = 10; s < 150; ++s) { const double r = rmsOf(out, 48000 * s, 48000 * (s + 1)); lo = std::min(lo, r); hi = std::max(hi, r); }
    std::printf("  Frozen + EVOLVE + HALO, 10-150 s: 1 s RMS range %.2f dB\n   per second:", db(hi / lo));
    for (int s = 10; s < 150; s += 2) std::printf(" %.0f", db(rmsOf(out, 48000 * s, 48000 * (s + 1))));
    std::printf("\n");
    Stats st; st.add(out);
    check(st.safe() && db(hi / lo) < 8, "a frozen, mutating tail holds its level");
  }
  // E2. Live (not frozen): HALO and EVOLVE at full on a steady bed don't swell.
  {
    UndertowProcessor p;
    set(p, ut::drift_on, 0); set(p, ut::echo_on, 0); set(p, ut::mix, 100); set(p, ut::mut_on, 1);
    set(p, ut::evolve_amount, 100); set(p, ut::evolve_size, 100); set(p, ut::evolve_tone, 100); set(p, ut::evolve_formant, 100);
    set(p, ut::halo_climb, 100);
    UndertowProcessor q;
    set(q, ut::drift_on, 0); set(q, ut::echo_on, 0); set(q, ut::mix, 100);
    const auto out = render(p, 120, 2), plain = render(q, 120, 2);
    double lo = 1e9, hi = 0;
    for (int s = 10; s < 120; ++s) { const double r = rmsOf(out, 48000 * s, 48000 * (s + 1)); lo = std::min(lo, r); hi = std::max(hi, r); }
    const double ref = rmsOf(plain, 48000 * 10, 48000 * 120);
    std::printf("  Live HALO + EVOLVE at full, 10-120 s: 1 s RMS %+.1f to %+.1f dB re the plain reverb\n   per 4 s:", db(lo / ref), db(hi / ref));
    for (int s = 10; s < 120; s += 4) std::printf(" %.0f", db(rmsOf(out, 48000 * s, 48000 * (s + 1))));
    std::printf("\n");
    check(db(hi / ref) < 6 && db(lo / ref) > -12, "live HALO + EVOLVE stays within +6 / -12 dB of the plain reverb");
  }
  // F. The MUTATE macro brings mutation in by itself.
  {
    UndertowProcessor a, b;
    a.setCurrentProgram(4); b.setCurrentProgram(4);
    set(b, ut::macro_mutate, 80);
    const auto x = render(a, 6, 3), y = render(b, 6, 3);
    juce::AudioBuffer<float> d(x);
    for (int c = 0; c < 2; ++c) d.addFrom(c, 0, y, c, 0, y.getNumSamples(), -1.f);
    std::printf("  MUTATE macro at +80 %%: %.1f dB difference re output\n", db(rmsOf(d, 48000, 48000 * 6) / rmsOf(x, 48000, 48000 * 6)));
    check(rmsOf(d, 48000, 48000 * 6) > rmsOf(x, 48000, 48000 * 6) * .05, "MUTATE macro engages mutation with the switch off");
  }
  // G. CPU with everything on, MUTATE included.
  {
    UndertowProcessor p;
    p.setCurrentProgram(11);
    set(p, ut::mut_on, 1);
    for (int k : {ut::harm_octave, ut::harm_fifth, ut::harm_odd, ut::harm_sub, ut::halo_climb, ut::evolve_amount, ut::evolve_formant}) set(p, k, 80);
    set(p, ut::pulse_focus_on, 1); set(p, ut::pulse_bands, 6);
    p.setPlayConfigDetails(2, 2, 48000, 64);
    p.prepareToPlay(48000, 64);
    juce::AudioBuffer<float> b(2, 64);
    int64_t pos = 0;
    for (int k = 0; k < 2000; ++k) { input(b, pos, 48000, 2); process(p, b); pos += 64; }
    double total = 0, worst = 0;
    const int blocks = 10000;
    for (int k = 0; k < blocks; ++k) {
      input(b, pos, 48000, 2); pos += 64;
      const auto t0 = std::chrono::steady_clock::now();
      process(p, b);
      const double us = std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - t0).count();
      total += us; worst = std::max(worst, us);
    }
    const double budget = 64.0 / 48000 * 1e6;
    std::printf("CPU with MUTATE at 48k/64: average %.2f%% worst %.2f%% of real time\n", total / blocks / budget * 100, worst / budget * 100);
    check(total / blocks / budget * 100 < 10, "average under 10 % with every engine and MUTATE");
  }
}

// ---- --evolve-bank: how much each preset's space moves from one pass of a
// repeating phrase to the next (the same measure as UndertowPairs, on a
// synthetic held chord and riff). Presets meant to evolve must move at least
// 0.8x as much as MUTATE / Living Freeze.
namespace evo {
constexpr int sr = 48000, loopLen = sr * 8, passes = 12;
using Frames = std::vector<std::array<double, 24>>;
Frames bands(const juce::AudioBuffer<float> &b) {
  constexpr int order = 12, n = 1 << order;
  juce::dsp::FFT fft(order);
  juce::dsp::WindowingFunction<float> win(n, juce::dsp::WindowingFunction<float>::hann);
  std::vector<float> w(2 * n);
  const int hop = sr / 2;
  Frames out;
  for (int at = 0; at + hop <= b.getNumSamples(); at += hop) {
    std::array<double, 24> e{};
    for (int f = at; f + n <= at + hop + n / 2 && f + n <= b.getNumSamples(); f += n / 2) {
      std::fill(w.begin(), w.end(), 0.f);
      for (int i = 0; i < n; ++i) w[(size_t)i] = .5f * (b.getSample(0, f + i) + b.getSample(1, f + i));
      win.multiplyWithWindowingTable(w.data(), n);
      fft.performFrequencyOnlyForwardTransform(w.data());
      for (int k = 1; k < n / 2; ++k) {
        const double hz = (double)k * sr / n;
        if (hz < 60 || hz >= 12000) continue;
        e[(size_t)(int)(24 * std::log(hz / 60) / std::log(200.0))] += (double)w[(size_t)k] * w[(size_t)k];
      }
    }
    out.push_back(e);
  }
  return out;
}
double change(const Frames &f) {
  const int per = loopLen / (sr / 2);
  double sum = 0;
  int count = 0;
  for (int w = per * 2; w < (int)f.size(); ++w) {
    double top = 0, s = 0;
    int used = 0;
    for (double v : f[(size_t)w]) top = std::max(top, v);
    for (size_t k = 0; k < 24; ++k) {
      const double a = f[(size_t)w][k], b = f[(size_t)(w - per)][k];
      if (std::max(a, b) < top * 1e-5) continue;
      const double d = 10 * std::log10((a + top * 1e-6) / (b + top * 1e-6));
      s += d * d;
      ++used;
    }
    if (used) { sum += std::sqrt(s / used); ++count; }
  }
  return count ? sum / count : 0;
}
// The slow movement: each pass's average spectrum against the previous pass's.
// Averaging over a whole pass removes the reverb's own shimmer and free LFOs.
double drift(const Frames &f) {
  const int per = loopLen / (sr / 2);
  std::vector<std::array<double, 24>> passMean;
  for (int start = 0; start + per <= (int)f.size(); start += per) {
    std::array<double, 24> m{};
    for (int w = start; w < start + per; ++w)
      for (size_t k = 0; k < 24; ++k) m[k] += f[(size_t)w][k] / per;
    passMean.push_back(m);
  }
  double sum = 0;
  int count = 0;
  for (size_t p = 3; p < passMean.size(); ++p) {
    double top = 0, s = 0;
    int used = 0;
    for (double v : passMean[p]) top = std::max(top, v);
    for (size_t k = 0; k < 24; ++k) {
      const double a = passMean[p][k], b = passMean[p - 1][k];
      if (std::max(a, b) < top * 1e-4) continue; // within 40 dB of the loudest band
      const double d = 10 * std::log10((a + top * 1e-6) / (b + top * 1e-6));
      s += d * d;
      ++used;
    }
    if (used) { sum += std::sqrt(s / used); ++count; }
  }
  return count ? sum / count : 0;
}
// a 4-bar phrase at 120 bpm: two bars of a saw riff, two of a held saw chord
void phrase(juce::AudioBuffer<float> &b, int64_t at, std::array<double, 4> &ph) {
  const int riff[]{36, 36, 48, 39, 36, 43, 46, 34};
  for (int i = 0; i < b.getNumSamples(); ++i) {
    const int t = (int)((at + i) % loopLen);
    float x = 0;
    if (t < sr * 4) {
      const int step = t / (sr / 4), inStep = t % (sr / 4);
      const double hz = 440 * std::pow(2.0, (riff[step % 8] - 69) / 12.0);
      ph[0] += hz / sr; ph[0] -= std::floor(ph[0]);
      x = (float)(2 * ph[0] - 1) * .25f * (inStep < sr / 6 ? 1.f : 0.f);
    } else {
      const double hz[3]{130.8, 196.0, 311.1};
      const float env = std::min(1.f, (t - sr * 4) / 4800.f) * std::min(1.f, (loopLen - t) / 4800.f);
      for (int k = 0; k < 3; ++k) { ph[(size_t)k + 1] += hz[k] / sr; ph[(size_t)k + 1] -= std::floor(ph[(size_t)k + 1]); x += (float)(2 * ph[(size_t)k + 1] - 1); }
      x *= .08f * env;
    }
    b.setSample(0, i, x);
    b.setSample(1, i, x);
  }
}
double measure(int preset) {
  UndertowProcessor q;
  q.setPlayConfigDetails(2, 2, sr, 256);
  q.prepareToPlay(sr, 256);
  q.setCurrentProgram(preset);
  set(q, ut::mix, 100); // the space alone
  juce::AudioBuffer<float> out(2, loopLen * passes), b(2, 256);
  std::array<double, 4> ph{};
  for (int at = 0; at < out.getNumSamples(); at += 256) {
    phrase(b, at, ph);
    process(q, b);
    for (int c = 0; c < 2; ++c) out.copyFrom(c, at, b, c, 0, 256);
  }
  return drift(bands(out));
}
} // namespace evo

void evolveBankTest() {
  const auto &bank = ut::factoryPresets();
  const double ref = evo::measure(15); // MUTATE / Living Freeze
  std::printf("Living Freeze reference: %.2f dB change in the average spectrum from one pass to the next\n", ref);
  const std::set<juce::String> evolving{"MUTATE", "RIOT PAIRS", "CINEMATIC", "SHIMMER", "FROZEN", "TEXTURE"};
  for (int i = 0; i < (int)bank.size(); ++i) {
    const double v = evo::measure(i);
    // the RIOT PAIRS and every evolving category from 0.6.0 on; the first MUTATE
    // presets keep their 0.4.0 voicing (Rust Organ and Sinking Cathedral are
    // meant to sit stiller than Living Freeze)
    const bool must = i >= 16 && (evolving.count(bank[(size_t)i].category) > 0 || juce::String(bank[(size_t)i].category) == "DRONE");
    std::printf("  %2d %-38s %5.2f dB  %4.2fx%s\n", i, (juce::String(bank[(size_t)i].category) + " / " + bank[(size_t)i].name).toRawUTF8(),
                v, v / ref, must ? (v >= .8 * ref ? "  evolving" : "  TOO STILL") : "");
    if (must) check(v >= .8 * ref, (juce::String(bank[(size_t)i].name) + " evolves at least 0.8x Living Freeze").toRawUTF8());
  }
}
