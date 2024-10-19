/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
//==============================================================================

void LookAndFeel::drawRotarySlider(juce::Graphics& g,
    int x,
    int y,
    int width,
    int height,
    float sliderPosProportional,
    float rotaryStartAngle,
    float rotaryEndAngle,
    juce::Slider& slider)
{
    using namespace juce;

    auto bounds = Rectangle<float>(x, y, width, height);

    auto enabled = slider.isEnabled();

    g.setColour(enabled ? Colour(255u, 126u, 13u) : Colours::darkgrey); //apka Digital Color Meter
    g.fillEllipse(bounds);

    g.setColour(enabled ? Colour(207u, 34u, 0u) : Colours::grey);
    g.drawEllipse(bounds, 2.f);

    if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
    {
        auto center = bounds.getCentre();

        Path p;

        Rectangle<float> r;
        r.setLeft(center.getX() - 2);
        r.setRight(center.getX() + 2);
        r.setTop(bounds.getY());
        r.setBottom(center.getY() - rswl->getTextHeight() * 1.5);

        p.addRoundedRectangle(r, 2.f);

        jassert(rotaryStartAngle < rotaryEndAngle);

        auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle); //zmapowanie wartosci radionow pomiedzy granice rotary slidera

        p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

        g.fillPath(p);

        g.setFont(rswl->getTextHeight());
        auto text = rswl->getDisplayString();
        auto strWidth = g.getCurrentFont().getStringWidth(text);

        r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
        r.setCentre(center);

        g.setColour(enabled ? Colours::black : Colours::darkgrey);
        g.fillRect(r);

        g.setColour(enabled ? Colours::white : Colours::lightgrey);
        g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
    }
}

void LookAndFeel::drawToggleButton(juce::Graphics& g,
    juce::ToggleButton& toggleButton,
    bool shouldDrawButtonAsHighlighted,
    bool shouldDrawButtonAsDown)
{
    using namespace juce;

    Path powerButton;

    auto bounds = toggleButton.getLocalBounds();
    auto size = jmin(bounds.getWidth(), bounds.getHeight()) - 80;
    auto r = bounds.withSizeKeepingCentre(size, size).toFloat();

    float ang = 30.f;

    size -= 10;

    powerButton.addCentredArc(r.getCentreX(),
        r.getCentreY(),
        size * 0.5,
        size * 0.5, 0.f,
        degreesToRadians(ang),
        degreesToRadians(360.f - ang),
        true);

    powerButton.startNewSubPath(r.getCentreX(), r.getY());
    powerButton.lineTo(r.getCentre());

    PathStrokeType pst(2.f, PathStrokeType::JointStyle::curved);

    auto color = toggleButton.getToggleState() ?  Colour(207u, 34u, 0u) : Colours::dimgrey;

    g.setColour(color);
    g.strokePath(powerButton, pst);
    g.drawEllipse(r, 2);
}

void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    using namespace juce;

    auto startAng = degreesToRadians(180.f + 55.f);
    auto endAng = degreesToRadians(180.f - 55.f) + MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderBounds();

    getLookAndFeel().drawRotarySlider(g,
        sliderBounds.getX(),
        sliderBounds.getY(),
        sliderBounds.getWidth(),
        sliderBounds.getHeight(),
        jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
        startAng,
        endAng,
        *this);

    auto center = sliderBounds.toFloat().getCentre();
    auto radius = sliderBounds.getWidth() * 0.5f;

    g.setColour(Colour(255u, 126u, 13u));

    auto numChoices = labels.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto pos = labels[i].pos;
        jassert(0.f <= pos);
        jassert(pos <= 1.22f);

        auto ang = jmap(pos, 0.f, 1.f, startAng, endAng);

        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, ang);

        Rectangle<float> r;
        auto str = labels[i].label;

        if (labels[i].pos == 1.22f) {
            g.setFont(getTextHeight() + 1);
            r.setSize(g.getCurrentFont().getStringWidthFloat(str), getTextHeight() - 6);
            r.setCentre(c);
            r.setY(r.getY() + getTextHeight() - 6);
        }
        else {
            g.setFont(getTextHeight() - 2);
            r.setSize(g.getCurrentFont().getStringWidthFloat(str), getTextHeight() + 2);
            r.setCentre(c);
            r.setY(r.getY() + getTextHeight() + 2);
        }

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::verticallyCentred, 1);
    }
}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    //return getLocalBounds();
    auto bounds = getLocalBounds();

    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);

    return r;
}

juce::String RotarySliderWithLabels::getDisplayString() const
{
    if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
    {
        return choiceParam->getCurrentChoiceName();
    }

    juce::String str;

    auto numChoices = showPercentages.size();
    for (int i = 0; i < numChoices; ++i)
    {
        if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
        {
            auto val = getValue();
            float minValue = floatParam->range.start;
            float maxValue = floatParam->range.end;

            float percentValue = round((val - minValue) / (maxValue - minValue) * 100.f);

            auto perc = showPercentages[i].showPercentage;
            if (perc == true) {
                str = juce::String(percentValue) + " %"; // 1 decimal place
            }
            else {
                str = juce::String(val);
            }
        }
        else
        {
            jassertfalse; //probably not necessery
        }
    }

    if (suffix.isNotEmpty())
    {
        str << " ";
        str << suffix;
    }

    return str;
}

void RotarySliderWithLabels::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown() && this->isEnabled())
    {
        showTextEditor();
    }
    else
    {
        juce::Slider::mouseDown(event);
    }
}

void RotarySliderWithLabels::showTextEditor()
{
    auto* editor = new juce::TextEditor();
    auto* editorListener = new juce::TextEditor::Listener;
    auto localBounds = getLocalBounds();
    editor->setJustification(juce::Justification::centred);
    if (suffix == "steps") {
        editor->setText(juce::String(getValue()));
    }
    else if (suffix == " ") {
        if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
        {
            auto val = getValue();
            float minValue = floatParam->range.start;
            float maxValue = floatParam->range.end;

            editor->setText(juce::String(round((val - minValue) / (maxValue - minValue) * 100.f)));

        }
        else
        {
            jassertfalse; //probably not necessery
        }
    }
    
    editor->setBounds(localBounds.getCentreX(), localBounds.getCentreY(), 50, 25);
    editor->addListener(editorListener);

    editor->onReturnKey = [this, editor]() {
        updateSliderValue(editor, this);
        };

    editor->onFocusLost = [this, editor]() {
        updateSliderValue(editor, this);
        };

    addAndMakeVisible(editor);
    editor->grabKeyboardFocus();
}

void RotarySliderWithLabels::updateSliderValue(juce::TextEditor* editor, juce::Slider* slider)
{
    // Update the slider value
    if (suffix == "steps") {
        double newVal = editor->getText().getDoubleValue();
        slider->setValue(newVal);
    }
    else if (suffix == " ") {
        if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
        {
            auto val = getValue();
            float minVal = floatParam->range.start;
            float maxVal = floatParam->range.end;

            double newVal = editor->getText().getDoubleValue();
            newVal = minVal + (newVal / 100.f) * (maxVal - minVal);
            slider->setValue(newVal);
        }
        else
        {
            jassertfalse; //probably not necessery
        }
    }

    // Remove the editor
    editor->removeChildComponent(editor);
    delete editor;
}

void PowerButton::paint(juce::Graphics& g)
{
    using namespace juce;

    auto buttonBounds = getButtonBounds();

    getLookAndFeel().drawToggleButton(g,
        *this,
        true,
        true);

    auto center = buttonBounds.toFloat().getCentre();
    auto radius = buttonBounds.getWidth() * 0.5f;

    g.setColour(Colour(255u, 126u, 13u));

    auto numChoices = names.size();
    for (int i = 0; i < numChoices; ++i)
    {
        auto c = center.getPointOnCircumference(radius + getTextHeight() * 0.5f + 1, degreesToRadians(180.f));

        Rectangle<float> r;
        auto str = names[i].name;

        g.setFont(getTextHeight());
        r.setSize(g.getCurrentFont().getStringWidthFloat(str), getTextHeight());
        r.setCentre(c);
        r.setY(r.getY() + getTextHeight() - 20);

        g.drawFittedText(str, r.toNearestInt(), juce::Justification::verticallyCentred, 1);
    }
}

juce::Rectangle<int> PowerButton::getButtonBounds() const
{
    auto bounds = getLocalBounds();

    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight() * 2;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), 0);
    r.setY(2);

    return r;
}

//==============================================================================
BitCrusherAudioProcessorEditor::BitCrusherAudioProcessorEditor (BitCrusherAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    bitStepsSlider(*audioProcessor.apvts.getParameter("Bit Steps"), "steps"),
    dryWetMixSlider(*audioProcessor.apvts.getParameter("Dry Wet Mix"), " "),

    bitStepsSliderAttachment(audioProcessor.apvts, "Bit Steps", bitStepsSlider),
    dryWetMixSliderAttachment(audioProcessor.apvts, "Dry Wet Mix", dryWetMixSlider),
    bypassButtonAttachment(audioProcessor.apvts, "Bypass", bypassButton)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    auto chainSettings = getChainSettings(audioProcessor.apvts);

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    bitStepsSlider.labels.add({ 0.f, "1" });
    bitStepsSlider.labels.add({ 1.22f, "Quantization Steps" });
    bitStepsSlider.labels.add({ 1.f, "32" });

    dryWetMixSlider.labels.add({ 0.f, "0 %" });
    dryWetMixSlider.labels.add({ 1.22f, "Dry/Wet Mix" });
    dryWetMixSlider.labels.add({ 1.f, "100 %" });

    bitStepsSlider.showPercentages.add({ false });
    dryWetMixSlider.showPercentages.add({ true });

    bypassButton.names.add({ "BYPASS" });
    
    bypassButton.setLookAndFeel(&lnf);

    bypassButton.setToggleState(false, false);


    auto safePtr = juce::Component::SafePointer<BitCrusherAudioProcessorEditor>(this);

    bypassButton.onClick = [safePtr]()
        {
            if (auto* comp = safePtr.getComponent()) {
                auto bypassed = comp->bypassButton.getToggleState();

                comp->bitStepsSlider.setEnabled(!bypassed);
                comp->dryWetMixSlider.setEnabled(!bypassed);
            }
        };

    setSize (400, 300);
}

BitCrusherAudioProcessorEditor::~BitCrusherAudioProcessorEditor()
{
    bypassButton.setLookAndFeel(nullptr);
}

//==============================================================================
void BitCrusherAudioProcessorEditor::paint (juce::Graphics& g)
{
    using namespace juce;
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(Colours::black);
}

void BitCrusherAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    auto bounds = getLocalBounds();

    bounds.removeFromTop(20);
    bounds.removeFromBottom(20);

    auto slidersArea = bounds.removeFromTop(bounds.getHeight() * 0.5f);

    bitStepsSlider.setBounds(slidersArea.removeFromLeft(slidersArea.getWidth() * 0.5f));
    dryWetMixSlider.setBounds(slidersArea);

    bypassButton.setBounds(bounds);
}

std::vector<juce::Component*> BitCrusherAudioProcessorEditor::getComps()
{
    return
    {
        &bitStepsSlider,
        &dryWetMixSlider,

        &bypassButton
    };
}