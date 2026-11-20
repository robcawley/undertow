std::array<float, ut::count> values(UndertowProcessor &p) {
  std::array<float, ut::count> result{};
  for (size_t k = 0; k < result.size(); ++k)
    result[k] = p.state.getRawParameterValue(ut::specs[k].id)->load();
  return result;
}
void activatePulse(UndertowProcessor &p, int bands) {
  set(p, ut::pulse_on, 1);
  set(p, ut::pulse_bands, (float)bands);
  set(p, ut::pulse_focus_on, 1);
  set(p, ut::pulse_focus_harm, 8);
  set(p, ut::pulse_focus_depth, 100);
  set(p, ut::pulse_focus_q, 60);
  for (int i = 0; i < 6; ++i) {
    set(p, ut::pulse_b1_depth + 7 * i, 100);
    set(p, ut::pulse_b1_shape + 7 * i, (float)(i % 5));
  }
}
void exchangeTest();
void xyTest() {
  exchangeTest();
  UndertowProcessor p;
  auto a = values(p);
  auto frame = p.extraState();
  ut::Timeline time;
  auto result = p.modulation.evaluate(a, frame, time);
  check(result.values == a,
        "neutral scene / XY / macros leave real values exact");
  const int target = ut::void_decay;
  auto *parameter = p.state.getParameter(ut::specs[target].id);
  a[target] = parameter->convertFrom0to1(.3f);
  frame.sceneB[target] = .7f;
  for (int i = 0; i < 4; ++i)
    frame.corners[(size_t)i][target] = .05f * (i + 1);
  const float points[][2]{{0, 1}, {1, 1}, {1, 0}, {0, 0}};
  bool exact = true;
  for (int i = 0; i < 4; ++i) {
    a[ut::xy_x] = points[i][0];
    a[ut::xy_y] = points[i][1];
    result = p.modulation.evaluate(a, frame, time);
    const float base = parameter->convertTo0to1(a[target]);
    const float expected = base + frame.corners[(size_t)i][target];
    exact &= result.normalized[target] == expected;
    a[(size_t)(ut::xy_flip_a + i)] = 1;
    result = p.modulation.evaluate(a, frame, time);
    exact &=
        result.normalized[target] == base - frame.corners[(size_t)i][target];
    a[(size_t)(ut::xy_flip_a + i)] = 0;
  }
  check(exact, "all four full corners and flips equal signed normalized "
               "offsets exactly");
  a[ut::xy_x] = a[ut::xy_y] = .5f;
  a[ut::bloom] = 0;
  result = p.modulation.evaluate(a, frame, time);
  check(result.values[target] == a[target],
        "BLOOM zero equals Scene A exactly");
  a[ut::bloom] = 100;
  result = p.modulation.evaluate(a, frame, time);
  check(result.normalized[target] == frame.sceneB[target] &&
            result.values[target] ==
                parameter->convertFrom0to1(frame.sceneB[target]),
        "BLOOM full equals Scene B exactly");
  a[ut::bloom] = 50;
  result = p.modulation.evaluate(a, frame, time);
  check(std::abs(result.normalized[target] - .5f) < 1.e-7,
        "BLOOM midpoint linear in normalized space");
  bool choices = true;
  for (int k = 0; k < ut::count; ++k)
    if (ut::specs[(size_t)k].kind != 0)
      choices &= result.values[(size_t)k] == a[(size_t)k];
  check(choices, "choices and booleans follow Scene A");
  a[ut::bloom] = 0;
  a[ut::xy_x] = .25f;
  a[ut::xy_y] = .75f;
  result = p.modulation.evaluate(a, frame, time);
  check(ut::Modulation::weights(.25f, .75f)[0] == .25f,
        "centre-to-corner quadrant has bilinear weight");
  a[ut::xy_amount] = 0;
  result = p.modulation.evaluate(a, frame, time);
  check(result.values[target] == a[target], "XY amount zero is neutral");
  a[ut::xy_amount] = 100;
  for (int mode = 1; mode <= 3; ++mode) {
    a[ut::xy_x] = a[ut::xy_y] = .5f;
    a[ut::xy_current] = (float)mode;
    time.beat = 17.125;
    const auto first = p.modulation.evaluate(a, frame, time),
               second = p.modulation.evaluate(a, frame, time);
    check(first.values == second.values && first.x == second.x &&
              first.y == second.y,
          "Current is deterministic from absolute beat");
  }
  a[ut::xy_current] = 0;
  frame.motionCount = 2;
  frame.motion[0] = {0, .1f, .2f};
  frame.motion[1] = {2, .9f, .8f};
  a[ut::xy_motion_play] = 1;
  a[ut::xy_motion_bars] = 2;
  time.beat = 4;
  result = p.modulation.evaluate(a, frame, time);
  check(std::abs(result.x - .5f) < 1.e-6 && std::abs(result.y - .5f) < 1.e-6,
        "stored path interpolates bar position");
  a[ut::xy_current] = 3;
  const auto ignored = p.modulation.evaluate(a, frame, time);
  check(ignored.x == result.x && ignored.y == result.y,
        "recorded path takes priority over Current");
  auto golden = values(p);
  auto clean = p.extraState();
  ut::Timeline position;
  golden[ut::xy_current] = 1;
  position.beat = 0;
  auto orbit = p.modulation.evaluate(golden, clean, position);
  check(std::abs(orbit.x - .6f) < 1.e-6 && orbit.y == .5f,
        "Orbit begins one radius to the right of the puck");
  golden[ut::xy_current] = 2;
  golden[ut::xy_angle] = 0;
  position.beat = 2;
  auto tide = p.modulation.evaluate(golden, clean, position);
  check(std::abs(tide.x - .6f) < 1.e-6 && tide.y == .5f,
        "Tide follows its angle and tempo division");
  golden[ut::xy_current] = 0;
  golden[ut::macro_glow] = 100;
  auto bright = p.modulation.evaluate(golden, clean, position);
  check(bright.values[ut::void_damp] < golden[ut::void_damp] &&
            bright.values[ut::echo_tone] > golden[ut::echo_tone] &&
            bright.values[ut::echo_lowcut] > golden[ut::echo_lowcut],
        "Glow up opens the top and removes wet low-frequency weight");
  auto routing = values(p);
  auto routeFrame = p.extraState();
  routing[ut::xy_x] = 0;
  routing[ut::xy_y] = 1;
  routeFrame.sceneB[ut::void_decay] = .8f;
  routeFrame.corners[0][ut::bloom] = .5f;
  auto routed = p.modulation.evaluate(routing, routeFrame, position);
  const float sourceNorm = p.state.getParameter("void_decay")
                               ->convertTo0to1(routing[ut::void_decay]);
  check(routed.values[ut::bloom] == 50 &&
            std::abs(routed.normalized[ut::void_decay] -
                     (sourceNorm + .5f * (.8f - sourceNorm))) < 1.e-6,
        "corner offsets to BLOOM alter the scene morph");
  routeFrame.corners[0].fill(0);
  routing[ut::xy_amount] = 50;
  routeFrame.corners[0][ut::xy_amount] = .5f;
  routeFrame.corners[0][ut::void_decay] = .1f;
  routed = p.modulation.evaluate(routing, routeFrame, position);
  check(routed.values[ut::xy_amount] == 75 &&
            std::abs(routed.normalized[ut::void_decay] - (sourceNorm + .075f)) <
                1.e-6,
        "corner offsets to XY amount scale audio destinations after a bounded "
        "control prepass");
  routeFrame.corners[0].fill(0);
  routing[ut::xy_amount] = 100;
  routing[ut::xy_x] = .25f;
  routing[ut::xy_y] = .75f;
  routing[ut::xy_current] = 1;
  position.beat = 0;
  routeFrame.corners[0][ut::xy_radius] = .2f;
  const auto radiusChanged =
      p.modulation.evaluate(routing, routeFrame, position);
  auto noRadius = routeFrame;
  noRadius.corners[0][ut::xy_radius] = 0;
  const auto radiusBase = p.modulation.evaluate(routing, noRadius, position);
  check(radiusChanged.x > radiusBase.x,
        "corner offsets to Current radius change the moving effective puck");
  p.copySceneAToB();
  const auto captured = p.extraState();
  bool capturedExact = true;
  for (int k = 0; k < ut::count; ++k)
    if (ut::snapshotParameter(k))
      capturedExact &=
          captured.sceneB[(size_t)k] ==
          p.state.getParameter(ut::specs[(size_t)k].id)->getValue();
  check(capturedExact, "Copy A to B captures eligible values");
  const float original = p.state.getParameter("void_decay")->getValue();
  p.setSceneBValue(target, .8f);
  p.swapScenes();
  check(p.extraState().sceneB[target] == original &&
            std::abs(p.state.getParameter("void_decay")->getValue() - .8f) <
                1.e-6,
        "Swap exchanges scenes without changing choices");
  p.copySceneBToA();
  check(std::abs(p.state.getParameter("void_decay")->getValue() - original) <
            1.e-6,
        "Copy B to A restores captured values");
  auto rawBefore = values(p);
  prepare(p);
  juce::AudioBuffer<float> b(2, 64);
  for (int i = 0; i < 250; ++i) {
    input(b, i * 64, 48000, 2);
    process(p, b);
  }
  check(rawBefore == values(p),
        "audio thread does not write parameter storage");
  for (int macro :
       {ut::macro_depth, ut::macro_motion, ut::macro_glow, ut::macro_chaos}) {
    auto patch = values(p);
    auto f = p.extraState();
    patch[(size_t)macro] = 100;
    const auto up = p.modulation.evaluate(patch, f, time);
    patch[(size_t)macro] = -100;
    const auto down = p.modulation.evaluate(patch, f, time);
    bool bounded = true, changed = false;
    for (int k = 0; k < ut::count; ++k) {
      bounded &= up.values[(size_t)k] >= ut::specs[(size_t)k].lo &&
                 up.values[(size_t)k] <= ut::specs[(size_t)k].hi;
      changed |= up.values[(size_t)k] != down.values[(size_t)k] && k != macro;
    }
    check(bounded && changed,
          "macro endpoints affect bounded effective targets");
  }
}
void exchangeTest() {
  ut::StateExchange exchange;
  std::atomic<bool> done{false};
  bool coherent = true;
  int frames = 0;
  std::thread reader([&] {
    do {
      const auto &f = exchange.consume();
      if (f.motionCount == 1) {
        const float mark = f.sceneB[0];
        for (float n : f.sceneB)
          coherent &= n == mark;
        for (const auto &corner : f.corners)
          for (float n : corner)
            coherent &= n == mark;
        coherent &= f.motion[0].x == mark;
        for (auto pattern : f.patterns)
          coherent &= pattern == (uint16_t)(mark * 65536);
        ++frames;
      }
    } while (!done.load(std::memory_order_acquire));
  });
  ut::ExtraState f;
  f.motionCount = 1;
  for (int i = 1; i <= 50000; ++i) {
    const float mark = (float)(i % 65536) / 65536;
    f.sceneB.fill(mark);
    for (auto &corner : f.corners)
      corner.fill(mark);
    f.motion[0].x = mark;
    f.patterns.fill((uint16_t)(i % 65536));
    exchange.publish(f);
    if (i % 37 == 0)
      std::this_thread::yield();
  }
  done.store(true, std::memory_order_release);
  reader.join();
  std::printf("EXCHANGE 50000 publications, %d coherent reader observations\n",
              frames);
  check(coherent && frames > 0, "concurrent non-parameter frames never tear");
}
struct TestPlayHead final : juce::AudioPlayHead {
  double bpm = 120, beat = 0;
  bool playing = true;
  int numerator = 4, denominator = 4;
  juce::Optional<PositionInfo> getPosition() const override {
    PositionInfo p;
    p.setBpm(bpm);
    p.setPpqPosition(beat);
    p.setIsPlaying(playing);
    p.setTimeSignature(TimeSignature{numerator, denominator});
    return p;
  }
};
void pulseTransitions();
void syncTest() {
  pulseTransitions();
  double maxPhaseError = 0, maxStoppedError = 0, maxJump = 0;
  for (double tempo : {60., 85., 120., 174.}) {
    std::vector<float> passes[2];
    for (int pass = 0; pass < 2; ++pass) {
      UndertowProcessor p;
      TestPlayHead head;
      head.bpm = tempo;
      p.setPlayHead(&head);
      set(p, ut::anchor_on, 0);
      set(p, ut::mix, 100);
      set(p, ut::drift_sync, 1);
      set(p, ut::drift_div, 11);
      set(p, ut::echo_mode, 2);
      set(p, ut::echo_div, 8);
      activatePulse(p, 1);
      set(p, ut::pulse_focus_on, 0);
      set(p, ut::pulse_position, 2);
      set(p, ut::xy_current, 3);
      prepare(p);
      juce::AudioBuffer<float> b(2, 64);
      Stats st;
      for (int block = 0; block < 3000; ++block) {
        if (block == 1000)
          head.beat -= 4;
        if (block == 1800)
          head.bpm = tempo * 1.1;
        if (block == 2300)
          head.playing = false;
        input(b, block * 64, 48000, 1);
        process(p, b);
        st.add(b);
        const double lastBeat = head.beat + 63 * head.bpm / (60 * 48000);
        if (head.playing) {
          const double pulsePhase = ut::fraction(lastBeat / ut::beats[8]);
          maxPhaseError = std::max(
              maxPhaseError,
              std::abs((double)p.engine.pulse.phases[0].load() - pulsePhase));
          maxPhaseError = std::max(maxPhaseError,
                                   std::abs((double)p.engine.driftPhase.load() -
                                            ut::fraction(lastBeat)));
          maxPhaseError =
              std::max(maxPhaseError,
                       std::abs((double)p.engine.echoPhase.load() -
                                ut::fraction(2 * lastBeat / ut::beats[8])));
        }
        const auto patch = values(p);
        ut::Timeline expected;
        expected.beat = head.beat;
        expected.bpm = head.bpm;
        const auto current =
            p.modulation.evaluate(patch, p.extraState(), expected);
        // The stopped clock advances internally rather than following the
        // host's fixed PPQ.
        if (head.playing)
          maxPhaseError = std::max(
              maxPhaseError, (double)std::abs(p.effectiveX() - current.x));
        for (int c = 0; c < 2; ++c)
          for (int n = 0; n < 64; ++n)
            passes[pass].push_back(b.getSample(c, n));
        if (head.playing)
          head.beat += 64 * head.bpm / (60 * 48000);
      }
      maxJump = std::max(maxJump, st.jump);
      check(st.safe(), "tempo / loop / stop render stays finite and bounded");
      p.setPlayHead(nullptr);
    }
    check(passes[0] == passes[1], "two bounce passes including loop and tempo "
                                  "changes are sample-identical");
    ut::Transport clock;
    clock.prepare(48000);
    const auto on = clock.begin(64, tempo, 13.25, true, true);
    const auto off = clock.begin(64, tempo, 0, false, true);
    maxStoppedError = std::max(maxStoppedError, std::abs(off.beat - on.at(64)));
  }
  std::printf("SYNC maximum canonical phase/Current error %.12g, stopped "
              "continuity %.12g, complete render maxjump %.9g\n",
              maxPhaseError, maxStoppedError, maxJump);
  check(maxJump < .05,
        "loop and tempo changes have no large discontinuity on the sine probe");
  check(maxPhaseError < 1.e-6 && maxStoppedError == 0,
        "synced phases match PPQ and stopped clock continues");
}
void pulseTransitions() {
  UndertowProcessor p;
  activatePulse(p, 6);
  set(p, ut::anchor_on, 0);
  set(p, ut::mix, 100);
  set(p, ut::echo_mode, 2);
  set(p, ut::echo_age, 100);
  prepare(p);
  juce::AudioBuffer<float> b(2, 64);
  Stats st;
  double largest = 0;
  int worstBlock = 0, worstSample = 0, worstChannel = 0;
  for (int k = 0; k < 9000; ++k) {
    if (k % 375 == 0) {
      set(p, ut::pulse_position, (float)((k / 375) % 3));
      set(p, ut::pulse_bands, (float)(1 + (k / 375) % 6));
      set(p, ut::echo_div, (float)((k / 375) % 21));
      set(p, ut::echo_sync, (float)((k / 750) % 2));
    }
    if (k % 250 == 0) {
      auto f = p.extraState();
      f.patterns[0] ^= 65535;
      f.sceneB[ut::pulse_b1_phase] = (k % 500 == 0) ? .15f : .85f;
      f.corners[0][ut::pulse_b1_depth] = (k % 500 == 0) ? .3f : -.3f;
      p.setExtraState(f);
      set(p, ut::bloom, 50);
      set(p, ut::xy_x, 0);
      set(p, ut::xy_y, 1);
      set(p, ut::pulse_b1_source, 1);
    }
    input(b, k * 64, 48000, 1);
    for (int n = 0; n < 64; ++n)
      b.setSample(1, n,
                  .2f * std::sin(juce::MathConstants<double>::twoPi * 131.3 *
                                 (k * 64 + n) / 48000));
    process(p, b);
    if (k == 375)
      for (int c = 0; c < 2; ++c)
        st.previous[c] = b.getSample(c, 63);
    if (k > 375) {
      for (int c = 0; c < 2; ++c) {
        float previous = st.previous[c];
        for (int n = 0; n < 64; ++n) {
          const double jump = std::abs(b.getSample(c, n) - previous);
          if (jump > largest) {
            largest = jump;
            worstBlock = k;
            worstSample = n;
            worstChannel = c;
          }
          previous = b.getSample(c, n);
        }
      }
      st.add(b);
    }
  }
  st.print(
      "stereo routing / reverse divisions / pattern / scene / corner changes");
  std::printf("TRANSITION largest at block %d sample %d channel %d\n",
              worstBlock, worstSample, worstChannel);
  const double ratio =
      st.jump / std::max(1.e-12, std::sqrt(st.jumpPower / st.n));
  check(st.safe() && ratio < 12, "stereo and non-parameter transitions remain "
                                 "bounded without large jumps");
}
void pulseTest() {
  UndertowProcessor p;
  auto v = values(p);
  auto frame = p.extraState();
  v[ut::pulse_on] = 1;
  v[ut::pulse_bands] = 6;
  for (int i = 0; i < 5; ++i)
    v[(size_t)(ut::pulse_xover1 + i)] = 16000;
  const auto ordered = ut::Pulse::orderedCrossovers(v, 44100);
  bool order = ordered[0] >= 20 && ordered[4] <= 16000;
  for (int i = 1; i < 5; ++i)
    order &= ordered[(size_t)i] - ordered[(size_t)(i - 1)] >= 20;
  check(order,
        "crossovers ordered with 20 Hz minimum gap, parameters untouched");
  for (int bands = 1; bands <= 6; ++bands) {
    for (double hz : {40., 200., 800., 2500., 6000., 12000.}) {
      v = values(p);
      v[ut::pulse_on] = 1;
      v[ut::pulse_bands] = (float)bands;
      ut::Pulse pulse;
      pulse.prepare(48000, 64);
      for (int band = 0; band < 6; ++band) {
        const int base = ut::pulse_b1_sync + 7 * band;
        v[(size_t)base] = 0;
        v[(size_t)(base + 2)] = .05f;
        v[(size_t)(base + 3)] = 100;
        v[(size_t)(base + 4)] = 3;
      }
      double inputPower = 0, outputPower = 0;
      ut::Timeline t;
      t.increment = 120. / (60 * 48000);
      for (int block = 0; block < 1500; ++block) {
        t.beat = block * 64 * t.increment;
        pulse.begin(v, frame, t);
        for (int n = 0; n < 64; ++n) {
          pulse.tick(v, n);
          const float in =
              .1f * (float)std::sin(juce::MathConstants<double>::twoPi * hz *
                                    (block * 64 + n) / 48000);
          float pair[]{in, in};
          pulse.apply(pair);
          if (block > 750) {
            inputPower += in * in;
            outputPower += pair[0] * pair[0];
          }
        }
      }
      const double db = 10 * std::log10(outputPower / inputPower);
      std::printf("PULSE FLAT %d bands %.0f Hz %.9g dB\n", bands, hz, db);
      check(std::abs(db) < .025, "phase-compensated band sum flat");
    }
  }
  // Depth-zero and disabled paths do not even apply crossover phase.
  ut::Pulse bypass;
  bypass.prepare(48000, 64);
  v = values(p);
  v[ut::pulse_on] = 1;
  ut::Timeline time;
  bypass.begin(v, frame, time);
  bool exact = true;
  for (int n = 0; n < 1000; ++n) {
    bypass.tick(v, n);
    float pair[]{.1f, -.2f};
    bypass.apply(pair);
    exact &= pair[0] == .1f && pair[1] == -.2f;
  }
  check(exact, "all zero depths bypass PULSE bit-for-bit");
  v[ut::pulse_focus_on] = 1;
  v[ut::pulse_focus_depth] = 0;
  bypass.begin(v, frame, time);
  float zeroFocus[]{.123f, -.234f};
  bypass.tick(v, 0);
  bypass.apply(zeroFocus);
  check(zeroFocus[0] == .123f && zeroFocus[1] == -.234f,
        "focus depth zero is exactly neutral");
  ut::Pulse sub;
  sub.prepare(48000, 64);
  v = values(p);
  v[ut::pulse_on] = 1;
  v[ut::pulse_sub] = 1;
  v[ut::pulse_bands] = 1;
  v[ut::pulse_b1_depth] = 100;
  v[ut::pulse_b1_shape] = 3;
  v[ut::pulse_b1_div] = 11;
  time.beat = .75;
  time.increment = 0;
  sub.begin(v, frame, time);
  for (int n = 0; n < 5000; ++n)
    sub.tick(v, n);
  v[ut::pulse_b1_depth] = 0;
  sub.begin(v, frame, time);
  float last = sub.subGain(), maxSubStep = 0;
  for (int n = 0; n < 2000; ++n) {
    sub.tick(v, n);
    const float next = sub.subGain();
    maxSubStep = std::max(maxSubStep, std::abs(next - last));
    last = next;
  }
  std::printf("SUB depth-zero transition largest gain step %.9g final %.9g\n",
              maxSubStep, last);
  check(maxSubStep < .02f && last == 1,
        "pulse_sub fades to unity when depths become zero");
  v = values(p);
  v[ut::pulse_b1_source] = 1;
  v[ut::pulse_b1_phase] = 360;
  const double rotated = ut::Pulse::cycles(0, v, 0, 0);
  v[ut::pulse_b1_phase] = 0;
  const double plain = ut::Pulse::cycles(0, v, 0, 0);
  check(((int)std::floor(rotated) % 16) == ((int)std::floor(plain) % 16),
        "360 degree step phase rotates one complete pattern");
  UndertowProcessor off, on;
  set(on, ut::pulse_on, 1);
  set(on, ut::pulse_focus_on, 1);
  prepare(off);
  prepare(on);
  juce::AudioBuffer<float> a(2, 64), b(2, 64);
  bool neutralAudio = true;
  for (int k = 0; k < 1500; ++k) {
    input(a, k * 64, 48000, 2);
    b.makeCopyOf(a);
    process(off, a);
    process(on, b);
    for (int c = 0; c < 2; ++c)
      neutralAudio &= std::memcmp(a.getReadPointer(c), b.getReadPointer(c),
                                  64 * sizeof(float)) == 0;
  }
  check(neutralAudio, "zero-depth PULSE/focus equals disabled stages in "
                      "complete processor audio");
  // A root at each octave and an off-note probe quantify focus rather than
  // relying on a clipper.
  double maxBoost = -100, minBoost = 100;
  for (double hz : {36.70809599, 73.41619198, 146.83238396, 293.6647679,
                    587.3295358, 1000.}) {
    ut::Pulse focus;
    focus.prepare(48000, 64);
    v = values(p);
    v[ut::pulse_on] = 1;
    v[ut::pulse_focus_on] = 1;
    v[ut::pulse_focus_depth] = 100;
    v[ut::pulse_focus_harm] = 8;
    v[ut::pulse_focus_q] = 60;
    v[ut::pulse_focus_div] = 20;
    double inPower = 0, outPower = 0;
    ut::Timeline t;
    t.increment = 0;
    t.beat = 16; // centre of a very slow focus pulse
    for (int block = 0; block < 3000; ++block) {
      focus.begin(v, frame, t);
      for (int n = 0; n < 64; ++n) {
        focus.tick(v, n);
        const float x =
            .03f * (float)std::sin(juce::MathConstants<double>::twoPi * hz *
                                   (block * 64 + n) / 48000);
        float pair[]{x, x};
        focus.apply(pair);
        if (block > 2000) {
          inPower += x * x;
          outPower += pair[0] * pair[0];
        }
      }
    }
    const double db = 10 * std::log10(outPower / inPower);
    maxBoost = std::max(maxBoost, db);
    minBoost = std::min(minBoost, db);
    std::printf("FOCUS %.6f Hz gain %.6f dB\n", hz, db);
  }
  check(maxBoost > 0 && maxBoost <= 12.0001,
        "focus boosts selected harmonics without exceeding +12 dB");
  ut::Pulse changing;
  changing.prepare(48000, 64);
  v = values(p);
  v[ut::pulse_on] = 1;
  v[ut::pulse_focus_on] = 1;
  v[ut::pulse_focus_depth] = 100;
  v[ut::pulse_focus_harm] = 8;
  v[ut::pulse_focus_div] = 20;
  double dynamicPeak = 0;
  for (int block = 0; block < 4500; ++block) {
    v[ut::pulse_focus_note] = (float)((block / 375) % 12);
    v[ut::pulse_focus_q] = (block / 125) % 2 ? 1.f : 60.f;
    ut::Timeline t;
    t.beat = 16;
    t.increment = 0;
    changing.begin(v, frame, t);
    for (int n = 0; n < 64; ++n) {
      changing.tick(v, n);
      const float x =
          .03f * (float)std::sin(juce::MathConstants<double>::twoPi *
                                 73.41619198 * (block * 64 + n) / 48000);
      float pair[]{x, x};
      changing.apply(pair);
      dynamicPeak = std::max(dynamicPeak, (double)std::abs(pair[0]));
    }
  }
  std::printf("FOCUS changing note/Q peak %.9g, bound %.9g\n", dynamicPeak,
              .03 * 3.981071706);
  check(dynamicPeak <= .03 * 3.981071706 + 1.e-6,
        "focus transient bound is +12 dB above recent input peak");
  // Sixteen step clocks are one division per step, with smooth transitions.
  ut::Pulse steps;
  steps.prepare(48000, 64);
  v = values(p);
  v[ut::pulse_on] = 1;
  v[ut::pulse_bands] = 1;
  v[ut::pulse_b1_depth] = 100;
  v[ut::pulse_b1_source] = 1;
  v[ut::pulse_b1_div] = 11;
  frame.patterns[0] = 0xaaaa;
  double low = 0, high = 0;
  int lowN = 0, highN = 0;
  for (int block = 0; block < 6000; ++block) {
    ut::Timeline t;
    t.beat = block * 64 / 24000.;
    steps.begin(v, frame, t);
    for (int n = 0; n < 64; ++n) {
      steps.tick(v, n);
      float pair[]{.1f, .1f};
      steps.apply(pair);
      const double beat = t.at(n);
      if (ut::fraction(beat) > .5) {
        if (((int)std::floor(beat) & 1) == 0) {
          low += pair[0] * pair[0];
          ++lowN;
        } else {
          high += pair[0] * pair[0];
          ++highN;
        }
      }
    }
  }
  std::printf("STEPS off rms %.9g on rms %.9g\n", std::sqrt(low / lowN),
              std::sqrt(high / highN));
  check(low / lowN < 1.e-8 && high / highN > .009,
        "stored step bits control successive divisions");
}
