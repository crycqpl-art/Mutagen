#include "PluginEditor.h"
#include "Presets.h"

using namespace mutagen;

static juce::Font uiFont (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

//==============================================================================
MutagenLookAndFeel::MutagenLookAndFeel()
{
    setColour (juce::ComboBox::backgroundColourId, Palette::panel().brighter (0.06f));
    setColour (juce::ComboBox::outlineColourId, Palette::line());
    setColour (juce::ComboBox::textColourId, Palette::text());
    setColour (juce::ComboBox::arrowColourId, Palette::dim());
    setColour (juce::PopupMenu::backgroundColourId, Palette::panel());
    setColour (juce::PopupMenu::textColourId, Palette::text());
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Palette::seq().withAlpha (0.35f));
    setColour (juce::PopupMenu::highlightedTextColourId, Palette::text());
    setColour (juce::TextButton::buttonColourId, Palette::panel().brighter (0.08f));
    setColour (juce::TextButton::textColourOffId, Palette::text());
    setColour (juce::Label::textColourId, Palette::dim());
    setColour (juce::Slider::textBoxTextColourId, Palette::text());
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TooltipWindow::backgroundColourId, Palette::panel());
}

void MutagenLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                           float startAngle, float endAngle, juce::Slider& s)
{
    const auto accent = s.findColour (juce::Slider::rotarySliderFillColourId);
    const auto bounds = juce::Rectangle<int> (x, y, w, h).toFloat().reduced (6.0f);
    const float radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto c = bounds.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float arcR = radius - 3.0f;

    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (Palette::line());
    g.strokePath (track, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);
    g.setColour (accent.withAlpha (0.25f));
    g.strokePath (value, juce::PathStrokeType (9.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (accent);
    g.strokePath (value, juce::PathStrokeType (4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float knobR = arcR - 9.0f;
    g.setGradientFill (juce::ColourGradient (Palette::panel().brighter (0.25f), c.x, c.y - knobR,
                                             Palette::bg(), c.x, c.y + knobR, false));
    g.fillEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f);
    g.setColour (Palette::line().brighter (0.2f));
    g.drawEllipse (c.x - knobR, c.y - knobR, knobR * 2.0f, knobR * 2.0f, 1.0f);

    const juce::Point<float> tip (c.x + (knobR - 4.0f) * std::sin (angle), c.y - (knobR - 4.0f) * std::cos (angle));
    const juce::Point<float> mid (c.x + knobR * 0.35f * std::sin (angle), c.y - knobR * 0.35f * std::cos (angle));
    g.setColour (Palette::text());
    g.drawLine ({ mid, tip }, 2.5f);
}

void MutagenLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour& bgc,
                                               bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    auto c = bgc;
    if (down) c = c.brighter (0.2f);
    else if (over) c = c.brighter (0.1f);
    g.setColour (c);
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (b.findColour (juce::ComboBox::outlineColourId).brighter (over ? 0.4f : 0.0f));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);
}

void MutagenLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool over, bool)
{
    const auto accent = b.findColour (juce::ToggleButton::tickColourId);
    auto r = b.getLocalBounds().toFloat().reduced (1.0f);
    const bool on = b.getToggleState();
    g.setColour (on ? accent.withAlpha (0.22f) : Palette::panel().brighter (over ? 0.12f : 0.06f));
    g.fillRoundedRectangle (r, r.getHeight() * 0.5f);
    g.setColour (on ? accent : Palette::line().brighter (0.3f));
    g.drawRoundedRectangle (r, r.getHeight() * 0.5f, 1.2f);

    const float d = r.getHeight() * 0.36f;
    auto dot = juce::Rectangle<float> (d, d).withCentre ({ r.getX() + r.getHeight() * 0.5f + 2.0f, r.getCentreY() });
    g.setColour (on ? accent : Palette::dim().withAlpha (0.5f));
    g.fillEllipse (dot);

    g.setColour (on ? Palette::text() : Palette::dim());
    g.setFont (uiFont (13.0f, true));
    g.drawText (b.getButtonText(), r.withTrimmedLeft (r.getHeight() * 0.8f), juce::Justification::centred);
}

void MutagenLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<int> (0, 0, w, h).toFloat().reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (box.findColour (juce::ComboBox::outlineColourId));
    g.drawRoundedRectangle (r, 6.0f, 1.0f);

    juce::Path arrow;
    const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (box.findColour (juce::ComboBox::arrowColourId));
    g.fillPath (arrow);
}

juce::Font MutagenLookAndFeel::getComboBoxFont (juce::ComboBox&)          { return uiFont (14.0f); }
juce::Font MutagenLookAndFeel::getTextButtonFont (juce::TextButton&, int) { return uiFont (13.0f, true); }

juce::Label* MutagenLookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setFont (uiFont (12.5f));
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::textColourId, Palette::text().withAlpha (0.85f));
    return l;
}

//==============================================================================
Knob::Knob (juce::AudioProcessorValueTreeState& st, const juce::String& id, const juce::String& name, juce::Colour c)
{
    slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 18);
    slider.setColour (juce::Slider::rotarySliderFillColourId, c);
    slider.setPopupDisplayEnabled (false, false, nullptr);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxTextColourId, Palette::text().withAlpha (0.85f));
    addAndMakeVisible (slider);

    label.setText (name.toUpperCase(), juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (uiFont (11.5f, true));
    addAndMakeVisible (label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (st, id, slider);
}

void Knob::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (16));
    slider.setBounds (r);
}

Choice::Choice (juce::AudioProcessorValueTreeState& st, const juce::String& id, const juce::String& name)
{
    if (auto* prm = dynamic_cast<juce::AudioParameterChoice*> (st.getParameter (id)))
        box.addItemList (prm->choices, 1);
    box.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (box);

    label.setText (name.toUpperCase(), juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    label.setFont (uiFont (11.5f, true));
    addAndMakeVisible (label);

    attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (st, id, box);
}

void Choice::resized()
{
    auto r = getLocalBounds();
    label.setBounds (r.removeFromTop (16));
    box.setBounds (r.withSizeKeepingCentre (r.getWidth(), juce::jmin (r.getHeight(), 28)));
}

//==============================================================================
StepGrid::StepGrid (MutagenProcessor& p) : proc (p)
{
    setTooltip ("Click: next mode  |  Right-click: choose mode  |  Shift+click: PASS  |  Drag: paint");
}

juce::Rectangle<float> StepGrid::cellBounds (int i) const
{
    const float gap = 6.0f;
    const float w = ((float) getWidth() - gap * (MutagenProcessor::numSteps - 1)) / (float) MutagenProcessor::numSteps;
    const float x = (float) i * (w + gap);
    return { x, 0.0f, w, (float) getHeight() };
}

int StepGrid::cellAt (juce::Point<int> pt) const
{
    for (int i = 0; i < MutagenProcessor::numSteps; ++i)
        if (cellBounds (i).expanded (3.0f, 0.0f).contains (pt.toFloat()))
            return i;
    return -1;
}

void StepGrid::paint (juce::Graphics& g)
{
    const int playing = proc.currentStep.load();
    const bool seqOn = proc.getParam ("seqOn") > 0.5f;
    const auto names = stepModeShortNames();

    for (int i = 0; i < MutagenProcessor::numSteps; ++i)
    {
        const int mode = (int) proc.getParam (MutagenProcessor::stepId (i));
        auto r = cellBounds (i);
        auto col = Palette::mode (mode);
        if (! seqOn) col = col.withSaturation (0.0f).withAlpha (0.5f);

        // beat shading
        g.setColour (Palette::panel().brighter ((i / 4) % 2 == 0 ? 0.05f : 0.0f));
        g.fillRoundedRectangle (r, 6.0f);

        auto fill = r.reduced (2.0f);
        if (mode == Pass)
        {
            g.setColour (col);
            g.fillRoundedRectangle (fill, 5.0f);
        }
        else if (mode == Gate)
        {
            g.setColour (Palette::bg());
            g.fillRoundedRectangle (fill, 5.0f);
            g.setColour (Palette::dim().withAlpha (0.5f));
            g.drawRoundedRectangle (fill, 5.0f, 1.0f);
        }
        else
        {
            g.setGradientFill (juce::ColourGradient (col.withAlpha (0.95f), fill.getX(), fill.getY(),
                                                     col.withAlpha (0.45f), fill.getX(), fill.getBottom(), false));
            g.fillRoundedRectangle (fill, 5.0f);
        }

        if (i == playing)
        {
            g.setColour (juce::Colours::white.withAlpha (0.9f));
            g.drawRoundedRectangle (r.reduced (0.5f), 6.0f, 2.5f);
            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.fillRoundedRectangle (r, 6.0f);
        }

        g.setColour (mode == Pass || mode == Gate ? Palette::dim() : Palette::bg().withAlpha (0.85f));
        g.setFont (uiFont (10.5f, true));
        g.drawText (juce::String (i + 1), fill.removeFromTop (20.0f), juce::Justification::centred);

        g.setColour (mode == Pass || mode == Gate ? Palette::text().withAlpha (0.8f) : Palette::bg());
        g.setFont (uiFont (r.getWidth() < 50.0f ? 11.0f : 12.5f, true));
        g.drawText (names[mode], r.withTrimmedTop (20.0f).withTrimmedBottom (12.0f), juce::Justification::centred);
    }
}

void StepGrid::mouseDown (const juce::MouseEvent& e)
{
    const int i = cellAt (e.getPosition());
    if (i < 0) return;
    const auto id = MutagenProcessor::stepId (i);
    const int cur = (int) proc.getParam (id);

    if (e.mods.isPopupMenu())
    {
        juce::PopupMenu m;
        const auto names = stepModeLongNames();
        for (int k = 0; k < NumStepModes; ++k)
            m.addColouredItem (k + 1, names[k], Palette::mode (k == Gate ? 0 : k).brighter (0.2f), true, k == cur);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this),
                         [this, id] (int res) { if (res > 0) { proc.setParam (id, (float) (res - 1)); repaint(); } });
        paintMode = -1;
        return;
    }

    paintMode = e.mods.isShiftDown() ? (int) Pass : (cur + 1) % NumStepModes;
    proc.setParam (id, (float) paintMode);
    lastPainted = i;
    repaint();
}

void StepGrid::mouseDrag (const juce::MouseEvent& e)
{
    if (paintMode < 0) return;
    const int i = cellAt (e.getPosition());
    if (i >= 0 && i != lastPainted)
    {
        proc.setParam (MutagenProcessor::stepId (i), (float) paintMode);
        lastPainted = i;
        repaint();
    }
}

//==============================================================================
MutagenEditor::MutagenEditor (MutagenProcessor& p)
    : AudioProcessorEditor (&p), proc (p), grid (p),
      rate (p.apvts, "rate", "Rate"),
      cloudAmt (p.apvts, "cloudAmt", "Amount", Palette::cloud()),
      cloudSize (p.apvts, "cloudSize", "Grain Size", Palette::cloud()),
      cloudDensity (p.apvts, "cloudDensity", "Density", Palette::cloud()),
      cloudSpread (p.apvts, "cloudSpread", "Spread", Palette::cloud()),
      chord (p.apvts, "chord", "Chord"),
      octave (p.apvts, "octave", "Octave"),
      smearAmt (p.apvts, "smearAmt", "Amount", Palette::smear()),
      smearBlur (p.apvts, "smearBlur", "Blur", Palette::smear()),
      cutoff (p.apvts, "cutoff", "Cutoff", Palette::tone()),
      reso (p.apvts, "reso", "Resonance", Palette::tone()),
      filterType (p.apvts, "filterType", "Type"),
      verbMix (p.apvts, "verbMix", "Reverb", Palette::space()),
      verbSize (p.apvts, "verbSize", "Room", Palette::space()),
      mix (p.apvts, "mix", "Dry/Wet", Palette::text()),
      gain (p.apvts, "gain", "Output", Palette::text())
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (grid);

    presetBox.addItemList (presetNames(), 1);
    presetBox.setTextWhenNothingSelected ("Presets...");
    presetBox.onChange = [this]
    {
        const int idx = presetBox.getSelectedItemIndex();
        if (idx >= 0) proc.applyPreset (idx);
    };
    addAndMakeVisible (presetBox);

    dicePattern.setColour (juce::TextButton::buttonColourId, Palette::seq().withAlpha (0.22f));
    dicePattern.setColour (juce::ComboBox::outlineColourId, Palette::seq());
    dicePattern.setTooltip ("Random step pattern");
    dicePattern.onClick = [this] { proc.randomisePattern(); presetBox.setSelectedItemIndex (-1, juce::dontSendNotification); };
    addAndMakeVisible (dicePattern);

    diceAll.setColour (juce::TextButton::buttonColourId, Palette::cloud().withAlpha (0.18f));
    diceAll.setColour (juce::ComboBox::outlineColourId, Palette::cloud());
    diceAll.setTooltip ("Randomise everything (pattern, cloud, smear, filter, reverb)");
    diceAll.onClick = [this] { proc.randomiseEverything(); presetBox.setSelectedItemIndex (-1, juce::dontSendNotification); };
    addAndMakeVisible (diceAll);

    seqOn.setColour (juce::ToggleButton::tickColourId, Palette::seq());
    freeze.setColour (juce::ToggleButton::tickColourId, Palette::smear());
    addAndMakeVisible (seqOn);
    addAndMakeVisible (freeze);
    seqOnAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "seqOn", seqOn);
    freezeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (p.apvts, "freeze", freeze);

    for (juce::Component* c : std::initializer_list<juce::Component*> {
             &rate, &cloudAmt, &cloudSize, &cloudDensity, &cloudSpread, &chord, &octave,
             &smearAmt, &smearBlur, &cutoff, &reso, &filterType, &verbMix, &verbSize, &mix, &gain })
        addAndMakeVisible (c);

    setSize (1000, 640);
    startTimerHz (30);
}

MutagenEditor::~MutagenEditor()
{
    setLookAndFeel (nullptr);
}

void MutagenEditor::timerCallback()
{
    meterIn = juce::jmax (proc.inputLevel.load(), meterIn * 0.85f);
    meterOut = juce::jmax (proc.outputLevel.load(), meterOut * 0.85f);
    grid.repaint();
    repaint (getLocalBounds().removeFromTop (60));
}

void MutagenEditor::drawPanel (juce::Graphics& g, juce::Rectangle<int> r, const juce::String& title, juce::Colour accent) const
{
    auto rf = r.toFloat();
    g.setColour (Palette::panel());
    g.fillRoundedRectangle (rf, 10.0f);
    g.setColour (Palette::line());
    g.drawRoundedRectangle (rf.reduced (0.5f), 10.0f, 1.0f);

    g.setColour (accent);
    g.fillRoundedRectangle (rf.getX() + 14.0f, rf.getY() + 15.0f, 4.0f, 14.0f, 2.0f);
    g.setColour (Palette::text());
    g.setFont (uiFont (14.0f, true));
    g.drawText (title, r.getX() + 26, r.getY() + 10, 300, 24, juce::Justification::centredLeft);
}

void MutagenEditor::paint (juce::Graphics& g)
{
    g.fillAll (Palette::bg());

    // header
    auto header = getLocalBounds().removeFromTop (60).reduced (20, 0);
    g.setGradientFill (juce::ColourGradient (Palette::seq(), (float) header.getX(), 0.0f,
                                             Palette::cloud(), (float) header.getX() + 200.0f, 0.0f, false));
    g.setFont (uiFont (30.0f, true));
    g.drawText ("MUTAGEN", header.removeFromLeft (190), juce::Justification::centredLeft);
    g.setColour (Palette::dim());
    g.setFont (uiFont (12.0f));
    g.drawText ("sample mutation engine", header.removeFromLeft (170).translated (0, 4), juce::Justification::centredLeft);

    // level meters
    auto meters = juce::Rectangle<int> (getWidth() - 20 - 70, 20, 70, 20);
    auto drawMeter = [&g] (juce::Rectangle<int> r, float level, const juce::String& lbl)
    {
        g.setColour (Palette::dim());
        g.setFont (uiFont (10.0f, true));
        g.drawText (lbl, r.removeFromLeft (24), juce::Justification::centredLeft);
        auto bar = r.toFloat().reduced (0.0f, 2.0f);
        g.setColour (Palette::line());
        g.fillRoundedRectangle (bar, 2.0f);
        const float db = juce::Decibels::gainToDecibels (level, -60.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        g.setColour (level > 0.99f ? juce::Colours::red : Palette::space());
        g.fillRoundedRectangle (bar.withWidth (bar.getWidth() * frac), 2.0f);
    };
    drawMeter (meters.removeFromTop (10), meterIn, "IN");
    drawMeter (meters, meterOut, "OUT");

    drawPanel (g, seqPanel, "SEQUENCER", Palette::seq());
    drawPanel (g, cloudPanel, "CHORD CLOUD", Palette::cloud());
    drawPanel (g, smearPanel, "SPECTRAL SMEAR", Palette::smear());
    drawPanel (g, tonePanel, "FILTER", Palette::tone());
    drawPanel (g, outPanel, "SPACE + OUTPUT", Palette::space());

    g.setColour (Palette::dim().withAlpha (0.8f));
    g.setFont (uiFont (11.5f));
    g.drawText ("Click a step to change its mode  |  right-click: pick from list  |  shift-click: PASS  |  drag to paint",
                seqPanel.getX() + 20, seqPanel.getBottom() - 26, seqPanel.getWidth() - 40, 18,
                juce::Justification::centredLeft);
}

void MutagenEditor::resized()
{
    auto area = getLocalBounds().reduced (20, 0);
    auto header = area.removeFromTop (60);

    header.removeFromRight (90); // meters
    diceAll.setBounds (header.removeFromRight (100).withSizeKeepingCentre (100, 30));
    header.removeFromRight (8);
    dicePattern.setBounds (header.removeFromRight (120).withSizeKeepingCentre (120, 30));
    header.removeFromRight (8);
    presetBox.setBounds (header.removeFromRight (180).withSizeKeepingCentre (180, 30));

    seqPanel = area.removeFromTop (230);
    {
        auto r = seqPanel.reduced (16, 0);
        auto top = r.removeFromTop (44).withTrimmedTop (8);
        top.removeFromLeft (120);
        seqOn.setBounds (top.removeFromLeft (100).withSizeKeepingCentre (100, 28));
        top.removeFromLeft (12);
        rate.label.setVisible (false);
        rate.setBounds (top.removeFromLeft (90).withSizeKeepingCentre (90, 44).translated (0, -8));
        r.removeFromBottom (30);
        grid.setBounds (r.withTrimmedTop (4));
    }

    area.removeFromTop (12);
    auto bottom = area.removeFromTop (getHeight() - area.getY() - 20);
    const int gap = 12;
    const int unit = (bottom.getWidth() - gap * 3) / 9;
    cloudPanel = bottom.removeFromLeft (unit * 3);  bottom.removeFromLeft (gap);
    smearPanel = bottom.removeFromLeft (unit * 2);  bottom.removeFromLeft (gap);
    tonePanel  = bottom.removeFromLeft (unit * 2);  bottom.removeFromLeft (gap);
    outPanel   = bottom;

    auto layoutRows = [] (juce::Rectangle<int> panel, std::vector<std::vector<juce::Component*>> rows)
    {
        auto r = panel.reduced (10, 0).withTrimmedTop (40).withTrimmedBottom (10);
        const int rowH = r.getHeight() / (int) rows.size();
        for (auto& row : rows)
        {
            auto rr = r.removeFromTop (rowH);
            const int w = rr.getWidth() / (int) row.size();
            for (auto* c : row)
            {
                auto cell = rr.removeFromLeft (w).reduced (4, 4);
                if (dynamic_cast<Choice*> (c) != nullptr)
                    cell = cell.withSizeKeepingCentre (cell.getWidth(), 50);
                else if (dynamic_cast<juce::ToggleButton*> (c) != nullptr)
                    cell = cell.withSizeKeepingCentre (juce::jmin (cell.getWidth(), 130), 34);
                c->setBounds (cell);
            }
        }
    };

    layoutRows (cloudPanel, { { &cloudAmt, &cloudSize, &cloudDensity }, { &cloudSpread, &chord, &octave } });
    layoutRows (smearPanel, { { &smearAmt, &smearBlur }, { &freeze } });
    layoutRows (tonePanel,  { { &cutoff, &reso }, { &filterType } });
    layoutRows (outPanel,   { { &verbMix, &verbSize }, { &mix, &gain } });
}
