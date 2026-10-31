#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>
#include <cmath>

namespace mutagen
{

//==============================================================================
// Circular buffer addressed by an ever-growing 64-bit sample counter.
struct RingBuffer
{
    void setSize (int sizePowerOfTwo)
    {
        data.assign ((size_t) sizePowerOfTwo, 0.0f);
        mask = (juce::int64) sizePowerOfTwo - 1;
    }

    void clear()                                  { std::fill (data.begin(), data.end(), 0.0f); }
    juce::int64 size() const                      { return mask + 1; }
    void write (juce::int64 index, float v)       { data[(size_t) (index & mask)] = v; }
    float at (juce::int64 index) const            { return data[(size_t) (index & mask)]; }

    // 4-point Hermite interpolation
    float read (double pos) const
    {
        const auto i = (juce::int64) std::floor (pos);
        const auto f = (float) (pos - (double) i);
        const float y0 = at (i - 1), y1 = at (i), y2 = at (i + 1), y3 = at (i + 2);
        const float c1 = 0.5f * (y2 - y0);
        const float c2 = y0 - 2.5f * y1 + 2.0f * y2 - 0.5f * y3;
        const float c3 = 0.5f * (y3 - y0) + 1.5f * (y1 - y2);
        return ((c3 * f + c2) * f + c1) * f + y1;
    }

    std::vector<float> data;
    juce::int64 mask = 0;
};

//==============================================================================
enum StepMode
{
    Pass = 0, Stutter, Reverse, HalfSpeed, OctaveUp, Freeze, TapeStop, Gate, Scatter,
    NumStepModes
};

inline juce::StringArray stepModeShortNames()
{
    return { "PASS", "STUT", "REV", "HALF", "OCT+", "FRZ", "STOP", "GATE", "SCAT" };
}

inline juce::StringArray stepModeLongNames()
{
    return { "Pass (dry)", "Stutter (ratchet)", "Reverse", "Half speed (-12)",
             "Double speed (+12)", "Freeze (hold)", "Tape stop", "Gate (mute)", "Scatter (jump back)" };
}

//==============================================================================
// Gross-Beat-style step sequencer. Every step of the grid chooses how the
// recorded audio is played back: live, reversed, slowed, frozen, etc.
class StepSequencer
{
public:
    static constexpr int bufferSize = 1 << 21; // ~43 s at 48 kHz

    void prepare (double sampleRate, int numChannels)
    {
        sr = sampleRate;
        rings.resize ((size_t) numChannels);
        for (auto& r : rings) r.setSize (bufferSize);
        writeCount = bufferSize;  // keep indices positive
        currentStepIndex = -1;
        fadeLen = juce::jmax (16, (int) (0.004 * sr));
        fadeRemaining = 0;
    }

    void reset()
    {
        for (auto& r : rings) r.clear();
        currentStepIndex = -1;
        fadeRemaining = 0;
    }

    // Writes one input frame. Must be called once per sample before process().
    void pushInput (int channel, float v)  { rings[(size_t) channel].write (writeCount, v); }

    // Call once per sample (after pushInput for all channels) to check the grid.
    void advanceClock (juce::int64 stepIndex, double stepLenSamples, int mode)
    {
        if (stepIndex != currentStepIndex)
        {
            previous = current;
            fadeRemaining = (currentStepIndex >= 0) ? fadeLen : 0;

            current.start = writeCount;
            current.length = juce::jmax (64.0, stepLenSamples);
            current.mode = mode;
            current.scatterBack = 1 + random.nextInt (8);
            current.stutterDiv = 2 + random.nextInt (3);
            currentStepIndex = stepIndex;
        }
    }

    float process (int channel, float liveInput) const
    {
        const auto& ring = rings[(size_t) channel];
        float out = render (current, ring, liveInput);

        if (fadeRemaining > 0)
        {
            const float g = (float) fadeRemaining / (float) fadeLen; // 1 -> 0
            out = out * (1.0f - g) + render (previous, ring, liveInput) * g;
        }
        return out;
    }

    // Call once per sample after process() on all channels.
    void endSample()
    {
        ++writeCount;
        if (fadeRemaining > 0) --fadeRemaining;
    }

    int getCurrentStepIndexInPattern (int patternLength) const
    {
        if (currentStepIndex < 0) return -1;
        return (int) (currentStepIndex % patternLength);
    }

private:
    struct Voice
    {
        juce::int64 start = 0;
        double length = 1000.0;
        int mode = Pass;
        int scatterBack = 1;
        int stutterDiv = 2;
    };

    static float edgeFade (double pos, double len, double fade)
    {
        const double a = juce::jmin (1.0, pos / fade, (len - pos) / fade);
        return (float) juce::jlimit (0.0, 1.0, a);
    }

    float readSafe (const RingBuffer& ring, double pos) const
    {
        const double newest = (double) writeCount - 2.0;
        const double oldest = (double) (writeCount - ring.size() + 8);
        return ring.read (juce::jlimit (oldest, newest, pos));
    }

    float render (const Voice& v, const RingBuffer& ring, float liveInput) const
    {
        const double p = (double) (writeCount - v.start); // samples since voice start
        const double L = v.length;
        const double s = (double) v.start;
        const double fade = (double) fadeLen;

        switch (v.mode)
        {
            case Pass: return liveInput;

            case Stutter:
            {
                const double seg = L / v.stutterDiv;
                const double ph = std::fmod (p, seg);
                return readSafe (ring, s + ph) * edgeFade (ph, seg, fade * 0.5);
            }

            case Reverse:   return readSafe (ring, s - 1.0 - p);
            case HalfSpeed: return readSafe (ring, s + 0.5 * p);
            case OctaveUp:  return readSafe (ring, s - L + 2.0 * p);

            case Freeze:
            {
                const double G = juce::jmin (L * 0.5, 0.09 * sr);
                const double base = s - G - 4.0;
                const double ph1 = std::fmod (p, G) / G;
                const double ph2 = std::fmod (p + 0.5 * G, G) / G;
                const auto w1 = (float) std::sin (juce::MathConstants<double>::pi * ph1);
                const auto w2 = (float) std::sin (juce::MathConstants<double>::pi * ph2);
                return readSafe (ring, base + ph1 * G) * w1 + readSafe (ring, base + ph2 * G) * w2;
            }

            case TapeStop:
            {
                const double t = juce::jmin (p, L);
                const double pos = s - 0.25 * L + t - (t * t) / (2.0 * L);
                const double x = t / L;
                return readSafe (ring, pos) * (float) (1.0 - x * x * x);
            }

            case Gate: return 0.0f;

            case Scatter:
                return readSafe (ring, s - v.scatterBack * L + p);

            default: return liveInput;
        }
    }

    std::vector<RingBuffer> rings;
    juce::int64 writeCount = 0;
    juce::int64 currentStepIndex = -1;
    Voice current, previous;
    int fadeLen = 192, fadeRemaining = 0;
    double sr = 44100.0;
    juce::Random random { 1234 };
};

//==============================================================================
inline juce::StringArray chordNames()
{
    return { "Unison", "Octaves", "Fifths", "Major", "Minor", "Sus4", "Maj7", "Min7", "Add9 Dream", "Dark b9" };
}

inline std::vector<int> chordIntervals (int chord)
{
    switch (chord)
    {
        case 0:  return { 0 };
        case 1:  return { 0, 12, -12 };
        case 2:  return { 0, 7, 12 };
        case 3:  return { 0, 4, 7, 12 };
        case 4:  return { 0, 3, 7, 12 };
        case 5:  return { 0, 5, 7, 12 };
        case 6:  return { 0, 4, 7, 11 };
        case 7:  return { 0, 3, 7, 10 };
        case 8:  return { 0, 7, 14, 16, 19 };
        case 9:  return { 0, 1, 7, 13, -12 };
        default: return { 0 };
    }
}

//==============================================================================
// Granular "chord cloud": sprays short pitched grains taken from the recent
// audio, tuned to a chord. Turns melodies into pads and choirs.
class GrainCloud
{
public:
    static constexpr int bufferSize = 1 << 18; // ~5.4 s at 48 kHz
    static constexpr int maxGrains = 96;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        left.setSize (bufferSize);
        right.setSize (bufferSize);
        writeCount = bufferSize;
        for (auto& g : grains) g.active = false;
        spawnAccumulator = 0.0;

        for (int i = 0; i < windowSize; ++i)
            window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (windowSize - 1));
    }

    void reset()
    {
        left.clear(); right.clear();
        for (auto& g : grains) g.active = false;
    }

    void setParameters (float sizeMs, float densityPerSec, float spread, int chord, int octave)
    {
        grainLen = juce::jlimit (64.0, 0.9 * bufferSize / 4.0, sizeMs * 0.001 * sr);
        density = densityPerSec;
        spreadSeconds = spread * 2.0;
        intervals = chordIntervals (chord);
        octaveShift = octave;
        const double overlap = juce::jmax (1.0, (double) density * grainLen / sr);
        normGain = (float) (1.8 / std::sqrt (overlap));
    }

    void process (float inL, float inR, float& outL, float& outR, bool active = true)
    {
        left.write (writeCount, inL);
        right.write (writeCount, inR);

        if (! active)
        {
            for (auto& g : grains) g.active = false;
            ++writeCount;
            outL = outR = 0.0f;
            return;
        }

        spawnAccumulator += density / sr;
        while (spawnAccumulator >= 1.0)
        {
            spawnAccumulator -= 1.0;
            spawn();
        }

        float sumL = 0.0f, sumR = 0.0f;
        for (auto& g : grains)
        {
            if (! g.active) continue;

            const double pos = g.start + g.ratio * g.age;
            const float w = window[(size_t) juce::jlimit (0, windowSize - 1, (int) (g.age / g.length * (windowSize - 1)))];
            sumL += left.read (pos) * w * g.gainL;
            sumR += right.read (pos) * w * g.gainR;

            g.age += 1.0;
            if (g.age >= g.length) g.active = false;
        }

        ++writeCount;
        outL = softClip (sumL * normGain);
        outR = softClip (sumR * normGain);
    }

private:
    // transparent below ~-3 dBFS, rounds off coherent grain pile-ups above
    static float softClip (float x)
    {
        const float a = std::abs (x);
        if (a <= 0.7f) return x;
        const float y = 0.7f + 0.3f * std::tanh ((a - 0.7f) / 0.3f);
        return x < 0.0f ? -y : y;
    }

    struct Grain
    {
        bool active = false;
        double start = 0.0, ratio = 1.0, length = 1.0, age = 0.0;
        float gainL = 1.0f, gainR = 1.0f;
    };

    void spawn()
    {
        Grain* slot = nullptr;
        for (auto& g : grains)
            if (! g.active) { slot = &g; break; }
        if (slot == nullptr) return;

        const int semis = intervals[(size_t) random.nextInt ((int) intervals.size())] + 12 * octaveShift;
        const double cents = (random.nextDouble() - 0.5) * 12.0; // slight chorus
        const double ratio = std::pow (2.0, (semis + cents / 100.0) / 12.0);
        const double len = grainLen * (0.75 + 0.5 * random.nextDouble());

        // Make sure the grain never reads audio that has not been recorded yet.
        const double minDelay = juce::jmax (0.0, (ratio - 1.0) * len) + 8.0;
        double delay = minDelay + random.nextDouble() * spreadSeconds * sr + 0.01 * sr;
        delay = juce::jmin (delay, (double) bufferSize - ratio * len - 16.0);

        const float pan = (float) random.nextDouble();
        slot->active = true;
        slot->start = (double) writeCount - delay;
        slot->ratio = ratio;
        slot->length = len;
        slot->age = 0.0;
        slot->gainL = std::cos (pan * juce::MathConstants<float>::halfPi) * 1.2f;
        slot->gainR = std::sin (pan * juce::MathConstants<float>::halfPi) * 1.2f;
    }

    static constexpr int windowSize = 1024;
    std::array<float, windowSize> window {};
    std::array<Grain, maxGrains> grains {};
    RingBuffer left, right;
    juce::int64 writeCount = 0;
    double sr = 44100.0, grainLen = 4000.0, spreadSeconds = 0.5, spawnAccumulator = 0.0;
    float density = 10.0f, normGain = 1.0f;
    std::vector<int> intervals { 0 };
    int octaveShift = 0;
    juce::Random random { 987 };
};

//==============================================================================
// Spectral smear / freeze (phase vocoder). Blurs the spectrum over time so
// anything turns into a soft, evolving texture. "Freeze" holds the sound.
class SpectralSmear
{
public:
    static constexpr int order = 11;
    static constexpr int N = 1 << order; // 2048
    static constexpr int hop = N / 4;
    static constexpr int numBins = N / 2 + 1;

    void prepare (double sampleRate)
    {
        sr = sampleRate;
        for (int i = 0; i < N; ++i)
            window[(size_t) i] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) N);
        reset();
    }

    void reset()
    {
        inRing.fill (0.0f);
        outRing.fill (0.0f);
        magSmooth.fill (0.0f);
        deltaSmooth.fill (0.0f);
        prevPhaseIn.fill (0.0f);
        synthPhase.fill (0.0f);
        writeIdx = 0;
        hopCounter = 0;
    }

    void setParameters (float blur, bool freezeOn, float jitter)
    {
        // blur 0..1 -> time constant 20 ms .. 10 s
        const double T = 0.02 * std::pow (500.0, (double) blur);
        smoothCoeff = (float) std::exp (-(double) hop / (sr * T));
        frozen = freezeOn;
        phaseJitter = jitter;
    }

    float process (float x, bool active = true)
    {
        inRing[(size_t) writeIdx] = x;
        const float y = outRing[(size_t) writeIdx];
        outRing[(size_t) writeIdx] = 0.0f;
        writeIdx = (writeIdx + 1) & (N - 1);

        if (++hopCounter >= hop)
        {
            hopCounter = 0;
            if (active) processFrame();
        }
        return y;
    }

private:
    void processFrame()
    {
        for (int i = 0; i < N; ++i)
            fftData[(size_t) i] = inRing[(size_t) ((writeIdx + i) & (N - 1))] * window[(size_t) i];
        std::fill (fftData.begin() + N, fftData.end(), 0.0f);

        fft.performRealOnlyForwardTransform (fftData.data(), true);

        const float twoPi = juce::MathConstants<float>::twoPi;
        const float a = smoothCoeff;

        for (int k = 0; k < numBins; ++k)
        {
            const float re = fftData[(size_t) (2 * k)];
            const float im = fftData[(size_t) (2 * k + 1)];
            const float mag = std::sqrt (re * re + im * im);
            const float phase = std::atan2 (im, re);

            const float expected = twoPi * (float) k * (float) hop / (float) N;
            float d = phase - prevPhaseIn[(size_t) k] - expected;
            d -= twoPi * std::round (d / twoPi);
            prevPhaseIn[(size_t) k] = phase;

            if (! frozen)
            {
                magSmooth[(size_t) k] = a * magSmooth[(size_t) k] + (1.0f - a) * mag;
                deltaSmooth[(size_t) k] = expected + d;
            }

            float sp = synthPhase[(size_t) k] + deltaSmooth[(size_t) k]
                       + phaseJitter * (random.nextFloat() - 0.5f) * twoPi;
            sp -= twoPi * std::floor (sp / twoPi);
            synthPhase[(size_t) k] = sp;

            const float m = magSmooth[(size_t) k];
            fftData[(size_t) (2 * k)]     = m * std::cos (sp);
            fftData[(size_t) (2 * k + 1)] = m * std::sin (sp);
        }

        // mirror negative frequencies (conjugate symmetric)
        for (int k = 1; k < N / 2; ++k)
        {
            fftData[(size_t) (2 * (N - k))]     =  fftData[(size_t) (2 * k)];
            fftData[(size_t) (2 * (N - k) + 1)] = -fftData[(size_t) (2 * k + 1)];
        }

        fft.performRealOnlyInverseTransform (fftData.data());

        // Hann^2 at 75 % overlap sums to 1.5
        const float norm = 1.0f / 1.5f;
        for (int i = 0; i < N; ++i)
            outRing[(size_t) ((writeIdx + i) & (N - 1))] += fftData[(size_t) i] * window[(size_t) i] * norm;
    }

    juce::dsp::FFT fft { order };
    std::array<float, N> window {}, inRing {}, outRing {};
    std::array<float, 2 * N> fftData {};
    std::array<float, numBins> magSmooth {}, deltaSmooth {}, prevPhaseIn {}, synthPhase {};
    int writeIdx = 0, hopCounter = 0;
    double sr = 44100.0;
    float smoothCoeff = 0.5f, phaseJitter = 0.0f;
    bool frozen = false;
    juce::Random random { 42 };
};

} // namespace mutagen
