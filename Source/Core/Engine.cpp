#include "Engine.h"
namespace ut {
namespace {
constexpr float pi = 3.14159265358979323846f;
float sat(float x) noexcept { return x / (1.f + std::abs(x)); }
float finite(float x) noexcept {
  return std::isfinite(x) ? limit(-1.f, 1.f, x) : 0;
}
} // namespace
void Engine::prepare(double sampleRate, int block) {
  sr = sampleRate;
  pulse.prepare(sr, block);
  lastReadDelay = 0;
  previousEchoDelay = previousReversePhase = previousAgePhase =
      oldEchoIncrement = oldAgeIncrement = echoIncrement = lastBeatIncrement =
          0;
  retimeMode = 0;
  lastEchoDivision = -1;
  lastEchoSync = false;
  echoRetime = 1;
  driftDelay[0] = driftDelay[1] = 0;
  driftSlew = 1.f - std::exp(-1.f / (float)(sr * .02));
  smoothing = 1.f - std::exp(-1.f / (float)(sr * .02));
  for (auto &d : drift)
    d.prepare(sr, .06);
  for (auto &d : echo)
    d.prepare(sr, 200);
  for (auto &d : pre)
    d.prepare(sr, .51);
  for (auto &d : late)
    d.prepare(sr, 2);
  for (auto &d : tank)
    d.prepare(sr, .35);
  for (auto &d : diffusers)
    d.prepare(sr, .025);
  loopGains.fill(0);
  filterCoefficients.fill(0);
  const double spec = sr;
  anchor.prepare(spec);
  anchor.setType(CrossoverType::lowpass);
  wetHigh.prepare(spec);
  wetHigh.setType(CrossoverType::highpass);
  phase = wowPhase = flutterPhase = agePhase = 0;
  modPhases.fill(0);
  duckEnv = freeze = gravityEnv = 0;
  damp.fill(0);
  echoFilters = {};
  wetFilters = {};
  std::fill(std::begin(gates), std::end(gates), 0);
  first = true;
  anchorCutoff = -1;
  bypassBlend = 0;
  echoTime = 0;
  previousMode = oldMode = 0;
  modeFade = 1;
  for (auto &f : fifthShift)
    f.prepare(sr, .05);
  for (auto &f : haloShift)
    f.prepare(sr, .08);
  for (auto &row : formants)
    for (auto &f : row)
      f.reset();
  mutGate = 0;
  for (int ch = 0; ch < 2; ++ch) {
    harmEnv[ch] = harmDc[ch] = harmLp[ch] = subPrev[ch] = subLp[ch] = 0;
    haloPrev[ch] = haloLp[ch] = 0;
    subFlip[ch] = false;
  }
  auto pole = [this](float hz) { return 1.f - std::exp(-2 * pi * hz / (float)sr); };
  envAttack = 1.f - std::exp(-1.f / (float)(sr * .003));
  envRelease = 1.f - std::exp(-1.f / (float)(sr * .12));
  dcCoef = pole(20);
  subTrack = pole(300);
  subSmooth = pole(800);
  haloTone = pole(7000);
  haloAttack = 1.f - std::exp(-1.f / (float)(sr * .02));
  haloRelease = 1.f - std::exp(-1.f / (float)(sr * .6));
  haloEnv = 0;
  formEnvCoef = 1.f - std::exp(-1.f / (float)(sr * .06));
  frozenCoef = 1.f - std::exp(-1.f / (float)(sr * .5));
  frozenGlide = 1.f - std::exp(-1.f / (float)(sr * .4));
  frozenGain = 1;
  spaceGuard = 1;
  guardRelease = 1.f - std::exp(-1.f / (float)(sr * .25));
  frozenPow = frozenRef = 0;
  frozenCount = 0;
  formEnvIn[0] = formEnvIn[1] = formEnvOut[0] = formEnvOut[1] = 0;
  evolveGlide = 1.f - std::exp(-1.f / (float)(sr * .08));
  evolveSize = evolveTone = targetSize = targetTone = 0;
  evolveForm = targetForm = .5f;
}
void Engine::process(AudioView &b, int inputs,
                     const std::array<float, count> &v, const ExtraState &extra,
                     const Timeline &time) noexcept {
  const double bpm = time.bpm;
  pulse.begin(v, extra, time);
  NoDenormals denorm;
  if (first) {
    smooth = v;
    first = false;
  }
  // MUTATE is on with its switch, or as soon as the MUTATE macro is pushed up.
  const bool mutActive = v[mut_on] > .5f || v[macro_mutate] > .5f;
  const bool allOff = v[drift_on] < .5f && v[echo_on] < .5f &&
                      v[void_on] < .5f && !Pulse::requested(v) && !mutActive;
  static constexpr double haloRatios[]{2.0, 1.4983070768766815, .5,
                                       2.9966141537533630};
  const double haloRatio = haloRatios[(size_t)limit(0, 3, (int)v[halo_interval])];
  const double evolveBars = (double)(1 << limit(0, 4, (int)v[evolve_cycle]));
  const bool dryTarget = allOff || (v[anchor_on] < .5f && v[mix] == 0);
  if (allOff && bypassBlend == 0) {
    if (inputs == 1)
      b.copyFrom(1, 0, b, 0, 0, b.getNumSamples());
    return;
  }
  const auto coeff = [this](float f) {
    return 1.f - std::exp(-2 * pi * std::min(f, (float)sr * .45f) / (float)sr);
  };
  const float lp = coeff(v[echo_tone] * (1.f - .65f * v[echo_age] / 100)),
              hp = coeff(v[echo_lowcut]);
  const float tankLP = coeff(16000.f * std::pow(.025f, v[void_damp] / 100));
  const float tankHP = coeff(v[void_lowcut]);
  const float attack = 1.f - std::exp(-1.f / (float)(sr * .005));
  const float release =
      1.f - std::exp(-1.f / (float)(sr * v[duck_release] * .001));
  float lengths[8], gains[8];
  static constexpr float times[]{.0311f, .0379f, .0437f, .0533f,
                                 .0617f, .0719f, .0797f, .0893f};
  for (int j = 0; j < 8; ++j) {
    lengths[j] = (float)sr * times[j] * (.6f + 2.f * v[void_size] / 100);
    gains[j] = std::pow(10.f, -3.f * lengths[j] / ((float)sr * v[void_decay]));
  }
  const float targetTime =
      v[echo_sync] > .5f
          ? (float)(60 / bpm *
                    beats[(size_t)limit(0, 20, (int)v[echo_div])] * sr)
          : v[echo_time] * (float)sr * .001f;
  if (echoTime == 0)
    echoTime = targetTime;
  const int mode = (int)v[echo_mode];
  const bool echoSettingsChanged =
      lastEchoDivision >= 0 && (lastEchoDivision != (int)v[echo_div] ||
                                lastEchoSync != (v[echo_sync] > .5f));
  lastEchoDivision = (int)v[echo_div];
  lastEchoSync = v[echo_sync] > .5f;
  if (time.discontinuity || echoSettingsChanged) {
    previousEchoDelay = lastReadDelay > 0 ? lastReadDelay : echoTime;
    previousReversePhase = reversePhase;
    previousAgePhase = agePhase;
    oldEchoIncrement = echoIncrement;
    oldAgeIncrement = lastBeatIncrement * .071;
    retimeMode = previousMode;
    echoRetime = 0;
    if (mode == 2)
      echoTime = targetTime;
  }
  echoIncrement = v[echo_sync] > .5f
                      ? 2 * time.increment / beats[(size_t)(int)v[echo_div]]
                      : 2 / std::max(1., echoTime);
  lastBeatIncrement = time.increment;
  if (mode != previousMode) {
    modeFade = 0;
    oldMode = previousMode;
    previousMode = mode;
  }
  float peakIn = 0, peakOut = 0;
  for (int n = 0; n < b.getNumSamples(); ++n) {
    for (size_t k = 0; k < count; ++k)
      smooth[k] += smoothing * (v[k] - smooth[k]);
    const auto &s = smooth;
    pulse.tick(s, n);
    if (n % 16 == 0 && std::abs(anchorCutoff - s[anchor_freq]) > .001f) {
      anchorCutoff = s[anchor_freq];
      anchor.setCutoffFrequency(anchorCutoff);
      wetHigh.setCutoffFrequency(anchorCutoff);
    }
    const float targets[]{lp, hp, tankLP, tankHP, release};
    for (size_t j = 0; j < 5; ++j)
      filterCoefficients[j] += smoothing * (targets[j] - filterCoefficients[j]);
    for (size_t j = 0; j < 8; ++j)
      loopGains[j] += smoothing * (gains[j] - loopGains[j]);
    float x[2]{b.getSample(0, n), b.getSample(inputs == 1 ? 0 : 1, n)},
        low[2]{}, high[2]{};
    const float original[]{x[0], x[1]};
    bypassBlend = limit(
        0.f, 1.f, bypassBlend + (dryTarget ? -1.f : 1.f) / (float)(sr * .02));
    peakIn = std::max({peakIn, std::abs(x[0]), std::abs(x[1])});
    const float inputGain = std::pow(10.f, s[in_gain] / 20),
                outputGain = std::pow(10.f, s[out_gain] / 20);
    for (int ch = 0; ch < 2; ++ch) {
      x[ch] *= inputGain;
      anchor.processSample(ch, x[ch], low[ch], high[ch]);
      high[ch] = x[ch] + s[anchor_on] * (high[ch] - x[ch]);
    }
    {
      float mono = .5f * (low[0] + low[1]);
      for (int ch = 0; ch < 2; ++ch)
        low[ch] += s[anchor_mono] * (mono - low[ch]);
    }
    float wet[2]{high[0], high[1]};
    for (int j = 0; j < 3; ++j) {
      const bool on = v[j == 0 ? drift_on : j == 1 ? echo_on : void_on] > .5f;
      gates[j] = limit(0.f, 1.f,
                              gates[j] + (on ? 1.f : -1.f) / (float)(sr * .02));
    }
    if (v[drift_sync] > .5f) {
      const double cycles = time.at(n) / beats[(size_t)(int)v[drift_div]];
      phase = fraction(cycles);
      wowPhase = fraction(cycles * .371);
      flutterPhase = fraction(cycles * 23.7);
    } else {
      phase = fraction(phase + s[drift_rate] / sr);
      wowPhase = fraction(wowPhase + s[drift_rate] * .371 / sr);
      flutterPhase = fraction(flutterPhase + s[drift_rate] * 23.7 / sr);
    }
    agePhase = fraction(time.at(n) * .071);
    driftPhase.store((float)phase, std::memory_order_relaxed);
    if (gates[0] > 0) {
      for (int ch = 0; ch < 2; ++ch) {
        const float ph =
            2 * pi * ((float)phase + .25f * ch * s[drift_width] / 100);
        const float wobble =
            std::sin(ph) +
            s[drift_wow] / 100 * std::sin(2 * pi * (float)wowPhase + .1f * ch) +
            .08f * s[drift_flutter] / 100 *
                std::sin(2 * pi * (float)flutterPhase + .17f * ch);
        const float delay =
            (.012f + .004f * s[drift_depth] / 100 * wobble) * (float)sr;
        if (driftDelay[ch] == 0)
          driftDelay[ch] = delay;
        driftDelay[ch] += driftSlew * (delay - driftDelay[ch]);
        const float y = drift[(size_t)ch].read(driftDelay[ch]);
        drift[(size_t)ch].push(wet[ch]);
        wet[ch] += gates[0] * s[drift_mix] / 100 * (y - wet[ch]);
      }
    } else
      for (auto &d : drift)
        d.reset();
    // ---- MUTATE: HARMONICS. New partials from the upper band, each made from
    // a level-normalised copy so the timbre doesn't depend on how hard you play.
    mutGate = limit(0.f, 1.f, mutGate + (mutActive ? 1.f : -1.f) / (float)(sr * .02));
    float harm[2]{};
    if (mutGate > 0) {
      const float aOct = s[harm_octave] / 100, aFifth = s[harm_fifth] / 100,
                  aOdd = s[harm_odd] / 100, aSub = s[harm_sub] / 100;
      for (int ch = 0; ch < 2; ++ch) {
        const float in = wet[ch], mag = std::abs(in);
        harmEnv[ch] += (mag > harmEnv[ch] ? envAttack : envRelease) * (mag - harmEnv[ch]);
        const float level = harmEnv[ch] * 1.6f + 1.e-5f;
        const float xn = limit(-1.5f, 1.5f, in / level);
        float octave = std::abs(xn) - .64f;               // rectified: twice the frequency
        harmDc[ch] += dcCoef * (octave - harmDc[ch]);
        octave -= harmDc[ch];
        const float odd = std::sin(4.7f * xn) * .6f;      // folded: odd partials
        harmLp[ch] += subTrack * (xn - harmLp[ch]);       // flip-flop: an octave below
        if (harmLp[ch] > 0 && subPrev[ch] <= 0)
          subFlip[ch] = !subFlip[ch];
        subPrev[ch] = harmLp[ch];
        subLp[ch] += subSmooth * ((subFlip[ch] ? 1.f : -1.f) - subLp[ch]);
        const float fifth = fifthShift[(size_t)ch].process(in, 1.4983070768766815);
        harm[ch] = mutGate * ((aOct * 1.2f * octave + aOdd * odd + aSub * .7f * subLp[ch]) * level +
                              aFifth * fifth);
        wet[ch] += harm[ch];
      }
    }
    // The space bus carries only what the effects add: DRIFT's moving copy of
    // the direct sound, the echoes and the reverb. The untouched upper band is
    // the dry path, so MIX can go all the way to space.
    const float feed[2]{wet[0], wet[1]};
    float echoReturn[2]{};
    if (gates[1] > 0) {
      const double glide = smoothing * .05f * (targetTime - echoTime);
      echoTime += mode == 2 ? std::clamp(glide, -.5, .5) : glide;
      const double reverseDelay = v[echo_sync] > .5f ? targetTime : echoTime;
      if (v[echo_sync] > .5f)
        reversePhase =
            fraction(2 * time.at(n) / beats[(size_t)(int)v[echo_div]]);
      else
        reversePhase = fraction(reversePhase + 2. / std::max(1., echoTime));
      echoPhase.store((float)reversePhase, std::memory_order_relaxed);
      echoRetime = std::min(1.f, echoRetime + 1.f / (float)(sr * .02));
      if (echoRetime < 1) {
        previousReversePhase =
            fraction(previousReversePhase + oldEchoIncrement);
        previousAgePhase = fraction(previousAgePhase + oldAgeIncrement);
      }
      float delayed[2], oldDelayed[2];
      for (int ch = 0; ch < 2; ++ch) {
        const float age = 1.f + .0015f * s[echo_age] / 100 *
                                    std::sin(2 * pi * (float)agePhase + ch);
        double t = reversePhase;
        const float a = echo[(size_t)ch].read(reverseDelay * (1. + t));
        t += .5f;
        t -= std::floor(t);
        const float z = echo[(size_t)ch].read(reverseDelay * (1. + t));
        const float w = .5f - .5f * std::cos(2 * pi * reversePhase);
        const float reversed = w * a + (1 - w) * z,
                    normal = echo[(size_t)ch].read(echoTime * age);
        oldDelayed[ch] = oldMode == 2 ? reversed : normal;
        const float next = mode == 2 ? reversed : normal;
        delayed[ch] = oldDelayed[ch] + modeFade * (next - oldDelayed[ch]);
        if (echoRetime < 1) {
          const float oldAge =
              1 + .0015f * s[echo_age] / 100 *
                      std::sin(2 * pi * (float)previousAgePhase + ch);
          const float normalOld =
              echo[(size_t)ch].read(std::max(1., previousEchoDelay) * oldAge);
          const float a = echo[(size_t)ch].read(previousEchoDelay *
                                                (1 + previousReversePhase));
          const float z = echo[(size_t)ch].read(
              previousEchoDelay * (1 + fraction(previousReversePhase + .5)));
          const float w = .5f - .5f * std::cos(2 * pi * previousReversePhase);
          const float old = retimeMode == 2 ? w * a + (1 - w) * z : normalOld;
          delayed[ch] = old + echoRetime * (delayed[ch] - old);
        }
      }
      lastReadDelay = mode == 2 ? reverseDelay : echoTime;
      modeFade = std::min(1.f, modeFade + 1.f / (float)(sr * .02));
      for (int ch = 0; ch < 2; ++ch) {
        const float d = echoFilters[(size_t)ch].run(
            (1 - modeFade) * oldDelayed[oldMode == 1 ? 1 - ch : ch] +
                modeFade * delayed[mode == 1 ? 1 - ch : ch],
            filterCoefficients[0], filterCoefficients[1]);
        const float drive = s[echo_drive] / 100 * 3;
        const float feedback =
            (drive > 0 ? sat(d * drive) / drive : d) * s[echo_feedback] / 100;
        echo[(size_t)ch].push(limit(-4.f, 4.f, wet[ch] + feedback));
        echoReturn[ch] = gates[1] * s[echo_level] / 100 * delayed[ch];
        wet[ch] += echoReturn[ch];
      }
    } else {
      for (auto &d : echo)
        d.reset();
      echoFilters = {};
    }
    if (pulse.position() == 0)
      pulse.apply(wet);
    float reverbReturn[2]{};
    float sizeMod = 1, toneMod = 1, evolveDepth = 0;
    if (mutGate > 0) {
      // ---- MUTATE: EVOLVE. Slow, repeatable wander tied to song position
      // (or the pad), moving the reverb's size, tone and formants.
      if ((n & 15) == 0) {
        if (v[evolve_follow] > .5f) {
          targetSize = (v[xy_x] - .5f) * 2;
          targetTone = (v[xy_y] - .5f) * 2;
          targetForm = .5f + std::atan2(v[xy_y] - .5f, v[xy_x] - .5f) / (2 * pi);
        } else {
          const double ph = time.at(n) / (time.barBeats * evolveBars);
          targetSize = wanderAt(ph, 11);
          targetTone = wanderAt(ph * 1.31, 29);
          targetForm = .5f + .5f * wanderAt(ph * .83, 47);
        }
      }
      evolveSize += evolveGlide * (targetSize - evolveSize);
      evolveTone += evolveGlide * (targetTone - evolveTone);
      evolveForm += evolveGlide * (targetForm - evolveForm);
      evolveDepth = mutGate * s[evolve_amount] / 100;
      sizeMod = 1 + .14f * evolveDepth * s[evolve_size] / 100 * evolveSize;
      toneMod = std::pow(4.f, evolveDepth * s[evolve_tone] / 100 * evolveTone);
      if ((n & 15) == 0 && evolveDepth * s[evolve_formant] > 0) {
        // vowel-like formant sets, glided through by evolveForm
        static constexpr float vowels[5][3]{{325, 700, 2530},  {450, 800, 2830},
                                            {800, 1150, 2900}, {400, 1600, 2700},
                                            {290, 1870, 2800}};
        const float pos = limit(0.f, .999f, evolveForm) * 4;
        const int i = (int)pos;
        const float t = pos - (float)i;
        for (int f = 0; f < 3; ++f) {
          const float hz = vowels[i][f] + t * (vowels[std::min(4, i + 1)][f] - vowels[i][f]);
          for (int ch = 0; ch < 2; ++ch)
            formants[(size_t)ch][(size_t)f].set(hz * (ch ? 1.03f : 1.f), 9.f, sr);
          if (n == 0)
            formantHz[(size_t)f].store(hz, std::memory_order_relaxed);
        }
      }
    }
    if (gates[2] > 0) {
      const float step = 1.f / (float)(sr * .08);
      const float oldFreeze = freeze;
      freeze = limit(0.f, 1.f,
                            freeze + (v[void_freeze] > .5f ? step : -step));
      if (oldFreeze == 0 && freeze > 0)
        for (int j = 0; j < 8; ++j)
          frozenLengths[(size_t)j] = std::round(lengths[j]);
      float taps[8];
      double sum = 0;
      for (int j = 0; j < 8; ++j) {
        modPhases[(size_t)j] += (.071 + .011 * j) / sr;
        modPhases[(size_t)j] -= std::floor(modPhases[(size_t)j]);
        const float mod = std::sin(2 * pi * (float)modPhases[(size_t)j]) *
                          .0002f * (float)sr * s[void_mod] / 100;
        const float len =
            (float)sr * times[j] * (.6f + 2.f * s[void_size] / 100) * sizeMod + mod;
        const float normal = freeze < 1 ? tank[(size_t)j].read(len) : 0;
        // a frozen tail keeps breathing very slightly with EVOLVE
        const float heldLength =
            sizeMod == 1 ? frozenLengths[(size_t)j]
                         : frozenLengths[(size_t)j] * (1 + (sizeMod - 1) * .15f);
        const float held =
            freeze > 0 ? tank[(size_t)j].read(heldLength) : 0;
        const float raw = normal * (1 - freeze) + held * freeze;
        const float dampCoef = toneMod == 1 ? filterCoefficients[2]
                                            : std::min(.99f, filterCoefficients[2] * toneMod);
        damp[(size_t)j] += dampCoef * (raw - damp[(size_t)j]);
        taps[j] = damp[(size_t)j] * (1 - freeze) + raw * freeze;
        sum += taps[j];
      }
      // A future spectral stage can sit before the feedback write.
      float feedback[8];
      for (int j = 0; j < 8; ++j)
        feedback[j] = taps[j] - (float)(sum * .25);
      if (freeze > 0) {
        const float angle =
            .006f * s[void_mod] / 100 * std::sin(2 * pi * (float)phase);
        const float cs = std::cos(angle), sn = std::sin(angle);
        for (int j = 0; j < 8; j += 2) {
          const float a = feedback[j], z = feedback[j + 1];
          feedback[j] = cs * a - sn * z;
          feedback[j + 1] = sn * a + cs * z;
        }
      }
      float space[2]{};
      for (int ch = 0; ch < 2; ++ch) {
        pre[(size_t)ch].push(wet[ch]);
        const float feed = pre[(size_t)ch].read(
            std::max(1.f, s[void_predelay] * (float)sr * .001f));
        // HALO: last sample's pitch-shifted tail goes back into the reverb
        float filtered =
            (wetFilters[(size_t)ch].run(feed, 1.f, filterCoefficients[3]) + haloPrev[ch]) *
            (1 - freeze);
        for (int stage = 0; stage < 4; ++stage) {
          auto &d = diffusers[(size_t)(ch * 4 + stage)];
          const float delayed = d.read((float)sr * (.0031f + .0043f * stage));
          const float g = .7f * s[void_diffusion] / 100;
          const float y = delayed - g * filtered;
          d.push(filtered + g * y);
          filtered = y;
        }
        for (int j = ch; j < 8; j += 2) {
          const float orthogonal = feedback[j];
          const float g = loopGains[(size_t)j] * (1 - freeze) + freeze;
          tank[(size_t)j].push(
              limit(-8.f, 8.f, filtered * .18f + orthogonal * g));
          space[ch] += taps[j] * (j % 4 < 2 ? .35f : -.35f);
        }
      }
      const float grav = s[void_gravity] / 100;
      gravityEnv +=
          (1.f - std::exp(-1.f / (float)(sr * (grav < 0 ? 1.5 : .15)))) *
          (std::max(std::abs(wet[0]), std::abs(wet[1])) - gravityEnv);
      for (int ch = 0; ch < 2; ++ch) {
        late[(size_t)ch].push(space[ch]);
        const float shaped =
            grav < 0 ? (1 + grav) * space[ch] -
                           grav * late[(size_t)ch].read((float)sr *
                                                        (.05f + 1.2f * -grav))
                     : space[ch] * (1 - grav * .7f) +
                           grav * .7f * late[(size_t)ch].read((float)sr * .7f);
        const float mid = .5f * (space[0] + space[1]);
        const float wide = mid + (shaped - mid) * s[void_width] / 100;
        reverbReturn[ch] = gates[2] * s[void_level] / 100 * wide * spaceMakeup;
      }
      // A frozen, mutated tail is very coherent, so as EVOLVE moves the held
      // reads its taps drift in and out of phase. While MUTATE runs, hold the
      // frozen output at the level it had when it froze.
      {
        float target = 1;
        if (freeze >= 1 && mutGate > 0) {
          const float pw = space[0] * space[0] + space[1] * space[1];
          frozenPow += frozenCoef * (pw - frozenPow);
          if (frozenCount < (int)(sr * .5)) {
            ++frozenCount;
            frozenRef = frozenPow;
          } else
            target = limit(.1f, 2.f, std::sqrt(frozenRef / (frozenPow + 1.e-20f)));
        } else {
          frozenCount = 0;
          frozenPow = space[0] * space[0] + space[1] * space[1];
        }
        frozenGain += frozenGlide * (target - frozenGain);
        if (std::abs(frozenGain - 1) < 1.e-6f) frozenGain = 1;
        if (frozenGain != 1)
          for (float &r : reverbReturn) r *= frozenGain;
      }
      // ---- MUTATE: HALO. Shift the tail and feed it back, so every pass round
      // the reverb climbs (or sinks). Gain falls with longer decays and is
      // soft-limited, so the loop stays bounded.
      float climb = mutGate * s[halo_climb] / 100 * 3.5f / (1 + .03f * s[void_decay]);
      if (climb > 0) {
        // a slow leveller on the loop: once the tail passes the ceiling the
        // climb backs off, so HALO thickens a long tail instead of piling up
        const float peak = std::max(std::abs(space[0]), std::abs(space[1]));
        haloEnv += (peak > haloEnv ? haloAttack : haloRelease) * (peak - haloEnv);
        const float over = haloCeiling / std::max(haloCeiling, haloEnv);
        climb *= over * over * over;
        // detune compounds on every pass, so it stays small
        const double spread = std::exp2(s[halo_spread] * .06 / 1200);
        for (int ch = 0; ch < 2; ++ch) {
          const float y = haloShift[(size_t)ch].process(space[ch], haloRatio * (ch ? 1 / spread : spread));
          haloLp[ch] += haloTone * (y - haloLp[ch]);
          haloPrev[ch] = sat(haloLp[ch] * climb * 1.5f) / 1.5f;
        }
      } else {
        haloPrev[0] = haloPrev[1] = 0;
        haloEnv = 0;
      }
      // ---- MUTATE: FORMANT. Resonant peaks glide through the tail.
      const float formantMix = evolveDepth * s[evolve_formant] / 100;
      if (formantMix > 0)
        for (int ch = 0; ch < 2; ++ch) {
          float f = 0;
          for (auto &band : formants[(size_t)ch])
            f += band.run(reverbReturn[ch]);
          // level-matched: the resonances recolour the tail, never make it louder
          // (power envelopes; some of the plain tail always stays underneath)
          formEnvIn[ch] += formEnvCoef * (reverbReturn[ch] * reverbReturn[ch] - formEnvIn[ch]);
          formEnvOut[ch] += formEnvCoef * (f * f - formEnvOut[ch]);
          const float match = limit(.25f, 5.f, std::sqrt(formEnvIn[ch] / (formEnvOut[ch] + 1.e-12f)));
          reverbReturn[ch] += .8f * formantMix * (f * match - reverbReturn[ch]);
        }
    } else {
      for (auto &d : tank)
        d.reset();
      for (auto &d : diffusers)
        d.reset();
      wetFilters = {};
      for (auto &d : pre)
        d.reset();
      for (auto &d : late)
        d.reset();
      damp.fill(0);
      freeze = 0;
    }
    // space = DRIFT's moving copy + echoes + reverb
    float space[2];
    for (int ch = 0; ch < 2; ++ch)
      space[ch] = gates[0] * s[drift_mix] / 100 * feed[ch] + echoReturn[ch] +
                  reverbReturn[ch] + harm[ch];
    // A future texture stage can sit here, ahead of ducking.
    const float env = std::max(std::abs(x[0]), std::abs(x[1]));
    duckEnv +=
        (env > duckEnv ? attack : filterCoefficients[4]) * (env - duckEnv);
    const float duck = 1.f / (1.f + duckEnv * s[duck_amount] * .16f);
    // Both sides stay at full level up to 50 %, then the other one fades:
    // 0 % dry only, 50 % both, 100 % space only.
    const float m = s[mix] / 100;
    const float dryGain = std::min(1.f, 2 * (1 - m)),
                wetGain = std::min(1.f, 2 * m);
    for (int ch = 0; ch < 2; ++ch) {
      const float filtered = wetHigh.processSample(ch, space[ch]);
      space[ch] += s[anchor_on] * (filtered - space[ch]);
    }
    if (pulse.position() == 2) // WET ONLY: the space pulses, the dry never does
      pulse.apply(space);
    // Space guard: a long, swelling space under a loud held part can pile up
    // past full scale. Above -3 dBFS the space alone is held down (instant
    // attack, 250 ms release); the dry signal is never touched.
    {
      const float pk = std::max(std::abs(space[0]), std::abs(space[1])) * wetGain * duck * outputGain;
      const float want = pk > spaceCeiling ? spaceCeiling / pk : 1.f;
      spaceGuard = std::min(want, spaceGuard + guardRelease * (1.f - spaceGuard));
      if (spaceGuard > .99999f) spaceGuard = 1;
      if (spaceGuard < 1)
        for (float &x : space) x *= spaceGuard;
    }
    scope[(size_t)(scopeAt++ & (scopeSize - 1))].store(
        .5f * (space[0] + space[1]) * wetGain * duck, std::memory_order_relaxed);
    float upper[2];
    for (int ch = 0; ch < 2; ++ch)
      upper[ch] = high[ch] * dryGain + space[ch] * wetGain * duck;
    if (pulse.position() == 1) // AFTER: everything above the anchor pulses
      pulse.apply(upper);
    for (int ch = 0; ch < 2; ++ch) {
      const float out =
          (s[anchor_on] * low[ch] * pulse.subGain() + upper[ch]) * outputGain;
      const float y =
          bypassBlend == 0
              ? original[ch]
              : finite(original[ch] * (1 - bypassBlend) + out * bypassBlend);
      b.setSample(ch, n, y);
      peakOut = std::max(peakOut, std::abs(y));
    }
  }
  scopeWrite.store(scopeAt, std::memory_order_release);
  evolveX.store(evolveSize, std::memory_order_relaxed);
  evolveY.store(evolveTone, std::memory_order_relaxed);
  mutateGate.store(mutGate, std::memory_order_relaxed);
  meterIn.store(peakIn, std::memory_order_relaxed);
  meterOut.store(peakOut, std::memory_order_relaxed);
  duckMeter.store(duckEnv, std::memory_order_relaxed);
}
} // namespace ut
