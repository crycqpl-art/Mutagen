#include "Presets.h"
#include "PluginProcessor.h"
#include <iterator>

namespace
{
using namespace mutagen;

struct Preset
{
    const char* name;
    int pattern[16];
    // seqOn, rate, cloudAmt, cloudSize, cloudDensity, cloudSpread, chord, octave(index 0..4),
    // smearAmt, smearBlur, freeze, cutoff, reso, filterType, verbMix, verbSize, mix, gain
    float v[18];
};

constexpr int P = Pass, S = Stutter, R = Reverse, H = HalfSpeed, O = OctaveUp,
              F = Freeze, T = TapeStop, G = Gate, X = Scatter;

const Preset presets[] = {
    { "Init (clean)",
      { P,P,P,P, P,P,P,P, P,P,P,P, P,P,P,P },
      { 1, 2, 0.0f, 120, 18, 0.35f, 3, 2, 0.0f, 0.5f, 0, 20000, 0.707f, 0, 0.0f, 0.6f, 1.0f, 0 } },

    { "Lo-Fi Chop",
      { P,P,S,P, R,P,H,H, P,P,X,P, S,S,T,G },
      { 1, 2, 0.0f, 120, 18, 0.35f, 3, 2, 0.1f, 0.3f, 0, 6000, 0.9f, 0, 0.12f, 0.5f, 1.0f, 1 } },

    { "Dream Pad",
      { P,P,P,F, P,P,P,F, P,P,P,F, P,P,F,F },
      { 1, 1, 0.75f, 300, 25, 0.6f, 6, 2, 0.45f, 0.7f, 0, 9000, 0.707f, 0, 0.45f, 0.85f, 1.0f, 6.5f } },

    { "Glitch Rush",
      { S,P,O,X, P,S,R,G, X,O,P,S, R,G,S,S },
      { 1, 3, 0.15f, 40, 40, 0.2f, 2, 2, 0.0f, 0.4f, 0, 20000, 0.707f, 0, 0.1f, 0.4f, 1.0f, 2 } },

    { "Tape Ghost",
      { P,H,H,P, R,R,P,T, P,H,P,R, X,P,T,T },
      { 1, 1, 0.3f, 200, 14, 0.5f, 1, 1, 0.25f, 0.4f, 0, 3500, 0.8f, 0, 0.3f, 0.6f, 1.0f, 5 } },

    { "Frozen Choir",
      { F,F,P,F, F,P,F,F, F,F,P,F, F,P,F,F },
      { 1, 1, 0.9f, 450, 30, 0.8f, 8, 3, 0.6f, 0.85f, 0, 12000, 0.707f, 0, 0.55f, 0.9f, 1.0f, 6 } },

    { "Dark Matter",
      { R,P,H,X, R,H,P,X, R,P,H,X, T,T,G,G },
      { 1, 2, 0.6f, 250, 20, 0.5f, 9, 1, 0.5f, 0.6f, 0, 2500, 1.5f, 0, 0.4f, 0.7f, 1.0f, 8 } },

    { "Stutter Pop",
      { P,P,P,S, P,P,X,P, P,P,R,P, S,S,G,P },
      { 1, 2, 0.0f, 120, 18, 0.35f, 3, 2, 0.0f, 0.5f, 0, 20000, 0.707f, 0, 0.1f, 0.5f, 1.0f, 0 } },

    { "Shimmer Wash",
      { P,P,P,P, P,P,P,P, P,P,P,P, P,P,P,P },
      { 1, 2, 0.6f, 350, 28, 0.7f, 1, 3, 0.55f, 0.8f, 0, 180, 0.707f, 2, 0.5f, 0.9f, 1.0f, 9 } },

    { "Broken Radio",
      { P,G,P,S, X,G,P,O, P,G,R,S, X,G,O,O },
      { 1, 3, 0.2f, 60, 30, 0.3f, 2, 2, 0.2f, 0.2f, 0, 2200, 1.6f, 1, 0.15f, 0.4f, 1.0f, 12 } },
};

const char* valueIds[18] = { "seqOn", "rate", "cloudAmt", "cloudSize", "cloudDensity", "cloudSpread",
                             "chord", "octave", "smearAmt", "smearBlur", "freeze", "cutoff", "reso",
                             "filterType", "verbMix", "verbSize", "mix", "gain" };
} // namespace

juce::StringArray mutagen::presetNames()
{
    juce::StringArray names;
    for (auto& pr : presets) names.add (pr.name);
    return names;
}

void MutagenProcessor::applyPreset (int index)
{
    if (index < 0 || index >= (int) std::size (presets)) return;
    const auto& pr = presets[index];
    for (int i = 0; i < 18; ++i)
        setParam (valueIds[i], pr.v[i]);
    for (int i = 0; i < numSteps; ++i)
        setParam (stepId (i), (float) pr.pattern[i]);
}
