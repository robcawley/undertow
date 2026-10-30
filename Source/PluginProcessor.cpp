#include "PluginProcessor.h"
#include "PluginEditor.h"
UndertowProcessor::UndertowProcessor()
    : AudioProcessor(
          BusesProperties()
              .withInput("Input", juce::AudioChannelSet::stereo(), true)
              .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "UNDERTOW", ut::layout()) {
  for (size_t k = 0; k < ut::count; ++k) {
    raw[k] = state.getRawParameterValue(ut::specs[k].id);
    params[k] = state.getParameter(ut::specs[k].id);
  }
  // Each factory preset's scene A as normalised values, for the EDITED badge.
  const auto &bank = ut::factoryPresets();
  presetNormal.resize(bank.size());
  for (size_t i = 0; i < bank.size(); ++i) {
    for (size_t k = 0; k < ut::count; ++k)
      presetNormal[i][k] = params[k]->getDefaultValue();
    for (const auto &v : bank[i].a)
      presetNormal[i][(size_t)v.param] =
          params[(size_t)v.param]->convertTo0to1(v.value);
  }
  for (int k = 0; k < ut::count; ++k)
    extra.sceneB[(size_t)k] =
        state.getParameter(ut::specs[(size_t)k].id)->getDefaultValue();
  exchange.publish(extra);
  modulation.bind(state);
  for (int k = 0; k < ut::count; ++k)
    effective[(size_t)k].store(raw[(size_t)k]->load());
  setLatencySamples(0);
}
bool UndertowProcessor::isBusesLayoutSupported(const BusesLayout &l) const {
  return l.outputBuses.size() == 1 && l.inputBuses.size() == 1 &&
         l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo() &&
         (l.getMainInputChannelSet() == juce::AudioChannelSet::stereo() ||
          l.getMainInputChannelSet() == juce::AudioChannelSet::mono());
}
double UndertowProcessor::getTailLengthSeconds() const {
  auto value = [this](int k) {
    return raw[(size_t)k]->load(std::memory_order_relaxed);
  };
  const bool modulated = value(ut::bloom) > 0 || value(ut::macro_depth) != 0 ||
                         (hasCornerOffsets.load(std::memory_order_relaxed) &&
                          value(ut::xy_amount) > 0);
  double seconds = .1;
  if (value(ut::echo_on) > .5f) {
    const double delay = value(ut::echo_sync) > .5f
                             ? 3 * ut::beats[(size_t)juce::jlimit(
                                       0, 20, (int)value(ut::echo_div))]
                             : (modulated ? 2.5 : value(ut::echo_time) * .001);
    const double feedback = modulated ? .98 : value(ut::echo_feedback) * .01;
    const double repeats =
        feedback > 0
            ? std::ceil(std::log(1.e-6) / std::log(std::min(.98, feedback)))
            : 1;
    seconds += delay * (repeats + 3);
  }
  if (value(ut::void_on) > .5f)
    seconds += 3 * (modulated ? 100 : value(ut::void_decay)) +
               (modulated ? .5 : value(ut::void_predelay) * .001) + 2;
  if (value(ut::pulse_on) > .5f && value(ut::pulse_focus_on) > .5f &&
      (modulated || value(ut::pulse_focus_depth) > 0)) {
    const double root =
        32.70319566257483 * std::exp2(value(ut::pulse_focus_note) / 12.);
    seconds += 15 * (modulated ? 60 : value(ut::pulse_focus_q)) /
               (juce::MathConstants<double>::pi * root);
  }
  return seconds;
}
void UndertowProcessor::prepareToPlay(double sr, int bs) {
  engine.prepare(sr, bs);
  transport.prepare(sr);
  tempo = 120;
  barBeats = 4;
}
void UndertowProcessor::processBlock(juce::AudioBuffer<float> &b,
                                     juce::MidiBuffer &) {
  const auto &frame = exchange.consume();
  std::array<float, ut::count> values{};
  for (size_t k = 0; k < values.size(); ++k)
    values[k] = raw[k]->load(std::memory_order_relaxed);
  double ppq = 0;
  bool playing = false, hasPosition = false;
  if (auto *p = getPlayHead())
    if (auto pos = p->getPosition()) {
      if (auto bpm = pos->getBpm(); bpm && std::isfinite(*bpm))
        tempo = juce::jlimit(20., 400., *bpm);
      if (auto beat = pos->getPpqPosition(); beat && std::isfinite(*beat)) {
        ppq = *beat;
        hasPosition = true;
      }
      if (auto signature = pos->getTimeSignature();
          signature && signature->numerator > 0 && signature->denominator > 0)
        barBeats = signature->numerator * 4.0 / signature->denominator;
      playing = pos->getIsPlaying();
    }
  const auto timeline = transport.begin(b.getNumSamples(), tempo, ppq, playing,
                                        hasPosition, barBeats);
  beatNow.store(timeline.beat, std::memory_order_relaxed);
  barNow.store(timeline.barBeats, std::memory_order_relaxed);
  tempoNow.store(timeline.bpm, std::memory_order_relaxed);
  const auto snapshot = modulation.evaluate(values, frame, timeline);
  for (size_t k = 0; k < values.size(); ++k)
    effective[k].store(snapshot.values[k], std::memory_order_relaxed);
  puckX.store(snapshot.x, std::memory_order_relaxed);
  puckY.store(snapshot.y, std::memory_order_relaxed);
  engine.process(b, getTotalNumInputChannels(), snapshot.values, frame,
                 timeline);
}
void UndertowProcessor::copySceneAToB() {
  auto next = extraState();
  for (int k = 0; k < ut::count; ++k)
    if (ut::snapshotParameter(k))
      next.sceneB[(size_t)k] =
          state.getParameter(ut::specs[(size_t)k].id)->getValue();
  next.edited = true;
  setExtraState(next);
}
void UndertowProcessor::copySceneBToA() {
  const auto snapshot = extraState();
  for (int k = 0; k < ut::count; ++k)
    if (ut::snapshotParameter(k)) {
      auto *p = state.getParameter(ut::specs[(size_t)k].id);
      p->beginChangeGesture();
      p->setValueNotifyingHost(snapshot.sceneB[(size_t)k]);
      p->endChangeGesture();
    }
}
void UndertowProcessor::swapScenes() {
  const auto before = extraState();
  std::array<float, ut::count> a{};
  for (int k = 0; k < ut::count; ++k)
    if (ut::snapshotParameter(k))
      a[(size_t)k] = state.getParameter(ut::specs[(size_t)k].id)->getValue();
  copySceneBToA();
  auto next = before;
  for (int k = 0; k < ut::count; ++k)
    if (ut::snapshotParameter(k))
      next.sceneB[(size_t)k] = a[(size_t)k];
  next.edited = true;
  setExtraState(next);
}
void UndertowProcessor::setSceneBValue(int index, float normalized) {
  if (index < 0 || index >= ut::count || !ut::snapshotParameter(index) ||
      !std::isfinite(normalized))
    return;
  auto next = extraState();
  next.sceneB[(size_t)index] = std::clamp(normalized, 0.f, 1.f);
  next.edited = true;
  setExtraState(next);
}
int UndertowProcessor::getCurrentProgram() {
  std::lock_guard guard(extraMutex);
  return extra.preset;
}
const juce::String UndertowProcessor::getProgramName(int index) {
  const auto &bank = ut::factoryPresets();
  if (index < 0 || index >= (int)bank.size())
    return {};
  return juce::String(bank[(size_t)index].category) + " / " +
         bank[(size_t)index].name;
}
void UndertowProcessor::setCurrentProgram(int index) {
  const auto &bank = ut::factoryPresets();
  index = juce::jlimit(0, (int)bank.size() - 1, index);
  const auto &def = bank[(size_t)index];
  const auto before = extraState();
  ut::ExtraState next;
  // Interface preferences belong to the project, not the preset.
  next.width = before.width;
  next.height = before.height;
  next.preset = index;
  next.edited = false;
  std::array<float, ut::count> a = presetNormal[(size_t)index];
  // RETURN is a performance setting of the project, like the window size.
  a[ut::xy_return] = params[ut::xy_return]->getValue();
  for (size_t k = 0; k < ut::count; ++k)
    next.sceneB[k] = a[k];
  for (const auto &v : def.b)
    next.sceneB[(size_t)v.param] = params[(size_t)v.param]->convertTo0to1(v.value);
  for (int c = 0; c < 4; ++c)
    for (const auto &v : def.corners[(size_t)c])
      next.corners[(size_t)c][(size_t)v.param] = juce::jlimit(-1.f, 1.f, v.value);
  next.patterns = def.patterns;
  applySnapshot(a, next);
}
int UndertowProcessor::parameterIndex(const juce::String &id) const {
  for (int k = 0; k < ut::count; ++k)
    if (id == ut::specs[(size_t)k].id)
      return k;
  jassertfalse;
  return 0;
}
std::array<float, ut::count> UndertowProcessor::normalizedValues() const {
  std::array<float, ut::count> out{};
  for (size_t k = 0; k < ut::count; ++k)
    out[k] = params[k]->getValue();
  return out;
}
void UndertowProcessor::applySnapshot(
    const std::array<float, ut::count> &normalized, const ut::ExtraState &next) {
  for (size_t k = 0; k < ut::count; ++k) {
    auto *p = params[k];
    if (std::abs(p->getValue() - normalized[k]) > 1.e-7f) {
      p->beginChangeGesture();
      p->setValueNotifyingHost(normalized[k]);
      p->endChangeGesture();
    }
  }
  setExtraState(next);
}
bool UndertowProcessor::isEdited() const {
  int preset = 0;
  {
    std::lock_guard guard(extraMutex);
    if (extra.edited)
      return true;
    preset = extra.preset;
  }
  const auto &ref = presetNormal[(size_t)juce::jlimit(0, (int)presetNormal.size() - 1, preset)];
  for (size_t k = 0; k < ut::count; ++k)
    if (k != ut::xy_return && std::abs(params[k]->getValue() - ref[k]) > 1.e-4f)
      return true;
  return false;
}
juce::AudioProcessorEditor *UndertowProcessor::createEditor() {
  return new UndertowEditor(*this);
}
void UndertowProcessor::getStateInformation(juce::MemoryBlock &dest) {
  auto tree = state.copyState();
  tree.setProperty("state_version", 1, nullptr);
  const auto saved = extraState();
  juce::ValueTree extras("EXTRAS");
  extras.setProperty("width", saved.width, nullptr);
  extras.setProperty("height", saved.height, nullptr);
  // read by versions before 0.6.0, where RETURN was not a parameter
  extras.setProperty("return", params[ut::xy_return]->getValue() > .5f, nullptr);
  extras.setProperty("preset", saved.preset, nullptr);
  extras.setProperty("edited", saved.edited, nullptr);
  juce::ValueTree scene("SCENE_B");
  for (int k = 0; k < ut::count; ++k)
    if (ut::snapshotParameter(k))
      scene.setProperty(ut::specs[(size_t)k].id, saved.sceneB[(size_t)k],
                        nullptr);
  extras.appendChild(scene, nullptr);
  for (int corner = 0; corner < 4; ++corner) {
    juce::ValueTree entry("CORNER");
    entry.setProperty("index", corner, nullptr);
    for (int k = 0; k < ut::count; ++k)
      if (ut::specs[(size_t)k].kind == 0 &&
          saved.corners[(size_t)corner][(size_t)k] != 0)
        entry.setProperty(ut::specs[(size_t)k].id,
                          saved.corners[(size_t)corner][(size_t)k], nullptr);
    extras.appendChild(entry, nullptr);
  }
  juce::ValueTree patterns("PATTERNS");
  for (int band = 0; band < 6; ++band)
    patterns.setProperty("b" + juce::String(band),
                         (int)saved.patterns[(size_t)band], nullptr);
  extras.appendChild(patterns, nullptr);
  juce::ValueTree path("MOTION");
  for (int k = 0; k < saved.motionCount && k < 1024; ++k) {
    juce::ValueTree point("POINT");
    const auto &v = saved.motion[(size_t)k];
    point.setProperty("bar", v.bar, nullptr);
    point.setProperty("x", v.x, nullptr);
    point.setProperty("y", v.y, nullptr);
    path.appendChild(point, nullptr);
  }
  extras.appendChild(path, nullptr);
  tree.removeChild(tree.getChildWithName("EXTRAS"), nullptr);
  tree.appendChild(extras, nullptr);
  copyXmlToBinary(*tree.createXml(), dest);
}
void UndertowProcessor::setStateInformation(const void *data, int size) {
  auto xml = getXmlFromBinary(data, size);
  if (!xml)
    return;
  auto tree = juce::ValueTree::fromXml(*xml);
  if (!tree.hasType("UNDERTOW"))
    return;
  // Rebuild only known parameters and retain their exact bounded raw values.
  auto clean = state.copyState();
  std::array<float, ut::count> restored{};
  for (int k = 0; k < ut::count; ++k) {
    const auto &spec = ut::specs[(size_t)k];
    const auto child = tree.getChildWithProperty("id", spec.id);
    float value = child.isValid()
                      ? (float)child.getProperty("value", spec.initial)
                      : spec.initial;
    if (!std::isfinite(value))
      value = spec.initial;
    value = juce::jlimit(spec.lo, spec.hi, value);
    if (spec.kind != 0)
      value = std::round(value);
    // projects from before 0.6.0 kept RETURN with the interface settings
    if (k == ut::xy_return && !child.isValid())
      value = (bool)tree.getChildWithName("EXTRAS").getProperty("return", false) ? 1.f : 0.f;
    restored[(size_t)k] = value;
    clean.getChildWithProperty("id", spec.id)
        .setProperty("value", value, nullptr);
  }
  state.replaceState(clean);
  for (int k = 0; k < ut::count; ++k)
    raw[(size_t)k]->store(restored[(size_t)k], std::memory_order_relaxed);
  ut::ExtraState loaded;
  for (int k = 0; k < ut::count; ++k)
    loaded.sceneB[(size_t)k] =
        state.getParameter(ut::specs[(size_t)k].id)->getDefaultValue();
  const auto extras = tree.getChildWithName("EXTRAS");
  if (extras.isValid()) {
    loaded.width =
        juce::jlimit(960, 2560, (int)extras.getProperty("width", 1280));
    loaded.height =
        juce::jlimit(600, 1600, (int)extras.getProperty("height", 800));
    loaded.preset = juce::jlimit(0, getNumPrograms() - 1,
                                 (int)extras.getProperty("preset", 0));
    loaded.edited = (bool)extras.getProperty("edited", false);
    auto bounded = [](const juce::var &v, float fallback, float lo, float hi) {
      const float f = (float)v;
      return std::isfinite(f) ? juce::jlimit(lo, hi, f) : fallback;
    };
    const auto scene = extras.getChildWithName("SCENE_B");
    for (int k = 0; k < ut::count; ++k)
      if (ut::snapshotParameter(k) &&
          scene.hasProperty(ut::specs[(size_t)k].id))
        loaded.sceneB[(size_t)k] =
            bounded(scene.getProperty(ut::specs[(size_t)k].id),
                    loaded.sceneB[(size_t)k], 0, 1);
    for (auto entry : extras)
      if (entry.hasType("CORNER")) {
        const int corner = (int)entry.getProperty("index", -1);
        if (corner < 0 || corner > 3)
          continue;
        for (int k = 0; k < ut::count; ++k)
          if (ut::specs[(size_t)k].kind == 0 &&
              entry.hasProperty(ut::specs[(size_t)k].id))
            loaded.corners[(size_t)corner][(size_t)k] =
                bounded(entry.getProperty(ut::specs[(size_t)k].id), 0, -1, 1);
      }
    const auto patterns = extras.getChildWithName("PATTERNS");
    for (int band = 0; band < 6; ++band)
      loaded.patterns[(size_t)band] =
          (uint16_t)(int)patterns.getProperty("b" + juce::String(band), 65535);
    const auto motion = extras.getChildWithName("MOTION");
    loaded.motionCount = std::min(1024, motion.getNumChildren());
    for (int k = 0; k < loaded.motionCount; ++k) {
      const auto point = motion.getChild(k);
      auto &v = loaded.motion[(size_t)k];
      v.bar = bounded(point.getProperty("bar", 0), 0, 0, 16);
      v.x = bounded(point.getProperty("x", .5f), .5f, 0, 1);
      v.y = bounded(point.getProperty("y", .5f), .5f, 0, 1);
    }
  }
  setExtraState(loaded);
}
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter() {
  return new UndertowProcessor();
}
