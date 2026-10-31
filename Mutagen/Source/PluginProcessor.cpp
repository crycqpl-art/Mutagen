#include "PluginProcessor.h"
#include "PluginEditor.h"

static const char* paramIds[] = {
    "seqOn", "rate", "cloudAmt", "cloudSize", "cloudDensity", "cloudSpread", "chord", "octave",
    "smearAmt", "smearBlur", "freeze", "cutoff", "reso", "filterType", "verbMix", "verbSize",
    "mix", "gain"
};

juce::AudioProcessorValueTreeState::ParameterLayout MutagenProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto pct = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return String (juce::roundToInt (v * 100.0f)) + " %"; });
    auto fmt = [] (int decimals, const char* unit)
    {
        return AudioParameterFloatAttributes().withStringFromValueFunction (
            [decimals, unit] (float v, int) { return String (v, decimals) + unit; });
    };
    auto hz = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return v >= 1000.0f ? String (v / 1000.0f, 1) + " kHz" : String (juce::roundToInt (v)) + " Hz"; });
    auto db = AudioParameterFloatAttributes().withStringFromValueFunction (
        [] (float v, int) { return (v > 0.05f ? "+" : "") + String (v, 1) + " dB"; });

    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "seqOn", 1 }, "Sequencer On", true));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "rate", 1 }, "Step Rate",
                                                        StringArray { "1/4", "1/8", "1/16", "1/32" }, 2));

    for (int i = 0; i < numSteps; ++i)
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { stepId (i), 1 },
                                                            "Step " + String (i + 1),
                                                            mutagen::stepModeLongNames(), 0));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "cloudAmt", 1 }, "Cloud Amount",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.0f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "cloudSize", 1 }, "Grain Size",
                                                       NormalisableRange<float> (15.0f, 600.0f, 0.0f, 0.5f), 120.0f, fmt (0, " ms")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "cloudDensity", 1 }, "Grain Density",
                                                       NormalisableRange<float> (1.0f, 60.0f, 0.0f, 0.6f), 18.0f, fmt (1, " /s")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "cloudSpread", 1 }, "Grain Spread",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.35f, pct));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "chord", 1 }, "Chord",
                                                        mutagen::chordNames(), 3));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "octave", 1 }, "Cloud Octave",
                                                        StringArray { "-2", "-1", "0", "+1", "+2" }, 2));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "smearAmt", 1 }, "Smear Amount",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.0f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "smearBlur", 1 }, "Smear Blur",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.5f, pct));
    layout.add (std::make_unique<AudioParameterBool> (ParameterID { "freeze", 1 }, "Spectral Freeze", false));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "cutoff", 1 }, "Filter Cutoff",
                                                       NormalisableRange<float> (40.0f, 20000.0f, 0.0f, 0.25f), 20000.0f, hz));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "reso", 1 }, "Filter Resonance",
                                                       NormalisableRange<float> (0.5f, 6.0f, 0.0f, 0.5f), 0.707f, fmt (2, "")));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "filterType", 1 }, "Filter Type",
                                                        StringArray { "Low-pass", "Band-pass", "High-pass" }, 0));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "verbMix", 1 }, "Reverb Amount",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.0f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "verbSize", 1 }, "Reverb Size",
                                                       NormalisableRange<float> (0.0f, 1.0f), 0.6f, pct));

    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Dry/Wet",
                                                       NormalisableRange<float> (0.0f, 1.0f), 1.0f, pct));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "gain", 1 }, "Output",
                                                       NormalisableRange<float> (-24.0f, 12.0f), 0.0f, db));
    return layout;
}

MutagenProcessor::MutagenProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MUTAGEN", createLayout())
{
    for (int i = 0; i < pNumParams; ++i)
        p[i] = apvts.getRawParameterValue (paramIds[i]);
    for (int i = 0; i < numSteps; ++i)
        stepParams[(size_t) i] = apvts.getRawParameterValue (stepId (i));
}

bool MutagenProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}

void MutagenProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    const int numCh = juce::jmax (1, getTotalNumOutputChannels());

    sequencer.prepare (sr, numCh);
    cloud.prepare (sr);
    for (auto& s : smear) s.prepare (sr);

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) samplesPerBlock, (juce::uint32) numCh };
    filter.prepare (spec);
    filter.reset();
    lastFilterType = -1;

    reverb.setSampleRate (sr);
    reverb.reset();

    dryBuffer.setSize (numCh, samplesPerBlock);

    for (auto* s : { &mixSm, &gainSm, &cloudSm, &smearSm })
        s->reset (sr, 0.03);
    cutoffSm.reset (sr, 0.05);
    filterBypass.reset (sr, 0.05);
    filterBypass.setCurrentAndTargetValue (p[pFilterType]->load() < 0.5f && p[pCutoff]->load() > 19500.0f ? 1.0f : 0.0f);
    mixSm.setCurrentAndTargetValue (p[pMix]->load());
    gainSm.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (p[pGain]->load()));
    cloudSm.setCurrentAndTargetValue (p[pCloudAmt]->load());
    smearSm.setCurrentAndTargetValue (p[pSmearAmt]->load());
    cutoffSm.setCurrentAndTargetValue (p[pCutoff]->load());
}

void MutagenProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numIn = getTotalNumInputChannels();
    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    for (int ch = numIn; ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);
    if (numCh == 0 || numSamples == 0) return;

    if (dryBuffer.getNumSamples() < numSamples || dryBuffer.getNumChannels() < numCh)
        dryBuffer.setSize (numCh, numSamples, false, false, true);
    for (int ch = 0; ch < numCh; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);

    inputLevel.store (buffer.getMagnitude (0, numSamples));

    // ---- parameters
    const bool seqOn = p[pSeqOn]->load() > 0.5f;
    static constexpr double stepsPerBeatTable[] = { 1.0, 2.0, 4.0, 8.0 };
    const double stepsPerBeat = stepsPerBeatTable[juce::jlimit (0, 3, (int) p[pRate]->load())];

    cloud.setParameters (p[pCloudSize]->load(), p[pCloudDensity]->load(), p[pCloudSpread]->load(),
                         (int) p[pChord]->load(), (int) p[pOctave]->load() - 2);

    const float blur = p[pSmearBlur]->load();
    for (auto& s : smear)
        s.setParameters (blur, p[pFreeze]->load() > 0.5f, 0.12f * blur);

    const int filterType = (int) p[pFilterType]->load();
    if (filterType != lastFilterType)
    {
        using T = juce::dsp::StateVariableTPTFilterType;
        filter.setType (filterType == 0 ? T::lowpass : filterType == 1 ? T::bandpass : T::highpass);
        lastFilterType = filterType;
    }
    filter.setResonance (p[pReso]->load());

    mixSm.setTargetValue (p[pMix]->load());
    gainSm.setTargetValue (juce::Decibels::decibelsToGain (p[pGain]->load()));
    cloudSm.setTargetValue (p[pCloudAmt]->load());
    smearSm.setTargetValue (p[pSmearAmt]->load());
    cutoffSm.setTargetValue (juce::jmin (p[pCutoff]->load(), (float) (sr * 0.45)));
    // a fully opened low-pass fades itself out so the clean sound stays untouched
    filterBypass.setTargetValue (filterType == 0
                                     ? juce::jlimit (0.0f, 1.0f, (p[pCutoff]->load() - 14000.0f) / 5500.0f)
                                     : 0.0f);

    // ---- musical clock (host transport, or free-running when stopped)
    double bpm = 120.0, ppq = internalPpq;
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = juce::jlimit (20.0, 999.0, *b);
            if (pos->getIsPlaying())
                if (auto q = pos->getPpqPosition()) ppq = *q;
        }
    }

    const double samplesPerStep = (60.0 / bpm) * sr / stepsPerBeat;
    const double ppqPerSample = bpm / (60.0 * sr);

    auto* outL = buffer.getWritePointer (0);
    auto* outR = numCh > 1 ? buffer.getWritePointer (1) : nullptr;
    const auto* inL = dryBuffer.getReadPointer (0);
    const auto* inR = numCh > 1 ? dryBuffer.getReadPointer (1) : inL;

    for (int n = 0; n < numSamples; ++n)
    {
        const double ppqNow = ppq + (double) n * ppqPerSample;
        const auto stepIndex = (juce::int64) std::floor (ppqNow * stepsPerBeat + 1.0e-7);
        const int patternIndex = (int) (((stepIndex % numSteps) + numSteps) % numSteps);
        const int mode = seqOn ? (int) stepParams[(size_t) patternIndex]->load() : (int) mutagen::Pass;

        const float xL = inL[n], xR = inR[n];
        sequencer.pushInput (0, xL);
        if (numCh > 1) sequencer.pushInput (1, xR);
        sequencer.advanceClock (stepIndex, samplesPerStep, mode);

        float sL = sequencer.process (0, xL);
        float sR = numCh > 1 ? sequencer.process (1, xR) : sL;
        sequencer.endSample();

        // granular chord cloud
        float cL, cR;
        const float c = cloudSm.getNextValue();
        cloud.process (sL, sR, cL, cR, c > 0.0f || cloudSm.isSmoothing());
        sL = sL * (1.0f - c) + cL * c;
        sR = sR * (1.0f - c) + cR * c;

        // spectral smear
        const float m = smearSm.getNextValue();
        const bool smearActive = m > 0.0f || smearSm.isSmoothing();
        const float zL = smear[0].process (sL, smearActive);
        const float zR = smear[1].process (sR, smearActive);
        sL = sL * (1.0f - m) + zL * m * 1.4f;
        sR = sR * (1.0f - m) + zR * m * 1.4f;

        // filter
        if ((n & 15) == 0)
            filter.setCutoffFrequency (cutoffSm.getNextValue());
        else
            cutoffSm.getNextValue();

        const float fL = filter.processSample (0, sL);
        const float fR = outR != nullptr ? filter.processSample (1, sR) : 0.0f;
        const float b = filterBypass.getNextValue();
        outL[n] = fL + (sL - fL) * b;
        if (outR != nullptr) outR[n] = fR + (sR - fR) * b;
    }
    filter.snapToZero();

    // ---- reverb
    const float verbMix = p[pVerbMix]->load();
    juce::Reverb::Parameters rp;
    rp.roomSize = 0.3f + 0.69f * p[pVerbSize]->load();
    rp.damping = 0.45f;
    // juce::Reverb scales wet by 3 and dry by 2 internally
    rp.wetLevel = verbMix * 0.3f;
    rp.dryLevel = 0.5f * (1.0f - 0.4f * verbMix);
    rp.width = 1.0f;
    reverb.setParameters (rp);
    if (verbMix > 0.0001f)
    {
        if (outR != nullptr) reverb.processStereo (outL, outR, numSamples);
        else                 reverb.processMono (outL, numSamples);
    }

    // ---- dry/wet + output gain + safety
    for (int n = 0; n < numSamples; ++n)
    {
        const float mix = mixSm.getNextValue();
        const float g = gainSm.getNextValue();
        for (int ch = 0; ch < numCh; ++ch)
        {
            auto* d = buffer.getWritePointer (ch);
            float y = (dryBuffer.getSample (ch, n) * (1.0f - mix) + d[n] * mix) * g;
            if (! std::isfinite (y)) y = 0.0f;
            d[n] = juce::jlimit (-4.0f, 4.0f, y);
        }
    }

    outputLevel.store (buffer.getMagnitude (0, numSamples));
    internalPpq = ppq + (double) numSamples * ppqPerSample;
    currentStep.store (sequencer.getCurrentStepIndexInPattern (numSteps));
}

//==============================================================================
void MutagenProcessor::setParam (const juce::String& id, float realValue)
{
    if (auto* prm = apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 (realValue));
        prm->endChangeGesture();
    }
}

float MutagenProcessor::getParam (const juce::String& id) const
{
    if (auto* v = apvts.getRawParameterValue (id)) return v->load();
    return 0.0f;
}

void MutagenProcessor::randomisePattern()
{
    // weights for: Pass, Stutter, Reverse, Half, Oct+, Freeze, TapeStop, Gate, Scatter
    const float w[] = { 4.0f, 2.0f, 2.0f, 1.5f, 1.0f, 1.0f, 0.7f, 1.0f, 2.0f };
    float total = 0.0f;
    for (auto x : w) total += x;

    for (int i = 0; i < numSteps; ++i)
    {
        float r = uiRandom.nextFloat() * total;
        int mode = 0;
        for (int m = 0; m < mutagen::NumStepModes; ++m)
        {
            r -= w[m];
            if (r <= 0.0f) { mode = m; break; }
        }
        // repeat the previous step sometimes so patterns feel more musical
        if (i > 0 && uiRandom.nextFloat() < 0.25f)
            mode = (int) getParam (stepId (i - 1));
        setParam (stepId (i), (float) mode);
    }
}

void MutagenProcessor::randomiseEverything()
{
    randomisePattern();
    auto& r = uiRandom;
    setParam ("rate", (float) (1 + r.nextInt (3)));
    setParam ("cloudAmt", r.nextFloat() < 0.35f ? 0.0f : 0.2f + 0.7f * r.nextFloat());
    setParam ("cloudSize", 30.0f + 450.0f * r.nextFloat() * r.nextFloat());
    setParam ("cloudDensity", 6.0f + 34.0f * r.nextFloat());
    setParam ("cloudSpread", r.nextFloat());
    setParam ("chord", (float) r.nextInt (mutagen::chordNames().size()));
    const float oct[] = { 1.0f, 2.0f, 2.0f, 2.0f, 3.0f };
    setParam ("octave", oct[r.nextInt (5)]);
    setParam ("smearAmt", r.nextFloat() < 0.5f ? 0.0f : 0.15f + 0.55f * r.nextFloat());
    setParam ("smearBlur", 0.2f + 0.7f * r.nextFloat());
    setParam ("freeze", 0.0f);
    setParam ("cutoff", r.nextFloat() < 0.4f ? 20000.0f : 800.0f * std::pow (20.0f, r.nextFloat()));
    setParam ("reso", 0.6f + 1.6f * r.nextFloat());
    setParam ("filterType", r.nextFloat() < 0.8f ? 0.0f : (float) (1 + r.nextInt (2)));
    setParam ("verbMix", 0.5f * r.nextFloat());
    setParam ("verbSize", 0.3f + 0.65f * r.nextFloat());
}

//==============================================================================
void MutagenProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void MutagenProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessorEditor* MutagenProcessor::createEditor()
{
    return new MutagenEditor (*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MutagenProcessor();
}
