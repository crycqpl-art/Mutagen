#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP.h"

class MutagenProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numSteps = 16;

    MutagenProcessor();
    ~MutagenProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Mutagen"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 6.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::String stepId (int i) { return "step" + juce::String (i); }
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Helpers used by the UI (message thread)
    void setParam (const juce::String& id, float realValue);
    float getParam (const juce::String& id) const;
    void randomisePattern();
    void randomiseEverything();
    void applyPreset (int index);

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> currentStep { -1 };
    std::atomic<float> inputLevel { 0.0f }, outputLevel { 0.0f };

private:
    mutagen::StepSequencer sequencer;
    mutagen::GrainCloud cloud;
    std::array<mutagen::SpectralSmear, 2> smear;
    juce::dsp::StateVariableTPTFilter<float> filter;
    juce::Reverb reverb;
    juce::AudioBuffer<float> dryBuffer;

    juce::SmoothedValue<float> mixSm, gainSm, cloudSm, smearSm, cutoffSm, filterBypass;
    double internalPpq = 0.0;
    double sr = 44100.0;
    int lastFilterType = -1;

    std::array<std::atomic<float>*, numSteps> stepParams {};
    std::atomic<float>* p[32] {};
    enum { pSeqOn, pRate, pCloudAmt, pCloudSize, pCloudDensity, pCloudSpread, pChord, pOctave,
           pSmearAmt, pSmearBlur, pFreeze, pCutoff, pReso, pFilterType, pVerbMix, pVerbSize,
           pMix, pGain, pNumParams };

    juce::Random uiRandom;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MutagenProcessor)
};
