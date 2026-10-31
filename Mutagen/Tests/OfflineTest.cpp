// Offline test: runs the plugin's processor over a synthetic chord-progression
// melody with every preset, checks the output is sane and renders WAV demos.
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "../Source/PluginProcessor.h"
#include "../Source/Presets.h"
#include "../Source/PluginEditor.h"
#include <chrono>

static constexpr double SR = 48000.0;

static juce::AudioBuffer<float> makeTestMelody (double seconds)
{
    const int n = (int) (seconds * SR);
    juce::AudioBuffer<float> b (2, n);
    b.clear();

    // Am - F - C - G progression, 2 beats each at 120 BPM, with an arpeggio on top
    const int chords[4][3] = { { 57, 60, 64 }, { 53, 57, 60 }, { 48, 52, 55 }, { 55, 59, 62 } };
    const double beat = 0.5;
    auto hz = [] (int midi) { return 440.0 * std::pow (2.0, (midi - 69) / 12.0); };

    for (int i = 0; i < n; ++i)
    {
        const double t = i / SR;
        const int bar = (int) (t / (2 * beat)) % 4;
        const double tInChord = std::fmod (t, 2 * beat);
        float pad = 0.0f;
        for (int v = 0; v < 3; ++v)
        {
            const double ph = std::fmod (t * hz (chords[bar][v]), 1.0);
            pad += (float) (2.0 * std::abs (2.0 * ph - 1.0) - 1.0) * 0.12f; // triangle
        }
        pad *= (float) std::min (1.0, tInChord * 40.0);

        const int arpStep = (int) (t / (beat / 2));
        const double tInArp = std::fmod (t, beat / 2);
        const int note = chords[bar][arpStep % 3] + 12;
        const double ph = std::fmod (t * hz (note), 1.0);
        const float saw = (float) (2.0 * ph - 1.0) * 0.18f * (float) std::exp (-tInArp * 9.0);

        if (std::getenv ("MUTAGEN_SINE") != nullptr)
        {
            const float s = 0.5f * (float) std::sin (juce::MathConstants<double>::twoPi * 220.0 * t);
            b.setSample (0, i, s);
            b.setSample (1, i, s);
            continue;
        }
        b.setSample (0, i, pad + saw * 0.8f);
        b.setSample (1, i, pad * 0.9f + saw);
    }
    return b;
}

struct Stats { float peak = 0, rms = 0; bool finite = true; double msPerSec = 0; };

static Stats run (MutagenProcessor& proc, const juce::AudioBuffer<float>& input,
                  juce::AudioBuffer<float>& out, bool randomBlocks, int channels = 2)
{
    proc.setPlayConfigDetails (channels, channels, SR, 1024);
    proc.prepareToPlay (SR, 1024);
    out.setSize (channels, input.getNumSamples());
    juce::MidiBuffer midi;
    juce::Random r (7);

    const auto t0 = std::chrono::high_resolution_clock::now();
    int pos = 0;
    while (pos < input.getNumSamples())
    {
        const int bs = std::min (randomBlocks ? 1 + r.nextInt (1024) : 512, input.getNumSamples() - pos);
        juce::AudioBuffer<float> block (channels, bs);
        for (int ch = 0; ch < channels; ++ch)
            block.copyFrom (ch, 0, input, ch, pos, bs);
        proc.processBlock (block, midi);
        for (int ch = 0; ch < channels; ++ch)
            out.copyFrom (ch, pos, block, ch, 0, bs);
        pos += bs;
    }
    const auto t1 = std::chrono::high_resolution_clock::now();

    Stats s;
    s.msPerSec = std::chrono::duration<double, std::milli> (t1 - t0).count() / (input.getNumSamples() / SR);
    double sum = 0;
    for (int ch = 0; ch < channels; ++ch)
        for (int i = 0; i < out.getNumSamples(); ++i)
        {
            const float v = out.getSample (ch, i);
            if (! std::isfinite (v)) s.finite = false;
            s.peak = std::max (s.peak, std::abs (v));
            sum += v * v;
        }
    s.rms = (float) std::sqrt (sum / (channels * (double) out.getNumSamples()));
    return s;
}

static void writeWav (const juce::File& f, const juce::AudioBuffer<float>& b)
{
    f.deleteFile();
    juce::WavAudioFormat wav;
    std::unique_ptr<juce::OutputStream> os (f.createOutputStream().release());
    if (auto* w = wav.createWriterFor (os.get(), SR, (unsigned) b.getNumChannels(), 24, {}, 0))
    {
        os.release();
        std::unique_ptr<juce::AudioFormatWriter> writer (w);
        writer->writeFromAudioSampleBuffer (b, 0, b.getNumSamples());
    }
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::File outDir (argc > 1 ? juce::String (argv[1]) : juce::File::getCurrentWorkingDirectory().getFullPathName());
    outDir.createDirectory();

    if (std::getenv ("MUTAGEN_SNAPSHOT") != nullptr)
    {
        MutagenProcessor proc;
        proc.applyPreset (std::atoi (std::getenv ("MUTAGEN_SNAPSHOT")));
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::PNGImageFormat png;
        auto f = outDir.getChildFile ("ui.png");
        f.deleteFile();
        juce::FileOutputStream os (f);
        png.writeImageToStream (img, os);
        std::cout << "snapshot written" << std::endl;
        return 0;
    }

    int failures = 0;
    auto check = [&failures] (bool ok, const juce::String& what)
    {
        std::cout << (ok ? "  [ OK ] " : "  [FAIL] ") << what << std::endl;
        if (! ok) ++failures;
    };

    const auto input = makeTestMelody (16.0);
    writeWav (outDir.getChildFile ("00_input.wav"), input);

    // 1) Spectral smear reconstruction gain (blur ~ 0 should keep the level)
    {
        mutagen::SpectralSmear sm;
        sm.prepare (SR);
        sm.setParameters (0.0f, false, 0.0f);
        double inE = 0, outE = 0;
        for (int i = 0; i < input.getNumSamples(); ++i)
        {
            const float y = sm.process (input.getSample (0, i));
            if (i > 8192) { inE += input.getSample (0, i) * input.getSample (0, i); outE += y * y; }
        }
        const double ratio = std::sqrt (outE / inE);
        std::cout << "Smear level ratio (should be ~1): " << ratio << std::endl;
        check (ratio > 0.6 && ratio < 1.5, "spectral smear keeps level");
    }

    // 2) Clean preset should pass audio through unchanged
    {
        MutagenProcessor proc;
        proc.applyPreset (0);
        juce::AudioBuffer<float> out;
        run (proc, input, out, false);
        double err = 0, ref = 0;
        for (int i = 0; i < input.getNumSamples(); ++i)
        {
            const double d = out.getSample (0, i) - input.getSample (0, i);
            err += d * d; ref += input.getSample (0, i) * input.getSample (0, i);
        }
        const double snr = 10.0 * std::log10 (ref / std::max (err, 1e-20));
        std::cout << "Init preset pass-through SNR: " << snr << " dB" << std::endl;
        check (snr > 30.0, "init preset is (almost) transparent");
    }

    // 3) Every preset
    const auto names = mutagen::presetNames();
    for (int pi = 0; pi < names.size(); ++pi)
    {
        MutagenProcessor proc;
        proc.applyPreset (pi);
        juce::AudioBuffer<float> out;
        const auto s = run (proc, input, out, false);
        std::cout << "Preset " << pi << " '" << names[pi] << "': peak " << s.peak << ", rms " << s.rms
                  << ", CPU " << s.msPerSec << " ms per second of audio" << std::endl;
        check (s.finite, names[pi] + ": no NaN/inf");
        check (s.peak < 3.5f, names[pi] + ": peak sane");
        check (s.rms > 0.005f, names[pi] + ": produces sound");
        writeWav (outDir.getChildFile (juce::String (pi + 1).paddedLeft ('0', 2) + "_"
                                       + names[pi].replaceCharacters (" ()-", "____") + ".wav"), out);
    }

    if (std::getenv ("MUTAGEN_ISOLATE") != nullptr)
    {
        struct V { const char* name; std::vector<std::pair<const char*, float>> sets; };
        std::vector<V> vs = {
            { "iso_seq_only",   { { "cloudAmt", 0 }, { "smearAmt", 0 }, { "verbMix", 0 } } },
            { "iso_cloud_only", { { "seqOn", 0 }, { "smearAmt", 0 }, { "verbMix", 0 }, { "cloudAmt", 1 } } },
            { "iso_cloud_unison", { { "seqOn", 0 }, { "smearAmt", 0 }, { "verbMix", 0 }, { "cloudAmt", 1 }, { "chord", 0 }, { "octave", 2 } } },
            { "iso_smear_only", { { "seqOn", 0 }, { "cloudAmt", 0 }, { "verbMix", 0 }, { "smearAmt", 1 } } },
            { "iso_seq_noverb", { { "cloudAmt", 0 }, { "smearAmt", 0 }, { "verbMix", 0 }, { "cutoff", 20000 }, { "filterType", 0 } } },
            { "iso_verb_only",  { { "seqOn", 0 }, { "cloudAmt", 0 }, { "smearAmt", 0 } } },
        };
        for (auto& v : vs)
        {
            MutagenProcessor proc;
            proc.applyPreset (std::atoi (std::getenv ("MUTAGEN_ISOLATE")));
            for (auto& s : v.sets) proc.setParam (s.first, s.second);
            juce::AudioBuffer<float> out;
            run (proc, input, out, false);
            writeWav (outDir.getChildFile (juce::String (v.name) + ".wav"), out);
        }
    }

    // 4) Dice + random block sizes + mono
    {
        MutagenProcessor proc;
        juce::AudioBuffer<float> out;
        for (int k = 0; k < 6; ++k)
        {
            proc.randomiseEverything();
            const auto s = run (proc, input, out, true);
            check (s.finite && s.peak < 3.5f, "dice-all run " + juce::String (k) + " with random block sizes (peak "
                                                  + juce::String (s.peak, 2) + ")");
            if (k == 0) writeWav (outDir.getChildFile ("99_dice_random.wav"), out);
        }
        juce::AudioBuffer<float> mono (1, input.getNumSamples());
        mono.copyFrom (0, 0, input, 0, 0, input.getNumSamples());
        proc.applyPreset (5);
        const auto s = run (proc, mono, out, true, 1);
        check (s.finite && s.rms > 0.005f, "mono layout works");
    }

    // 5) State save / load round trip
    {
        MutagenProcessor a, b;
        a.applyPreset (3);
        juce::MemoryBlock mb;
        a.getStateInformation (mb);
        b.setStateInformation (mb.getData(), (int) mb.getSize());
        bool same = true;
        for (auto* prm : a.getParameters())
            if (auto* wp = dynamic_cast<juce::RangedAudioParameter*> (prm))
                if (std::abs (wp->getValue() - b.apvts.getParameter (wp->getParameterID())->getValue()) > 1e-5f)
                    same = false;
        check (same, "state save/restore round trip");
    }

    std::cout << (failures == 0 ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << std::endl;
    return failures == 0 ? 0 : 1;
}
