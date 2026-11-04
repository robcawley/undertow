// Golden renders: every factory preset through the same input, written as raw
// interleaved float32, so a refactor can be compared sample for sample.
//   --golden <dir>               write <dir>/NN.f32 for every preset
//   --golden-compare <a> <b>     compare two golden folders

constexpr int goldenRate = 48000, goldenBlock = 256;
constexpr int goldenSeconds = 7, goldenDrySeconds = 4;

void golden(const juce::File &dir) {
  dir.createDirectory();
  const int presetCount = (int)ut::factoryPresets().size();
  for (int preset = 0; preset < presetCount; ++preset) {
    UndertowProcessor p;
    prepare(p, goldenRate, goldenBlock);
    p.setCurrentProgram(preset);
    const int total = goldenRate * goldenSeconds,
              driven = goldenRate * goldenDrySeconds;
    juce::AudioBuffer<float> b(2, goldenBlock);
    std::vector<float> interleaved((size_t)total * 2);
    for (int pos = 0; pos < total; pos += goldenBlock) {
      const int n = std::min(goldenBlock, total - pos);
      b.setSize(2, n, false, false, true);
      b.clear();
      if (pos < driven) {
        // A bass riff with clicks, and noise bursts on the right only, so the
        // two channels differ.
        juce::AudioBuffer<float> riff(2, n), click(2, n), noise(2, n);
        input(riff, pos, goldenRate, 2);
        input(click, pos, goldenRate, 4);
        input(noise, pos, goldenRate, 3);
        for (int ch = 0; ch < 2; ++ch)
          for (int i = 0; i < n; ++i)
            b.setSample(ch, i,
                        riff.getSample(ch, i) + .5f * click.getSample(ch, i) +
                            (ch == 1 ? noise.getSample(ch, i) : 0.f));
      }
      process(p, b);
      for (int i = 0; i < n; ++i)
        for (int ch = 0; ch < 2; ++ch)
          interleaved[(size_t)(pos + i) * 2 + (size_t)ch] = b.getSample(ch, i);
    }
    auto file = dir.getChildFile(juce::String(preset).paddedLeft('0', 2) + ".f32");
    file.deleteFile();
    juce::FileOutputStream out(file);
    check(out.openedOk(), "open golden file");
    out.write(interleaved.data(), interleaved.size() * sizeof(float));
    out.flush();
  }
  std::printf("golden presets written: %d\n", presetCount);
}

void goldenCompare(const juce::File &a, const juce::File &b) {
  const int presetCount = (int)ut::factoryPresets().size();
  double worstDb = -400, worstAbs = 0;
  int identical = 0, worstPreset = -1;
  for (int preset = 0; preset < presetCount; ++preset) {
    const auto name = juce::String(preset).paddedLeft('0', 2) + ".f32";
    juce::MemoryBlock x, y;
    const bool ok = a.getChildFile(name).loadFileAsData(x) &&
                    b.getChildFile(name).loadFileAsData(y) &&
                    x.getSize() == y.getSize() && x.getSize() > 0;
    check(ok, ("golden pair present " + name).toRawUTF8());
    if (!ok)
      continue;
    const auto *p = static_cast<const float *>(x.getData());
    const auto *q = static_cast<const float *>(y.getData());
    const size_t count = x.getSize() / sizeof(float);
    double signal = 0, error = 0, maxAbs = 0;
    for (size_t i = 0; i < count; ++i) {
      const double d = (double)p[i] - (double)q[i];
      signal += (double)p[i] * p[i];
      error += d * d;
      maxAbs = std::max(maxAbs, std::abs(d));
    }
    if (error == 0) {
      ++identical;
      continue;
    }
    const double db = 10 * std::log10(error / std::max(signal, 1.e-30));
    std::printf("preset %2d  diff %.1f dB re signal, max %.3g\n", preset, db, maxAbs);
    if (db > worstDb) {
      worstDb = db;
      worstPreset = preset;
    }
    worstAbs = std::max(worstAbs, maxAbs);
  }
  std::printf("identical %d / %d; worst %.1f dB (preset %d), max abs %.3g\n",
              identical, presetCount, worstDb, worstPreset, worstAbs);
  check(worstDb <= -120, "every preset within -120 dB of the golden render");
}
