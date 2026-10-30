// Interface, factory bank, gestures and text. Included by Render.cpp.

juce::MouseEvent mouse(juce::Component &c, juce::Point<float> pos, int clicks = 1) {
  return juce::MouseEvent(juce::Desktop::getInstance().getMainMouseSource(), pos,
                          juce::ModifierKeys::leftButtonModifier, 1, 0, 0, 0, 0, &c, &c,
                          juce::Time::getCurrentTime(), pos, juce::Time::getCurrentTime(), clicks,
                          false);
}
void feed(UndertowProcessor &p, int blocks, int kind = 2, int64_t *at = nullptr) {
  static int64_t position = 0;
  int64_t &pos = at ? *at : position;
  juce::AudioBuffer<float> b(2, 256);
  for (int k = 0; k < blocks; ++k) {
    input(b, pos, 48000, kind);
    juce::MidiBuffer m;
    p.processBlock(b, m);
    pos += 256;
  }
}
std::unique_ptr<UndertowEditor> openEditor(UndertowProcessor &p) {
  std::unique_ptr<UndertowEditor> ed(dynamic_cast<UndertowEditor *>(p.createEditor()));
  ed->setSize(1280, 800);
  return ed;
}
void savePng(const juce::Image &img, const juce::File &f) {
  f.deleteFile();
  juce::FileOutputStream os(f);
  juce::PNGImageFormat().writeImageToStream(img, os);
  std::printf("wrote %s (%dx%d)\n", f.getFileName().toRawUTF8(), img.getWidth(), img.getHeight());
}

void uiTest(const juce::File &dir) {
  dir.createDirectory();
  UndertowProcessor p;
  p.setPlayConfigDetails(2, 2, 48000, 256);
  p.prepareToPlay(48000, 256);
  p.setCurrentProgram(11); // Wander Field: corners, pulse and Current all visible
  feed(p, 200, 2);
  auto ed = openEditor(p);
  auto &panel = ed->getPanel();
  auto settle = [&](int frames, int kind = 2) {
    for (int k = 0; k < frames; ++k) {
      feed(p, 6, kind);
      ed->tickForTest();
    }
  };
  auto shot = [&](const juce::String &name, float scale = 1.0f) {
    savePng(ed->createComponentSnapshot(ed->getLocalBounds(), true, scale), dir.getChildFile(name));
  };
  settle(40);
  shot("ui-main.png");
  shot("ui-main@2x.png", 2.0f);
  set(p, ut::void_freeze, 1);
  settle(10);
  shot("ui-freeze.png");
  set(p, ut::void_freeze, 0);
  p.setCurrentProgram(8); // Signal Lamp: steps on bands 2 and 3
  panel.selectBand(1);
  settle(30);
  shot("ui-steps.png");
  // MUTATE page: FREEZE stays, the VOID controls hide, the display moves
  p.setCurrentProgram(15); // Living Freeze
  panel.showPage(1);
  settle(80);
  shot("ui-mutate.png");
  {
    int voidShowing = 0, mutShowing = 0;
    for (auto *c : panel.allControls()) {
      const juce::String id = c->parameter().getParameterID();
      if (id.startsWith("void_") && id != "void_freeze") voidShowing += c->isVisible();
      if (id.startsWith("harm_") || id.startsWith("halo_") || id.startsWith("evolve_") || id == "mut_on") mutShowing += c->isVisible();
      if (id == "void_freeze") check(c->isVisible(), "FREEZE is on the MUTATE page");
    }
    std::printf("  MUTATE page: %d mutate controls showing, %d VOID controls showing\n", mutShowing, voidShowing);
    check(voidShowing == 0 && mutShowing == 14, "the MUTATE page shows its 14 controls and hides VOID's");
    check((int)p.state.state.getProperty("rightPage", 0) == 1, "the open page is kept with the project");
  }
  panel.showPage(0);
  p.setCurrentProgram(8);
  settle(10);
  panel.setEditB(true);
  settle(3);
  shot("ui-edit-b.png");
  panel.setEditB(false);
  panel.learnCorner(0);
  settle(3);
  shot("ui-learn.png");
  panel.learnCorner(0);
  p.setCurrentProgram(0);
  settle(20);
  shot("ui-default.png");
  for (auto [w, h, name] : {std::tuple{960, 600, "ui-75.png"}, std::tuple{2560, 1600, "ui-200.png"}}) {
    ed->setSize(w, h);
    settle(2);
    shot(name);
  }
  ed->setSize(1280, 800);
  check(p.extraState().width == 1280 && p.extraState().height == 800, "editor size stored in the project state");

  // ---- cost
  p.setCurrentProgram(11);
  settle(20);
  auto timeIt = [&](juce::Rectangle<int> area, int runs, bool withTick) {
    ed->createComponentSnapshot(area, true, 2.0f);
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    for (int k = 0; k < runs; ++k) {
      feed(p, 6, 2);
      const auto a = juce::Time::getMillisecondCounterHiRes();
      if (withTick)
        ed->tickForTest();
      ed->createComponentSnapshot(area, true, 2.0f);
      (void)a;
    }
    return (juce::Time::getMillisecondCounterHiRes() - t0) / runs;
  };
  // audio feeding cost per frame, measured separately and subtracted
  const auto f0 = juce::Time::getMillisecondCounterHiRes();
  for (int k = 0; k < 40; ++k)
    feed(p, 6, 2);
  const double audioMs = (juce::Time::getMillisecondCounterHiRes() - f0) / 40;
  const double full = timeIt(ed->getLocalBounds(), 10, true) - audioMs;
  std::printf("Full-window repaint at 2x (resize, preset change): %.2f ms\n", full);
  // Per frame while playing: the spectrum, the pad, the meters and (in STEPS) the grid.
  // The tail view and the knobs repaint only when a value changes.
  struct Area { const char *name; juce::Rectangle<int> r; bool everyFrame; };
  double perFrame = 0;
  for (const Area &a : {Area{"spectrum (15 fps)", {258, 540, 382, 146}, true}, Area{"XY pad", {500, 90, 280, 280}, true},
                        Area{"meters", {1150, 710, 108, 78}, true}, Area{"step grid", {866, 608, 148, 70}, true},
                        Area{"tail view", {892, 396, 364, 112}, false}, Area{"one knob", {896, 208, 70, 80}, false}}) {
    const double ms = std::max(0.0, timeIt(a.r, 40, false) - audioMs);
    if (a.everyFrame) perFrame += juce::String(a.name).contains("15 fps") ? ms * .5 : ms;
    std::printf("  %-12s repaint at 2x: %.3f ms%s\n", a.name, ms, a.everyFrame ? "  (every frame)" : "  (on change)");
  }
  double tickMs = 0;
  for (int k = 0; k < 60; ++k) {
    feed(p, 6, 2); // fresh audio each frame, so the FFT runs
    const auto t0 = juce::Time::getMillisecondCounterHiRes();
    ed->tickForTest();
    tickMs += juce::Time::getMillisecondCounterHiRes() - t0;
  }
  tickMs /= 60;
  perFrame += tickMs;
  const double core = perFrame * 30.0 / 1000.0 * 100.0;
  std::printf("Tick (FFT, meters, all controls) %.3f ms; worst-case frame %.3f ms; at 30 fps %.2f %% of one core\n",
              tickMs, perFrame, core);
  check(core < 5.0, "editor under 5 % of one core while playing (software renderer upper bound)");

  // ---- idle: let the default 8 s reverb tail die completely, then nothing may repaint
  p.setCurrentProgram(0);
  for (int k = 0; k < 900; ++k) {
    feed(p, 6, 5); // 30 s of processed silence
    ed->tickForTest();
  }
  std::map<juce::String, int> log;
  ui::repaintLog = &log;
  ui::repaintRequests = 0;
  for (int k = 0; k < 90; ++k) {
    feed(p, 6, 5);
    ed->tickForTest();
  }
  std::printf("Repaint requests over 3 s of processed silence: %d\n", ui::repaintRequests);
  for (auto &[who, n] : log) std::printf("  %s %d\n", who.toRawUTF8(), n);
  check(ui::repaintRequests == 0, "editor stops repainting while processing silence");
  for (int k = 0; k < 60; ++k)
    ed->tickForTest(); // transport stopped: the spectrum falls away
  log.clear();
  ui::repaintRequests = 0;
  for (int k = 0; k < 90; ++k)
    ed->tickForTest();
  std::printf("Repaint requests over 3 s with no audio: %d\n", ui::repaintRequests);
  for (auto &[who, n] : log) std::printf("  %s %d\n", who.toRawUTF8(), n);
  check(ui::repaintRequests == 0, "editor stops repainting when idle");
  ui::repaintLog = nullptr;
  ed.reset();
  check(!ui::sceneBEditing && ui::cornerLearning < 0, "editor modes reset when the editor closes");
}

void gesturesTest() {
  struct Recorder : juce::AudioProcessorParameter::Listener {
    std::vector<std::pair<int, char>> events;
    void parameterValueChanged(int i, float) override { events.emplace_back(i, 'V'); }
    void parameterGestureChanged(int i, bool start) override { events.emplace_back(i, start ? 'B' : 'E'); }
  } record;
  UndertowProcessor p;
  p.setPlayConfigDetails(2, 2, 48000, 256);
  p.prepareToPlay(48000, 256);
  feed(p, 20);
  for (auto *v : p.getParameters())
    v->addListener(&record);
  auto verify = [&](const char *name, std::initializer_list<int> ids, auto action, bool gestures = true) {
    record.events.clear();
    action();
    bool ok = true;
    int values = 0, begins = 0, ends = 0;
    for (int index : ids) {
      int depth = 0, count = 0, b = 0, e = 0;
      for (auto [i, type] : record.events)
        if (i == p.parameter(index).getParameterIndex()) {
          if (type == 'B') { ok = ok && depth == 0; ++depth; ++b; }
          if (type == 'E') { ok = ok && depth == 1; --depth; ++e; }
          if (type == 'V') { ok = ok && (!gestures || depth == 1); ++count; }
        }
      ok = ok && depth == 0 && count > 0 && (!gestures || (b > 0 && b == e)) && b <= 1;
      values += count; begins += b; ends += e;
    }
    std::printf("%-34s begin %d value %d end %d %s\n", name, begins, values, ends, ok ? "PASS" : "FAIL");
    if (!ok) ++failures;
  };
  ui::Knob knob(p, "void_decay", "DECAY");
  knob.setSize(80, 100);
  verify("Knob drag", {ut::void_decay}, [&] {
    knob.mouseDown(mouse(knob, {40, 60}));
    knob.mouseDrag(mouse(knob, {40, 20}));
    knob.mouseUp(mouse(knob, {40, 20}));
  });
  check(p.undoManager().canUndo(), "knob drag is undoable");
  const float moved = p.parameter(ut::void_decay).getValue();
  p.undoManager().undo();
  check(std::abs(p.parameter(ut::void_decay).getValue() - p.parameter(ut::void_decay).getDefaultValue()) < 1.e-6f &&
            moved != p.parameter(ut::void_decay).getDefaultValue(),
        "undo restores the knob");
  ui::Segments seg(p, "echo_mode", "");
  seg.setSize(240, 28);
  verify("Segments click", {ut::echo_mode}, [&] { seg.mouseDown(mouse(seg, {200, 14})); });
  ui::ListSelector list(p, "echo_div", "TIME");
  list.setSize(100, 40);
  verify("ListSelector step", {ut::echo_div}, [&] { list.mouseDown(mouse(list, {95, 25})); });
  ui::Switch sw(p, "pulse_on", "ON");
  sw.setSize(70, 20);
  verify("Switch", {ut::pulse_on}, [&] { sw.mouseDown(mouse(sw, {8, 10})); });
  ui::BarSlider bar(p, "bloom", "BLOOM");
  bar.setSize(196, 32);
  verify("BLOOM bar drag", {ut::bloom}, [&] {
    bar.mouseDown(mouse(bar, {80, 16}));
    bar.mouseDrag(mouse(bar, {120, 16}));
    bar.mouseUp(mouse(bar, {120, 16}));
  });
  ui::FreezeButton fr(p);
  fr.setSize(100, 92);
  verify("FREEZE click (latch)", {ut::void_freeze}, [&] { fr.mouseDown(mouse(fr, {50, 46})); fr.mouseUp(mouse(fr, {50, 46})); });
  check(p.parameter(ut::void_freeze).getValue() > .5f, "FREEZE latches on");
  fr.mouseDown(mouse(fr, {50, 46}));
  p.state.state.setProperty("freezeHold", true, nullptr);
  verify("FREEZE hold (momentary)", {ut::void_freeze}, [&] { fr.mouseDown(mouse(fr, {50, 46})); fr.mouseUp(mouse(fr, {50, 46})); });
  check(p.parameter(ut::void_freeze).getValue() < .5f, "FREEZE hold releases");
  p.state.state.setProperty("freezeHold", false, nullptr);

  ui::XYPad pad(p);
  pad.setSize(280, 280);
  verify("XY pad drag", {ut::xy_x, ut::xy_y}, [&] {
    pad.mouseDown(mouse(pad, {140, 140}));
    pad.mouseDrag(mouse(pad, {240, 40}));
    pad.mouseUp(mouse(pad, {240, 40}));
  });
  check(p.parameter(ut::xy_x).getValue() > .8f && p.parameter(ut::xy_y).getValue() > .8f, "pad up-right moves x and y up");
  verify("XY pad double-click centres", {ut::xy_x, ut::xy_y}, [&] { pad.mouseDoubleClick(mouse(pad, {10, 10}, 2)); });
  {
    set(p, ut::xy_return, 1);
    verify("XY Return glides in one gesture", {ut::xy_x, ut::xy_y}, [&] {
      pad.mouseDown(mouse(pad, {140, 140}));
      pad.mouseDrag(mouse(pad, {30, 250}));
      pad.mouseUp(mouse(pad, {30, 250}));
      const auto start = juce::Time::getMillisecondCounterHiRes();
      while (pad.gliding() && juce::Time::getMillisecondCounterHiRes() - start < 12000) {
        feed(p, 2);
        pad.tick(p.extraState(), false);
        juce::Thread::sleep(15);
      }
    });
    check(std::abs(p.parameter(ut::xy_x).getValue() - .5f) < 1.e-4f && std::abs(p.parameter(ut::xy_y).getValue() - .5f) < 1.e-4f,
          "Return ends exactly at the centre");
    set(p, ut::xy_return, 0);
  }
  set(p, ut::pulse_on, 1);
  set(p, ut::pulse_bands, 3);
  feed(p, 4);
  ui::Spectrum spec(p);
  spec.setBounds(0, 0, 382, 146);
  const auto xs = p.engine.pulse.crossoverTargets[0].load();
  const float lineX = 6 + 370 * std::log(xs / 20.f) / std::log(1000.f);
  verify("Spectrum crossover drag", {ut::pulse_xover1}, [&] {
    spec.mouseDown(mouse(spec, {lineX, 80}));
    spec.mouseDrag(mouse(spec, {lineX + 40, 80}));
    spec.mouseUp(mouse(spec, {lineX + 40, 80}));
  });
  check(p.parameter(ut::pulse_xover1).convertFrom0to1(p.parameter(ut::pulse_xover1).getValue()) > xs * 1.5f,
        "dragging right raises the crossover");

  // Scene B editing: knob writes Scene B, never the host parameter.
  {
    ui::sceneBEditing = true;
    ui::Knob bKnob(p, "void_size", "SIZE");
    bKnob.setSize(80, 100);
    const float hostBefore = p.parameter(ut::void_size).getValue(), bBefore = p.sceneBValue(ut::void_size);
    record.events.clear();
    bKnob.mouseDown(mouse(bKnob, {40, 60}));
    bKnob.mouseDrag(mouse(bKnob, {40, 10}));
    bKnob.mouseUp(mouse(bKnob, {40, 10}));
    bool hostTouched = false;
    for (auto [i, t] : record.events) hostTouched |= i == ut::void_size;
    check(!hostTouched && p.parameter(ut::void_size).getValue() == hostBefore && p.sceneBValue(ut::void_size) > bBefore,
          "EDIT B knob changes Scene B only");
    p.undoManager().undo();
    check(std::abs(p.sceneBValue(ut::void_size) - bBefore) < 1.e-6f, "undo restores the Scene B value");
    ui::sceneBEditing = false;
  }
  // Scene actions and presets reach the host as gestures.
  {
    auto next = p.extraState();
    next.sceneB[ut::void_decay] = .9f;
    p.setExtraState(next);
    verify("Scene B>A writes parameters", {ut::void_decay}, [&] { p.copySceneBToA(); });
    verify("Preset load writes parameters", {ut::echo_on, ut::void_decay}, [&] { p.setCurrentProgram(7); });
  }
  // Editor-level tools: learn, clear, randomise, steps, record; each one undo step.
  {
    p.setCurrentProgram(0);
    p.undoManager().clearUndoHistory();
    auto ed = openEditor(p);
    auto &panel = ed->getPanel();
    const auto paramsBefore = p.normalizedValues();
    panel.learnCorner(2);
    set(p, ut::void_decay, 40);
    set(p, ut::echo_feedback, 80);
    panel.learnCorner(2);
    const auto after = p.extraState();
    check(after.corners[2][ut::void_decay] > .1f && after.corners[2][ut::echo_feedback] > .1f &&
              after.corners[2][ut::void_size] == 0,
          "LEARN stores exactly the knobs that moved");
    bool restored = true;
    const auto now = p.normalizedValues();
    for (size_t k = 0; k < ut::count; ++k) restored &= std::abs(now[k] - paramsBefore[k]) < 1.e-6f;
    check(restored, "LEARN puts the knobs back where they were");
    p.undoManager().undo();
    check(p.extraState().corners[2][ut::void_decay] == 0 && !p.undoManager().canUndo(), "LEARN is one undo step");
    set(p, ut::pulse_on, 0); // same pulse setup as the fresh processor below
    panel.randomiseCorner(1);
    const auto r1 = p.extraState().corners[1];
    int nonzero = 0;
    for (float v : r1) nonzero += v != 0;
    check(nonzero >= 2, "RND sets a small variation");
    UndertowProcessor q;
    auto ed2 = openEditor(q);
    ed2->getPanel().randomiseCorner(1);
    check(q.extraState().corners[1] == r1, "RND is deterministic");
    panel.clearCorner(1);
    const auto cleared = p.extraState();
    bool empty = true;
    for (float v : cleared.corners[1]) empty &= v == 0;
    check(empty, "CLR empties the corner");
    // steps
    const auto pat = p.extraState().patterns[3];
    ui::StepGrid grid(p);
    grid.setBounds(0, 0, 160, 40);
    grid.band = 3;
    grid.mouseDown(mouse(grid, {5, 5}));
    check(p.extraState().patterns[3] == (uint16_t)(pat ^ 1u), "step click toggles step 1");
    p.undoManager().undo();
    check(p.extraState().patterns[3] == pat, "step toggle is undoable");
    // recording: two bars of a square path while the transport runs freely
    set(p, ut::xy_motion_bars, 0); // 1 bar
    panel.toggleRecord();
    int frames = 0;
    while (panel.isRecording() && frames < 400) {
      const float t = (float)frames / 60.f;
      set(p, ut::xy_x, .5f + .4f * std::cos(t * 6.f));
      set(p, ut::xy_y, .5f + .4f * std::sin(t * 6.f));
      feed(p, 6, 5);
      ed->tickForTest();
      ++frames;
    }
    const auto rec = p.extraState();
    std::printf("Recorded %d points in %d frames\n", rec.motionCount, frames);
    check(!panel.isRecording() && rec.motionCount > 20 && p.parameter(ut::xy_motion_play).getValue() > .5f,
          "REC captures one cycle and starts playback");
    bool sorted = true;
    for (int k = 1; k < rec.motionCount; ++k) sorted &= rec.motion[(size_t)k].bar > rec.motion[(size_t)k - 1].bar;
    check(sorted && rec.motion[(size_t)rec.motionCount - 1].bar < 1.f && frames < 90,
          "a one-bar recording stops after one bar, ordered within the loop");
    ed.reset();
    ed2.reset();
  }
  for (auto *v : p.getParameters())
    v->removeListener(&record);
}

void bankTest(const juce::File &dir) {
  dir.createDirectory();
  UndertowProcessor p;
  const auto &bank = ut::factoryPresets();
  check(p.getNumPrograms() == 80 && (int)bank.size() == 80, "eighty factory presets");
  std::set<juce::String> names;
  for (int i = 0; i < p.getNumPrograms(); ++i) {
    const auto n = p.getProgramName(i);
    check(n.contains(" / ") && names.insert(n).second, ("named and unique: " + n).toRawUTF8());
  }
  const char *expected[] {"INIT / Default", "BASS SPACE / Low Ceiling", "BASS SPACE / Iron Hall", "BASS SPACE / Undercurrent",
                          "DRONE / Salt Room", "DRONE / Tide Without Moon", "DRONE / Long Breath", "PULSE / Breathing Walls",
                          "PULSE / Signal Lamp", "PULSE / Focus in D", "MOTION / Drift Engine", "MOTION / Wander Field",
                          "MUTATE / Glass Choir", "MUTATE / Rust Organ", "MUTATE / Sinking Cathedral", "MUTATE / Living Freeze",
                          "RIOT PAIRS / Snarl Bass", "RIOT PAIRS / Rust Drone", "RIOT PAIRS / Low Orbit", "RIOT PAIRS / Glass Moths", "RIOT PAIRS / Abandoned Chamber", "RIOT PAIRS / Black Reservoir", "RIOT PAIRS / Hammer-On Lead", "RIOT PAIRS / Bowed Cable Drone", "RIOT PAIRS / Sodium Horizon", "RIOT PAIRS / Swarm Choir", "RIOT PAIRS / Mantle Bloom", "RIOT PAIRS / Tidal Armature", "RIOT PAIRS / Relay Cathedral", "RIOT PAIRS / Nightwell Vapour", "RIOT PAIRS / Faultline Halo", "RIOT PAIRS / Ghosts in the Busbar",
                          "CINEMATIC / Long Corridor",
                          "CINEMATIC / Ember Field",
                          "CINEMATIC / Cold Open",
                          "CINEMATIC / Slow Reveal",
                          "CINEMATIC / Last Light",
                          "CINEMATIC / Night Exterior",
                          "CINEMATIC / Underscore",
                          "CINEMATIC / Title Card",
                          "SHIMMER / Ascent Glass",
                          "SHIMMER / Choir Loft",
                          "SHIMMER / Prism Tail",
                          "SHIMMER / Upper Air",
                          "SHIMMER / Octave Rain",
                          "SHIMMER / Fifth Heaven",
                          "FROZEN / Ice Shelf",
                          "FROZEN / Held Breath",
                          "FROZEN / Still Water",
                          "FROZEN / Permafrost",
                          "FROZEN / Glass Lung",
                          "FROZEN / Suspended",
                          "RHYTHMIC / Tremor Grid",
                          "RHYTHMIC / Heartline",
                          "RHYTHMIC / Sidechain Tide",
                          "RHYTHMIC / Gated Cathedral",
                          "RHYTHMIC / Step Ghosts",
                          "RHYTHMIC / Dotted Machine",
                          "RHYTHMIC / Triplet Rain",
                          "RHYTHMIC / Morse Bloom",
                          "ECHOES / Tape Memory",
                          "ECHOES / Ping Field",
                          "ECHOES / Backwards Rain",
                          "ECHOES / Dub Siren",
                          "ECHOES / Worn Cassette",
                          "ECHOES / Long Return",
                          "TEXTURE / Rust Bloom",
                          "TEXTURE / Static Garden",
                          "TEXTURE / Folded Paper",
                          "TEXTURE / Dead Air",
                          "TEXTURE / Copper Air",
                          "TEXTURE / Dust Halo",
                          "BASS SPACE / Concrete Floor",
                          "BASS SPACE / Sub Cavern",
                          "BASS SPACE / Tight Room",
                          "BASS SPACE / Riff Halo",
                          "DRONE / Engine Hum",
                          "DRONE / Distant Choir",
                          "DRONE / Tidal Hum",
                          "DRONE / Mountain"};
  for (int i = 0; i < 80; ++i)
    check(p.getProgramName(i) == expected[i], "stable preset index");
  juce::AudioFormatManager formats;
  formats.registerBasicFormats();
  const auto source = testFile("UNDERTOW_TEST_RIFF");
  std::unique_ptr<juce::AudioFormatReader> r(formats.createReaderFor(source));
  juce::AudioBuffer<float> riff(2, 48000 * 6);
  riff.clear();
  if (r) r->read(&riff, 0, std::min<int>(riff.getNumSamples(), (int)r->lengthInSamples), 0, true, true);
  std::printf("Source: %s\n", r ? source.getFileName().toRawUTF8() : "synthetic bass riff");
  for (int i = 0; i < 80; ++i) {
    UndertowProcessor q;
    q.setPlayConfigDetails(2, 2, 48000, 256);
    q.prepareToPlay(48000, 256);
    q.setCurrentProgram(i);
    check(q.getCurrentProgram() == i && !q.isEdited(), "loads clean (not EDITED)");
    const auto e = q.extraState();
    int differs = 0, corners = 0;
    const auto a = q.normalizedValues();
    for (int k = 0; k < ut::count; ++k) differs += ut::snapshotParameter(k) && std::abs(e.sceneB[(size_t)k] - a[(size_t)k]) > 1.e-4f;
    for (auto &c : e.corners) for (float v : c) corners += v != 0;
    if (i > 0) check(differs >= 2 && corners >= 4, "Scene B and corners are set");
    // 6 s of the riff, then 6 s of tail; then the same sweeping BLOOM and the puck
    juce::AudioBuffer<float> out(2, 48000 * 12), b(2, 256);
    out.clear();
    int64_t pos = 0;
    for (int at = 0; at < out.getNumSamples(); at += 256) {
      const int n = std::min(256, out.getNumSamples() - at);
      b.setSize(2, n, false, false, true);
      b.clear();
      if (at < riff.getNumSamples()) {
        if (r) for (int ch = 0; ch < 2; ++ch) b.copyFrom(ch, 0, riff, ch, at, std::min(n, riff.getNumSamples() - at));
        else input(b, pos, 48000, 2);
      }
      if (i > 0) {
        set(q, ut::bloom, 100.f * (float)at / (float)out.getNumSamples());
        set(q, ut::xy_x, .5f + .45f * std::sin(at / 48000.f));
        set(q, ut::xy_y, .5f + .45f * std::cos(at / 61000.f));
      }
      process(q, b);
      for (int ch = 0; ch < 2; ++ch) out.copyFrom(ch, at, b, ch, 0, n);
      pos += n;
    }
    Stats st;
    st.add(out);
    st.print(q.getProgramName(i).toRawUTF8());
    check(st.safe() && st.peak < .98, "bounded through BLOOM and the pad");
    juce::WavAudioFormat wav;
    auto file = dir.getChildFile(juce::String(i).paddedLeft('0', 2) + " " + q.getProgramName(i).replace(" / ", " - ") + ".wav");
    file.deleteFile();
    std::unique_ptr<juce::AudioFormatWriter> w(wav.createWriterFor(new juce::FileOutputStream(file), 48000, 2, 24, {}, 0));
    if (w) w->writeFromAudioSampleBuffer(out, 0, out.getNumSamples());
  }
  // EDITED and state
  p.setCurrentProgram(4);
  check(!p.isEdited(), "fresh preset is not EDITED");
  set(p, ut::void_decay, 12);
  check(p.isEdited(), "moving a knob marks EDITED");
  juce::MemoryBlock m;
  p.getStateInformation(m);
  UndertowProcessor r2;
  r2.setStateInformation(m.getData(), (int)m.getSize());
  check(r2.getCurrentProgram() == 4 && r2.isEdited(), "preset index and EDITED survive save/load");
  auto keep = p.extraState();
  keep.width = 1600; keep.height = 1000;
  set(p, ut::xy_return, 1);
  p.setExtraState(keep);
  p.setCurrentProgram(1);
  check(p.extraState().width == 1600 && p.parameter(ut::xy_return).getValue() > .5f && !p.isEdited(),
        "preset load keeps the window size and RETURN, and RETURN doesn't count as an edit");
}

void textTest() {
  UndertowProcessor p;
  int fails = 0, checked = 0;
  for (int k = 0; k < ut::count; ++k) {
    if (ut::specs[(size_t)k].kind != 0) continue;
    auto &param = p.parameter(k);
    for (float n : {0.f, .25f, .5f, .75f, 1.f}) {
      const auto text = param.getText(n, 0);
      const float back = param.getValueForText(text);
      ++checked;
      if (param.getText(back, 0) != text) {
        ++fails;
        std::printf("  %s: '%s' -> '%s'\n", ut::specs[(size_t)k].id, text.toRawUTF8(), param.getText(back, 0).toRawUTF8());
      }
    }
  }
  std::printf("Display text round trips: %d of %d\n", checked - fails, checked);
  check(fails == 0, "every display string types back to itself");
  auto &freq = p.parameter(ut::echo_tone);
  check(std::abs(freq.convertFrom0to1(freq.getValueForText("2.5k")) - 2500) < 1, "'2.5k' types as 2500 Hz");
  check(std::abs(freq.convertFrom0to1(freq.getValueForText("2.5 kHz")) - 2500) < 1, "'2.5 kHz' types as 2500 Hz");
  auto &time = p.parameter(ut::echo_time);
  check(std::abs(time.convertFrom0to1(time.getValueForText("300 ms")) - 300) < .01, "'300 ms' types as 300 ms");
}

// How loud is the space? Reverb alone at full wet against pink-ish noise, then
// every preset on NEON RIOT bass: the level the effect adds over dry, while
// playing and in the tail.
void spaceTest() {
  auto rms = [](const juce::AudioBuffer<float> &b, int from, int to) {
    double s = 0;
    for (int c = 0; c < 2; ++c)
      for (int i = from; i < to; ++i) s += (double)b.getSample(c, i) * b.getSample(c, i);
    return std::sqrt(s / (2.0 * std::max(1, to - from)));
  };
  auto db = [](double v) { return 20 * std::log10(std::max(1e-9, v)); };
  {
    UndertowProcessor p;
    p.setPlayConfigDetails(2, 2, 48000, 256);
    p.prepareToPlay(48000, 256);
    set(p, ut::drift_on, 0); set(p, ut::echo_on, 0); set(p, ut::anchor_on, 0); set(p, ut::mix, 100);
    set(p, ut::void_decay, 3); set(p, ut::void_level, 100); set(p, ut::void_lowcut, 20);
    juce::AudioBuffer<float> in(2, 48000 * 4), out(2, 48000 * 4), b(2, 256);
    juce::Random r(7);
    float lp = 0;
    for (int i = 0; i < in.getNumSamples(); ++i) { lp += .2f * ((r.nextFloat() * 2 - 1) - lp); in.setSample(0, i, lp * .5f); in.setSample(1, i, lp * .5f); }
    for (int at = 0; at < in.getNumSamples(); at += 256) {
      for (int c = 0; c < 2; ++c) b.copyFrom(c, 0, in, c, at, 256);
      process(p, b);
      for (int c = 0; c < 2; ++c) out.copyFrom(c, at, b, c, 0, 256);
    }
    std::printf("VOID alone, 100 %% wet, 3 s decay, steady noise: reverb %.1f dB relative to input\n",
                db(rms(out, 48000 * 2, 48000 * 4) / rms(in, 48000 * 2, 48000 * 4)));
  }
  juce::AudioFormatManager formats;
  formats.registerBasicFormats();
  const auto source = testFile("UNDERTOW_TEST_RIFF");
  std::unique_ptr<juce::AudioFormatReader> r(formats.createReaderFor(source));
  juce::AudioBuffer<float> riff(2, 48000 * 4);
  riff.clear();
  if (r) r->read(&riff, 0, std::min<int>(riff.getNumSamples(), (int)r->lengthInSamples), 0, true, true);
  std::printf("%-30s %12s %12s %12s\n", "preset", "space(play)", "tail 1-3 s", "tail 4-6 s");
  for (int i = 0; i < (int)ut::factoryPresets().size(); ++i) {
    juce::AudioBuffer<float> outs[2];
    for (int pass = 0; pass < 2; ++pass) {
      UndertowProcessor p;
      p.setPlayConfigDetails(2, 2, 48000, 256);
      p.prepareToPlay(48000, 256);
      p.setCurrentProgram(i);
      if (pass == 1) set(p, ut::mix, 0); // same path, no space
      auto &out = outs[pass];
      out.setSize(2, 48000 * 10);
      juce::AudioBuffer<float> b(2, 256);
      for (int at = 0; at < out.getNumSamples(); at += 256) {
        b.clear();
        if (at < riff.getNumSamples()) for (int c = 0; c < 2; ++c) b.copyFrom(c, 0, riff, c, at, std::min(256, riff.getNumSamples() - at));
        process(p, b);
        for (int c = 0; c < 2; ++c) out.copyFrom(c, at, b, c, 0, 256);
      }
    }
    juce::AudioBuffer<float> diff(outs[0]);
    for (int c = 0; c < 2; ++c) diff.addFrom(c, 0, outs[1], c, 0, diff.getNumSamples(), -1.f);
    const double dr = rms(outs[1], 48000, 48000 * 4);
    std::printf("%-30s %9.1f dB %9.1f dB %9.1f dB\n", ut::factoryPresets()[(size_t)i].name,
                db(rms(diff, 48000, 48000 * 4) / dr), db(rms(outs[0], 48000 * 5, 48000 * 7) / dr),
                db(rms(outs[0], 48000 * 8, 48000 * 10) / dr));
  }
}
