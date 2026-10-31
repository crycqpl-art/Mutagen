#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

namespace mutagen
{
struct Palette
{
    static juce::Colour bg()     { return juce::Colour (0xff0d0e13); }
    static juce::Colour panel()  { return juce::Colour (0xff161821); }
    static juce::Colour line()   { return juce::Colour (0xff272a38); }
    static juce::Colour text()   { return juce::Colour (0xffe9eaf1); }
    static juce::Colour dim()    { return juce::Colour (0xff8a8fa3); }
    static juce::Colour seq()    { return juce::Colour (0xffff4fa3); }
    static juce::Colour cloud()  { return juce::Colour (0xff45d4ff); }
    static juce::Colour smear()  { return juce::Colour (0xff9b7bff); }
    static juce::Colour tone()   { return juce::Colour (0xffffb547); }
    static juce::Colour space()  { return juce::Colour (0xff5be3a1); }

    static juce::Colour mode (int m)
    {
        static const juce::uint32 c[] = { 0xff2a2d3a, 0xffff4fa3, 0xff45d4ff, 0xff5b8cff, 0xffffe14d,
                                          0xff9b7bff, 0xffff8a3d, 0xff0a0b10, 0xff5be3a1 };
        return juce::Colour (c[juce::jlimit (0, 8, m)]);
    }
};

class MutagenLookAndFeel : public juce::LookAndFeel_V4
{
public:
    MutagenLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool over, bool down) override;
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool over, bool down) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getTextButtonFont (juce::TextButton&, int) override;
    juce::Label* createSliderTextBox (juce::Slider&) override;
};

//==============================================================================
class StepGrid : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit StepGrid (MutagenProcessor&);
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;

private:
    int cellAt (juce::Point<int>) const;
    juce::Rectangle<float> cellBounds (int i) const;
    MutagenProcessor& proc;
    int paintMode = -1, lastPainted = -1;
};

//==============================================================================
struct Knob : public juce::Component
{
    Knob (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& name, juce::Colour);
    void resized() override;
    juce::Slider slider;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
};

struct Choice : public juce::Component
{
    Choice (juce::AudioProcessorValueTreeState&, const juce::String& id, const juce::String& name);
    void resized() override;
    juce::ComboBox box;
    juce::Label label;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
};

} // namespace mutagen

//==============================================================================
class MutagenEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit MutagenEditor (MutagenProcessor&);
    ~MutagenEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawPanel (juce::Graphics&, juce::Rectangle<int>, const juce::String& title, juce::Colour) const;

    MutagenProcessor& proc;
    mutagen::MutagenLookAndFeel lnf;
    juce::TooltipWindow tooltips { this, 600 };

    mutagen::StepGrid grid;
    juce::ComboBox presetBox;
    juce::TextButton dicePattern { "DICE PATTERN" }, diceAll { "DICE ALL" };
    juce::ToggleButton seqOn { "SEQ ON" }, freeze { "FREEZE" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> seqOnAtt, freezeAtt;
    mutagen::Choice rate;

    mutagen::Knob cloudAmt, cloudSize, cloudDensity, cloudSpread;
    mutagen::Choice chord, octave;
    mutagen::Knob smearAmt, smearBlur;
    mutagen::Knob cutoff, reso;
    mutagen::Choice filterType;
    mutagen::Knob verbMix, verbSize, mix, gain;

    juce::Rectangle<int> seqPanel, cloudPanel, smearPanel, tonePanel, outPanel;
    float meterIn = 0.0f, meterOut = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MutagenEditor)
};
