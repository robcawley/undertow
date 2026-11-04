// Checks the game-facing UNDERTOW core (Processor and the C API) with no JUCE.
//   GameCheck [golden-dir] [audition-source.wav] [audition-out-dir]
// golden-dir: the plug-in's golden renders (UndertowRender --golden); every
// plug-in preset rendered through the core must match them exactly.
#include "Processor.h"
#include "undertow.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <new>
#include <string>
#include <vector>

// ---- Allocation counting
static thread_local bool tracking = false;
static thread_local size_t allocations = 0;
void *operator new(std::size_t n) {
  if (tracking)
    ++allocations;
  if (void *p = std::malloc(n ? n : 1))
    return p;
  throw std::bad_alloc();
}
void *operator new[](std::size_t n) { return ::operator new(n); }
void operator delete(void *p) noexcept { std::free(p); }
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }
void operator delete[](void *p, std::size_t) noexcept { std::free(p); }

namespace {
int failures = 0;
void check(bool ok, const std::string &label) {
  std::printf("%s %s\n", ok ? "PASS" : "FAIL", label.c_str());
  if (!ok)
    ++failures;
}

// The plug-in render tool's test input: a bass riff with clicks, noise bursts
// on the right only.
void input(std::vector<float> &l, std::vector<float> &r, int64_t start, double sr) {
  for (size_t i = 0; i < l.size(); ++i) {
    const double t = (double)(start + (int64_t)i) / sr;
    const int notes[]{36, 36, 43, 39, 48, 34, 36, 46};
    const double f = 440.0 * std::pow(2.0, (notes[((int)(t * 4)) % 8] - 69) / 12.0);
    const float riff = .15f * (float)(2 * (t * f - std::floor(t * f)) - 1) *
                       ((int)(t * 16) % 4 < 3 ? 1.f : 0.f);
    uint32_t h = (uint32_t)(start + (int64_t)i) * 747796405u + 2891336453u;
    h = ((h >> ((h >> 28) + 4)) ^ h) * 277803737u;
    h = (h >> 22) ^ h;
    const float noise = ((float)(h & 65535) / 32768.f - 1) * .2f *
                        (std::fmod(t, .5) < .05 ? 1.f : 0.f);
    const float click = (start + (int64_t)i) % (int64_t)(sr * .5) == 0 ? .5f : 0;
    l[i] = riff + .5f * click;
    r[i] = riff + .5f * click + noise;
  }
}

struct Render {
  std::vector<float> left, right;
};

// Renders `seconds`, driven for `driven` seconds, in blocks of `block`.
// `at` runs before each block with the block's start sample.
template <typename F>
Render render(ut::Processor &p, double sr, int block, double seconds, double driven, F at) {
  const int total = (int)(sr * seconds), drivenSamples = (int)(sr * driven);
  Render out;
  out.left.resize((size_t)total);
  out.right.resize((size_t)total);
  std::vector<float> l, r;
  for (int pos = 0; pos < total; pos += block) {
    const int n = std::min(block, total - pos);
    l.assign((size_t)n, 0);
    r.assign((size_t)n, 0);
    if (pos < drivenSamples)
      input(l, r, pos, sr);
    at(pos);
    tracking = true;
    p.process(l.data(), r.data(), n);
    tracking = false;
    std::copy(l.begin(), l.end(), out.left.begin() + pos);
    std::copy(r.begin(), r.end(), out.right.begin() + pos);
  }
  return out;
}

struct Stats {
  double peak = 0, rms = 0, jump = 0;
  bool finite = true;
};
Stats stats(const Render &x, size_t from = 0, size_t to = SIZE_MAX) {
  Stats s;
  double power = 0;
  size_t n = 0;
  to = std::min(to, x.left.size());
  for (size_t i = from; i < to; ++i)
    for (const auto *ch : {&x.left, &x.right}) {
      const float v = (*ch)[i];
      if (!std::isfinite(v)) {
        s.finite = false;
        continue;
      }
      s.peak = std::max(s.peak, (double)std::abs(v));
      power += (double)v * v;
      ++n;
      if (i > from)
        s.jump = std::max(s.jump, (double)std::abs(v - (*ch)[i - 1]));
    }
  s.rms = std::sqrt(power / std::max<size_t>(1, n));
  return s;
}
double db(double x) { return 20 * std::log10(std::max(x, 1.e-12)); }

void writeWav(const std::string &path, const Render &x, int rate) {
  std::ofstream f(path, std::ios::binary);
  auto u32 = [&](uint32_t v) { f.write((const char *)&v, 4); };
  auto u16 = [&](uint16_t v) { f.write((const char *)&v, 2); };
  const uint32_t frames = (uint32_t)x.left.size(), bytes = frames * 8;
  f.write("RIFF", 4); u32(36 + bytes); f.write("WAVE", 4);
  f.write("fmt ", 4); u32(16); u16(3); u16(2); u32((uint32_t)rate); u32((uint32_t)rate * 8); u16(8); u16(32);
  f.write("data", 4); u32(bytes);
  for (uint32_t i = 0; i < frames; ++i) {
    f.write((const char *)&x.left[i], 4);
    f.write((const char *)&x.right[i], 4);
  }
}

// Minimal WAV reader: PCM 16/24/32 or float 32, mono or stereo.
bool readWav(const std::string &path, Render &out, int &rate) {
  std::ifstream f(path, std::ios::binary);
  if (!f)
    return false;
  std::vector<char> d((std::istreambuf_iterator<char>(f)), {});
  if (d.size() < 12 || std::memcmp(d.data(), "RIFF", 4) || std::memcmp(d.data() + 8, "WAVE", 4))
    return false;
  int format = 0, channels = 0, bits = 0;
  size_t pos = 12;
  auto rd = [&](size_t at, int n) { uint32_t v = 0; std::memcpy(&v, d.data() + at, (size_t)n); return v; };
  while (pos + 8 <= d.size()) {
    const uint32_t size = rd(pos + 4, 4);
    if (!std::memcmp(d.data() + pos, "fmt ", 4)) {
      format = (int)rd(pos + 8, 2);
      channels = (int)rd(pos + 10, 2);
      rate = (int)rd(pos + 12, 4);
      bits = (int)rd(pos + 22, 2);
      if (format == 0xFFFE)
        format = (int)rd(pos + 32, 2);
    } else if (!std::memcmp(d.data() + pos, "data", 4) && channels > 0) {
      const size_t bytesPer = (size_t)bits / 8, frames = std::min<size_t>(size, d.size() - pos - 8) / (bytesPer * (size_t)channels);
      out.left.resize(frames);
      out.right.resize(frames);
      for (size_t i = 0; i < frames; ++i)
        for (int c = 0; c < 2; ++c) {
          const char *s = d.data() + pos + 8 + (i * (size_t)channels + (size_t)std::min(c, channels - 1)) * bytesPer;
          float v = 0;
          if (format == 3 && bits == 32)
            std::memcpy(&v, s, 4);
          else if (bits == 16)
            v = (float)(int16_t)rd((size_t)(s - d.data()), 2) / 32768.f;
          else if (bits == 24) {
            int32_t x = (int32_t)(rd((size_t)(s - d.data()), 3) << 8) >> 8;
            v = (float)x / 8388608.f;
          } else if (bits == 32)
            v = (float)(int32_t)rd((size_t)(s - d.data()), 4) / 2147483648.f;
          (c ? out.right : out.left)[i] = v;
        }
      return frames > 0;
    }
    pos += 8 + size + (size & 1);
  }
  return false;
}

int game(const char *name) { return ut::Processor::findPreset(name); }

} // namespace

int main(int argc, char **argv) {
  using ut::Processor;
  const int games = Processor::gamePresetCount();
  std::printf("presets: %d game + %d plug-in\n", games, Processor::presetCount() - games);
  check(games == 12, "twelve game presets");

  // 1. Every plug-in preset through the core matches the plug-in's golden render.
  if (argc > 1) {
    int identical = 0, compared = 0;
    double worst = -400;
    for (int p = games; p < Processor::presetCount(); ++p) {
      char path[512];
      std::snprintf(path, sizeof path, "%s/%02d.f32", argv[1], p - games);
      std::ifstream f(path, std::ios::binary | std::ios::ate);
      if (!f)
        continue;
      std::vector<float> ref((size_t)f.tellg() / sizeof(float));
      f.seekg(0);
      f.read((char *)ref.data(), (std::streamsize)(ref.size() * sizeof(float)));
      auto proc = std::make_unique<Processor>();
      proc->prepare(48000, 256);
      proc->loadPreset(p);
      const auto out = render(*proc, 48000, 256, 7, 4, [](int) {});
      double err = 0, sig = 0;
      for (size_t i = 0; i < out.left.size() && 2 * i + 1 < ref.size(); ++i) {
        const double a = out.left[i] - ref[2 * i], b = out.right[i] - ref[2 * i + 1];
        err += a * a + b * b;
        sig += (double)ref[2 * i] * ref[2 * i] + (double)ref[2 * i + 1] * ref[2 * i + 1];
      }
      ++compared;
      if (err == 0)
        ++identical;
      else {
        const double d = 10 * std::log10(err / std::max(sig, 1.e-30));
        worst = std::max(worst, d);
        std::printf("  preset %d (%s) differs: %.1f dB\n", p - games, Processor::presetName(p), d);
      }
    }
    std::printf("core vs plug-in: identical %d / %d, worst %.1f dB\n", identical, compared, worst);
    check(compared == Processor::presetCount() - games, "golden renders found for every plug-in preset");
    check(worst <= -120, "every plug-in preset sounds the same through the game core");
  }

  // 2. Game presets: steady renders at 48 kHz, levels, CPU, no allocation.
  std::vector<Stats> steady((size_t)games);
  double worstCpu = 0;
  for (int g = 0; g < games; ++g) {
    auto p = std::make_unique<Processor>();
    p->prepare(48000, 1024);
    p->loadPreset(g);
    allocations = 0;
    const auto t0 = std::chrono::steady_clock::now();
    const auto out = render(*p, 48000, 512, 10, 6, [](int) {});
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    const auto s = stats(out, 0, 48000 * 6);
    // Dry reference over the same span.
    Render dry;
    dry.left.resize(48000 * 6);
    dry.right.resize(48000 * 6);
    input(dry.left, dry.right, 0, 48000);
    const double level = db(s.rms) - db(stats(dry).rms);
    const double cpu = 100 * seconds / 10;
    worstCpu = std::max(worstCpu, cpu);
    steady[(size_t)g] = s;
    std::printf("  %-11s level %+5.1f dB re dry, peak %.3f, CPU %.2f %% of one core\n",
                Processor::presetName(g), level, s.peak, cpu);
    check(s.finite && s.peak <= 1.f, std::string(Processor::presetName(g)) + ": finite and under full scale");
    check(std::abs(level) <= 4, std::string(Processor::presetName(g)) + ": level within 4 dB of dry");
    check(allocations == 0, std::string(Processor::presetName(g)) + ": no allocation while processing");
  }
  std::printf("worst CPU: %.2f %% of one core per instance (48 kHz)\n", worstCpu);

  // 3. Sample rates and block sizes.
  for (double sr : {44100.0, 48000.0, 96000.0})
    for (int block : {1, 37, 256, 511, 1024, 4096}) {
      bool ok = true;
      for (int g = 0; g < games; ++g) {
        auto p = std::make_unique<Processor>();
        p->prepare(sr, 1024);
        p->loadPreset(g);
        allocations = 0;
        const auto s = stats(render(*p, sr, block, 3, 2, [](int) {}));
        ok = ok && s.finite && s.peak <= 1.f && allocations == 0;
      }
      char label[96];
      std::snprintf(label, sizeof label, "all game presets bounded at %.0f Hz, block %d", sr, block);
      check(ok, label);
    }

  // 4. Non-finite input never reaches the output, and sound carries on after.
  {
    auto p = std::make_unique<Processor>();
    p->prepare(48000, 1024);
    p->loadPreset(game("Void"));
    std::vector<float> l(512), r(512);
    bool finite = true;
    double after = 0;
    for (int b = 0; b < 400; ++b) {
      input(l, r, b * 512, 48000);
      if (b >= 50 && b < 60)
        for (size_t i = 0; i < l.size(); i += 7) {
          l[i] = NAN;
          r[i] = (i & 8) ? INFINITY : -INFINITY;
        }
      p->process(l.data(), r.data(), 512);
      for (size_t i = 0; i < l.size(); ++i) {
        finite = finite && std::isfinite(l[i]) && std::isfinite(r[i]);
        if (b > 300)
          after = std::max(after, (double)std::abs(l[i]));
      }
    }
    check(finite, "NaN and infinite input: output stays finite");
    check(after > .01, "NaN and infinite input: the effect keeps working afterwards");
  }

  // 5. Denormal soak: a tiny signal, then a long silence, must not slow down.
  {
    auto p = std::make_unique<Processor>();
    p->prepare(48000, 1024);
    p->loadPreset(game("Void"));
    std::vector<float> l(512), r(512);
    auto timeSeconds = [&](int blocks, float level) {
      const auto t0 = std::chrono::steady_clock::now();
      for (int b = 0; b < blocks; ++b) {
        for (size_t i = 0; i < l.size(); ++i)
          l[i] = r[i] = level * (float)((i * 2654435761u >> 16) & 1 ? 1 : -1);
        p->process(l.data(), r.data(), 512);
      }
      return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    };
    const double loud = timeSeconds(940, .1f);  // 10 s
    timeSeconds(94, 1.e-30f);                   // 1 s of almost nothing
    const double quiet = timeSeconds(1880, 0);  // 20 s of silence
    std::printf("  denormal soak: %.3f s per 10 s loud, %.3f s per 10 s silent\n", loud, quiet / 2);
    check(quiet / 2 < loud * 2.5, "silence after a tiny signal costs no more than playing");
  }

  // 6. Morphs between every pair of game presets: no clicks, no overs.
  {
    int pairs = 0, clean = 0;
    double worstRatio = 0;
    for (int a = 0; a < games; ++a)
      for (int b = 0; b < games; ++b) {
        if (a == b)
          continue;
        auto p = std::make_unique<Processor>();
        p->prepare(48000, 1024);
        p->loadPreset(a);
        const int start = 48000 * 2;
        allocations = 0;
        const auto out = render(*p, 48000, 512, 5, 5, [&](int pos) {
          if (pos <= start && start < pos + 512)
            p->morphTo(b, 1.5f);
        });
        const auto during = stats(out, (size_t)start, (size_t)(start + 48000 * 2));
        const double reference = std::max(steady[(size_t)a].jump, steady[(size_t)b].jump);
        const double ratio = during.jump / std::max(reference, 1.e-9);
        worstRatio = std::max(worstRatio, ratio);
        const bool ok = during.finite && during.peak <= 1.f && during.jump <= reference * 1.5 + .02 &&
                        allocations == 0 && p->currentPreset() == b && !p->isMorphing();
        ++pairs;
        if (ok)
          ++clean;
        else
          std::printf("  morph %s -> %s: jump %.3f vs steady %.3f, peak %.3f, alloc %zu\n",
                      Processor::presetName(a), Processor::presetName(b), during.jump, reference,
                      during.peak, allocations);
      }
    std::printf("morphs: %d / %d clean; largest step during a morph %.2fx the steady presets'\n",
                clean, pairs, worstRatio);
    check(clean == pairs, "every morph between game presets is click-free and bounded");
  }

  // 7. Control from another thread's point of view: parameters, freeze, tempo.
  {
    auto p = std::make_unique<Processor>();
    p->prepare(48000, 1024);
    p->loadPreset(game("Cave"));
    const int mixIndex = Processor::findParameter("mix");
    check(mixIndex == ut::mix, "find parameter by id");
    p->setParameter(mixIndex, 80);
    p->setFreeze(true);
    p->setTempo(90);
    std::vector<float> l(256), r(256);
    p->process(l.data(), r.data(), 256);
    check(std::abs(p->parameter(mixIndex) - 80) < 1.e-3f, "set parameter takes effect");
    check(p->parameter(ut::void_freeze) == 1, "freeze takes effect");
    int queued = 0;
    while (p->setParameter(mixIndex, 50))
      ++queued;
    check(queued == 1024, "control queue holds 1024 changes, then refuses");
    p->process(l.data(), r.data(), 256);
    check(p->setParameter(mixIndex, 50), "queue drains on the next block");
    check(Processor::findPreset("Nope") == -1 && Processor::findParameter("nope") == -1,
          "unknown names return -1");
  }

  // 8. The C interface.
  {
    ut_instance *u = ut_create(48000, 512);
    check(u != nullptr, "C: create");
    check(ut_preset_count() == Processor::presetCount() && ut_game_preset_count() == games, "C: preset counts");
    check(std::strcmp(ut_preset_name(ut_find_preset("Orbit")), "Orbit") == 0, "C: preset names");
    check(ut_parameter_count() == ut::count && std::strcmp(ut_parameter_id(0), "mix") == 0, "C: parameter table");
    std::vector<float> st(1024, 0);
    check(ut_morph_to(u, ut_find_preset("Dream"), .2f) == 1, "C: morph accepted");
    for (int b = 0; b < 40; ++b)
      ut_process_interleaved(u, st.data(), 512);
    check(ut_current_preset(u) == ut_find_preset("Dream") && !ut_is_morphing(u), "C: morph completes");
    check(ut_set_parameter(u, -1, 0) == 0 && ut_morph_to(u, 9999, 1) == 0, "C: rejects bad input");
    ut_destroy(u);
  }

  // 9. Auditions: each game preset on real music, and a tour that morphs
  // through all twelve.
  if (argc > 3) {
    Render source;
    int rate = 48000;
    if (readWav(argv[2], source, rate)) {
      const size_t length = std::min<size_t>(source.left.size(), (size_t)rate * 16);
      for (int g = 0; g < games; ++g) {
        auto p = std::make_unique<Processor>();
        p->prepare(rate, 1024);
        p->loadPreset(g);
        Render out{std::vector<float>(source.left.begin(), source.left.begin() + (long)length),
                   std::vector<float>(source.right.begin(), source.right.begin() + (long)length)};
        out.left.resize(length + (size_t)rate * 4, 0);
        out.right.resize(length + (size_t)rate * 4, 0);
        for (size_t pos = 0; pos < out.left.size(); pos += 512)
          p->process(&out.left[pos], &out.right[pos], (int)std::min<size_t>(512, out.left.size() - pos));
        char path[512];
        std::snprintf(path, sizeof path, "%s/%02d %s.wav", argv[3], g + 1, Processor::presetName(g));
        writeWav(path, out, rate);
      }
      // The tour: 6 s in each preset, 2 s morphs, music looping underneath.
      auto p = std::make_unique<Processor>();
      p->prepare(rate, 1024);
      p->loadPreset(0);
      const size_t total = (size_t)rate * 8 * (size_t)games;
      Render out;
      out.left.resize(total);
      out.right.resize(total);
      for (size_t i = 0; i < total; ++i) {
        out.left[i] = source.left[i % source.left.size()];
        out.right[i] = source.right[i % source.right.size()];
      }
      for (size_t pos = 0; pos < total; pos += 512) {
        const size_t step = (size_t)rate * 8;
        if (pos % step < 512 && pos >= step)
          p->morphTo((int)(pos / step) % games, 2.f);
        p->process(&out.left[pos], &out.right[pos], (int)std::min<size_t>(512, total - pos));
      }
      writeWav(std::string(argv[3]) + "/00 Tour (all twelve, 2 s morphs).wav", out, rate);
      std::printf("auditions written to %s\n", argv[3]);
    } else
      check(false, std::string("read audition source ") + argv[2]);
  }

  std::printf("RESULT %d failures\n", failures);
  return failures ? 1 : 0;
}
