#include <cstdio>
#include <juce_audio_utils/juce_audio_utils.h>
#include <mach-o/dyld.h>
#include <map>
#include <set>
int main() {
  juce::ScopedJuceInitialiser_GUI gui;
  juce::AudioPluginFormatManager formats;
  formats.addDefaultFormats();
  juce::OwnedArray<juce::PluginDescription> descriptions;
  for (int i = 0; i < formats.getNumFormats(); ++i)
    formats.getFormat(i)->findAllTypesForFile(
        descriptions, "AudioUnit:Effects/aufx,Utow,Rcwl");
  if (descriptions.isEmpty()) {
    std::puts("FAIL: installed effect not found");
    return 1;
  }
  juce::String error;
  auto p = formats.createPluginInstance(*descriptions[0], 48000, 64, error);
  if (!p) {
    std::puts(error.toRawUTF8());
    return 1;
  }
  std::set<juce::String> names;
  int fails = 0;
  const auto expected =
      juce::File::getSpecialLocation(juce::File::userHomeDirectory)
          .getChildFile("Library/Audio/Plug-Ins/Components/UNDERTOW.component/"
                        "Contents/MacOS/UNDERTOW")
          .getFullPathName();
  bool installed = false;
  for (uint32_t i = 0; i < _dyld_image_count(); ++i)
    if (juce::String(_dyld_get_image_name(i)) == expected)
      installed = true;
  if (!installed)
    ++fails;
  std::printf("Installed image verified: %s\n", installed ? "PASS" : "FAIL");
  if (p->getParameters().size() != 132 || p->getLatencySamples() != 0)
    ++fails;
  for (auto *v : p->getParameters()) {
    auto n = v->getName(128);
    if (n.isEmpty() || !names.insert(n).second || !v->isAutomatable())
      ++fails;
  }
  p->setPlayConfigDetails(2, 2, 48000, 64);
  p->prepareToPlay(48000, 64);
  juce::AudioBuffer<float> b(2, 64);
  b.clear();
  juce::MidiBuffer midi;
  p->processBlock(b, midi);
  std::printf("Installed AU: %d named automatable parameters; latency %d; "
              "failures %d\n",
              p->getParameters().size(), p->getLatencySamples(), fails);
  std::vector<float> quiet, loud;
  for (bool high : {false, true}) {
    p->setCurrentProgram(0);
    for (auto *v : p->getParameters())
      if (v->getName(128) == "In Gain")
        v->setValueNotifyingHost(high ? 1.f : 0.f);
    p->prepareToPlay(48000, 64);
    for (int block = 0; block < 100; ++block) {
      for (int ch = 0; ch < 2; ++ch)
        for (int n = 0; n < 64; ++n)
          b.setSample(ch, n,
                      .05f * std::sin(juce::MathConstants<float>::twoPi * 440 *
                                      (block * 64 + n) / 48000));
      p->processBlock(b, midi);
      auto &output = high ? loud : quiet;
      for (int n = 0; n < 64; ++n)
        output.push_back(b.getSample(0, n));
    }
  }
  double difference = 0;
  for (size_t n = 0; n < quiet.size(); ++n)
    difference = std::max(difference, (double)std::abs(quiet[n] - loud[n]));
  if (difference < .0001)
    ++fails;
  std::printf("Host input gain write maximum audio difference %.9g; total "
              "failures %d\n",
              difference, fails);
  auto norm = [&](const char *name, float value) {
    for (auto *v : p->getParameters())
      if (v->getName(128) == name) {
        v->setValueNotifyingHost(value);
        return;
      }
    ++fails;
  };
  for (bool pulse : {true, false}) {
    std::vector<float> outputs[2];
    for (int pass = 0; pass < 2; ++pass) {
      p->setCurrentProgram(0);
      if (pulse) {
        norm("Drift On", 0);
        norm("Echo On", 0);
        norm("Void On", 0);
        norm("Anchor On", 0);
        norm("Mix", 1);
        norm("Pulse On", 1);
        norm("Pulse Bands", 0);
        norm("Pulse Position", 1);
        norm("Pulse B1 Depth", (float)pass);
      } else
        norm("Macro Depth", pass ? 1.f : .5f);
      p->prepareToPlay(48000, 64);
      for (int block = 0; block < 1500; ++block) {
        for (int c = 0; c < 2; ++c)
          for (int n = 0; n < 64; ++n)
            b.setSample(c, n,
                        .05f * std::sin(juce::MathConstants<double>::twoPi *
                                        440 * (block * 64 + n) / 48000));
        p->processBlock(b, midi);
        for (int n = 0; n < 64; ++n)
          outputs[pass].push_back(b.getSample(0, n));
      }
    }
    double delta = 0;
    for (size_t n = 0; n < outputs[0].size(); ++n)
      delta = std::max(delta, (double)std::abs(outputs[0][n] - outputs[1][n]));
    if (delta < 1.e-4)
      ++fails;
    std::printf("Host %s write maximum audio difference %.9g\n",
                pulse ? "pulse depth" : "depth macro", delta);
  }
  // Factory bank as the host sees it.
  const int programs = p->getNumPrograms();
  bool named = programs == 80;
  for (int i = 0; i < programs; ++i)
    named = named && p->getProgramName(i).contains(" / ");
  if (!named) ++fails;
  std::printf("Host programs: %d, first '%s', last '%s' %s\n", programs, p->getProgramName(0).toRawUTF8(),
              p->getProgramName(programs - 1).toRawUTF8(), named ? "PASS" : "FAIL");
  p->setCurrentProgram(7);
  float pulseOn = -1, decay = -1;
  for (auto *v : p->getParameters()) {
    if (v->getName(128) == "Pulse On") pulseOn = v->getValue();
    if (v->getName(128) == "Void Decay") decay = v->getValue();
  }
  const bool loaded = p->getCurrentProgram() == 7 && pulseOn > .5f;
  if (!loaded) ++fails;
  std::printf("Host program change reaches parameters (PULSE / Breathing Walls): %s\n", loaded ? "PASS" : "FAIL");
  juce::MemoryBlock saved;
  p->getStateInformation(saved);
  p->setCurrentProgram(0);
  p->setStateInformation(saved.getData(), (int)saved.getSize());
  float decayBack = -1;
  for (auto *v : p->getParameters())
    if (v->getName(128) == "Void Decay") decayBack = v->getValue();
  // After a state restore the AU layer reports an untitled preset (-1); the
  // processor keeps its own preset index, checked in UndertowRender --bank.
  const bool restored = std::abs(decayBack - decay) < 1.e-6f;
  if (!restored) ++fails;
  std::printf("Host state round trip keeps the preset: %s (program %d, decay %.4f vs %.4f)\n", restored ? "PASS" : "FAIL", p->getCurrentProgram(), decayBack, decay);
  // ---- Automation as Logic sees it -----------------------------------------
  {
    // Sub-menus: every parameter sits in one of the eight groups.
    const auto &tree = p->getParameterTree();
    juce::StringArray groupNames;
    int grouped = 0;
    for (auto *g : tree.getSubgroups(false)) {
      groupNames.add(g->getName());
      grouped += g->getParameters(true).size();
    }
    const bool groupsOk = groupNames.size() == 8 && grouped == p->getParameters().size();
    if (!groupsOk) ++fails;
    std::printf("Automation menu groups: %s (%d of %d parameters grouped) %s\n",
                groupNames.joinIntoString(", ").toRawUTF8(), grouped, p->getParameters().size(), groupsOk ? "PASS" : "FAIL");

    // Switches and stepped controls (as the host sees them, through their text):
    // Logic may write any value, and the plug-in reads it as the nearest step.
    std::map<juce::AudioProcessorParameter *, int> stepsOf;
    for (auto *v : p->getParameters()) {
      std::set<juce::String> texts;
      for (int i = 0; i <= 200; ++i) texts.insert(v->getText(i / 200.f, 64));
      stepsOf[v] = texts.size() <= 21 ? (int)texts.size() : 0;
    }
    auto same = [&](juce::AudioProcessorParameter *v, float a, float b) {
      return stepsOf[v] > 0 ? v->getText(a, 64) == v->getText(b, 64) : std::abs(a - b) <= 1.e-5f;
    };
    // Every parameter takes a host write, reads it back and shows readable text.
    int bad = 0;
    for (auto *v : p->getParameters()) {
      const int steps = stepsOf[v];
      for (float target : {0.f, .37f, 1.f}) {
        v->setValueNotifyingHost(target);
        const float got = v->getValue();
        if (steps > 0 ? v->getText(got, 64) != v->getText(target, 64) : std::abs(got - target) > 1.e-4f) {
          ++bad;
          std::printf("  %s: wrote %.3f read %.4f\n", v->getName(64).toRawUTF8(), target, got);
        }
        if (v->getText(got, 64).trim().isEmpty()) { ++bad; std::printf("  %s: empty text\n", v->getName(64).toRawUTF8()); }
      }
      if (v->getText(0.f, 64) == v->getText(1.f, 64)) {
        ++bad;
        std::printf("  %s: the same text at both ends ('%s')\n", v->getName(64).toRawUTF8(), v->getText(0.f, 64).toRawUTF8());
      }
      v->setValueNotifyingHost(v->getDefaultValue());
    }
    if (bad) ++fails;
    std::printf("Every parameter takes a host write, reads back and shows text at both ends: %s (%d problems)\n",
                bad ? "FAIL" : "PASS", bad);

    // Remembered: every parameter at a random value survives the host's save and reload.
    juce::Random rnd(9031);
    std::vector<float> wrote;
    for (auto *v : p->getParameters()) {
      v->setValueNotifyingHost(rnd.nextFloat());
      wrote.push_back(v->getValue());
    }
    juce::MemoryBlock blob;
    p->getStateInformation(blob);
    juce::String err2;
    auto q = formats.createPluginInstance(*descriptions[0], 48000, 64, err2);
    int lost = 0;
    if (q) {
      q->setStateInformation(blob.getData(), (int)blob.getSize());
      auto *qs = &q->getParameters();
      for (int i = 0; i < p->getParameters().size(); ++i) {
        auto *a = p->getParameters()[i];
        juce::AudioProcessorParameter *match = nullptr;
        for (auto *bq : *qs) if (bq->getName(128) == a->getName(128)) match = bq;
        if (!match || !same(a, match->getValue(), wrote[(size_t)i])) {
          ++lost;
          std::printf("  %s: saved %.5f, reloaded %.5f\n", a->getName(64).toRawUTF8(), wrote[(size_t)i],
                      match ? match->getValue() : -1.f);
        }
      }
    } else ++lost;
    if (lost) ++fails;
    std::printf("All %d parameters at random values survive save and reload in a new instance: %s (%d lost)\n",
                p->getParameters().size(), lost ? "FAIL" : "PASS", lost);

    // Every lane automated at once, a new value each block, for 8 s.
    p->setCurrentProgram(0);
    p->prepareToPlay(48000, 64);
    std::vector<float> from, to;
    for (auto *v : p->getParameters()) { from.push_back(v->getValue()); to.push_back(rnd.nextFloat()); }
    float peak = 0;
    bool finite = true;
    const int blocks = 48000 * 8 / 64;
    for (int block = 0; block < blocks; ++block) {
      const float t = (float)block / (float)blocks;
      int i = 0;
      for (auto *v : p->getParameters()) {
        const auto name = v->getName(64);
        // leave FREEZE and the output trims to the music; everything else moves
        if (name != "Void Freeze" && name != "Out Gain" && name != "In Gain")
          v->setValueNotifyingHost(from[(size_t)i] + (to[(size_t)i] - from[(size_t)i]) * (.5f - .5f * std::cos(6.2831853f * t)));
        ++i;
      }
      for (int ch = 0; ch < 2; ++ch)
        for (int n = 0; n < 64; ++n)
          b.setSample(ch, n, .1f * std::sin(juce::MathConstants<float>::twoPi * 220 * (block * 64 + n) / 48000));
      p->processBlock(b, midi);
      for (int ch = 0; ch < 2; ++ch)
        for (int n = 0; n < 64; ++n) {
          const float s = b.getSample(ch, n);
          finite = finite && std::isfinite(s);
          peak = std::max(peak, std::abs(s));
        }
    }
    const bool safe = finite && peak <= 1.f;
    if (!safe) ++fails;
    std::printf("Every lane automated at once for 8 s: %s (peak %.3f)\n", safe ? "PASS" : "FAIL", peak);
    p->setCurrentProgram(0);
  }
  {
    std::unique_ptr<juce::AudioProcessorEditor> ed(p->createEditorIfNeeded());
    const bool custom = ed != nullptr && p->hasEditor() && ed->getWidth() >= 960 && ed->getHeight() >= 600;
    if (!custom) ++fails;
    std::printf("Host opens the custom editor: %s (%dx%d)\n", custom ? "PASS" : "FAIL", ed ? ed->getWidth() : 0,
                ed ? ed->getHeight() : 0);
  }
  std::printf("HOST RESULT %d failures\n", fails);
  return fails ? 1 : 0;
}
