/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

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

    auto outline = slider.findColour(Slider::rotarySliderOutlineColourId);
    auto fill = slider.findColour(Slider::rotarySliderFillColourId);

    auto bounds = Rectangle<int>(x, y, width, height).toFloat().reduced(3);

    auto radius = jmin(bounds.getWidth(), bounds.getHeight()) / 2.0f;
    auto toAngle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);
    auto lineW = jmin(5.0f, radius * 0.5f);
    auto arcRadius = radius - lineW * 0.5f;


    //background arc, dial bg path
    Path backgroundArc;
    backgroundArc.addCentredArc(bounds.getCentreX(),
        bounds.getCentreY(),
        arcRadius,
        arcRadius,
        0.0f,
        rotaryStartAngle,
        rotaryEndAngle,
        true);

    //line
    Point<float> thumbPoint(bounds.getCentreX() + (arcRadius)*std::cos(toAngle - MathConstants<float>::halfPi),
        bounds.getCentreY() + (arcRadius)*std::sin(toAngle - MathConstants<float>::halfPi));

    g.setColour(slider.findColour(Slider::thumbColourId));
    g.drawLine(backgroundArc.getBounds().getCentreX(),
        backgroundArc.getBounds().getCentreY(),
        thumbPoint.getX(),
        thumbPoint.getY(),
        lineW);

    //bg arc cd.
    g.setColour(outline);
    g.strokePath(backgroundArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));

    //value arc / fill path
    if (slider.isEnabled())
    {
        Path valueArc;
        valueArc.addCentredArc(bounds.getCentreX(),
            bounds.getCentreY(),
            arcRadius,
            arcRadius,
            0.0f,
            rotaryStartAngle,
            toAngle,
            true);

        g.setColour(fill);
        g.strokePath(valueArc, PathStrokeType(lineW, PathStrokeType::curved, PathStrokeType::rounded));
    }

    //point
    auto thumbWidth = lineW * 1.33f;

    g.setColour(Colour(121u, 27u, 27u));
    g.fillEllipse(Rectangle<float>(thumbWidth, thumbWidth).withCentre(thumbPoint));
}

//============================================================

void RotarySliderWithLabels::paint(juce::Graphics& g)
{
    using namespace juce;

    auto startAng = degreesToRadians(180.f + 90.f);
    auto endAng = degreesToRadians(180.f) + MathConstants<float>::twoPi;

    auto range = getRange();

    auto sliderBounds = getSliderBounds();

    //g.setColour(Colours::red);
    //g.drawRect(getLocalBounds());
    g.setColour(Colours::white);
    g.drawRoundedRectangle(sliderBounds.toFloat(), 2.f, 2.f);

    this->setColour(juce::Slider::ColourIds::rotarySliderFillColourId, Colour(165u, 56u, 56u));
    this->setColour(juce::Slider::ColourIds::thumbColourId, Colour(25u, 25u, 25u));
    this->setColour(juce::Slider::ColourIds::rotarySliderOutlineColourId, Colour(63u, 63u, 63u));

    getLookAndFeel().drawRotarySlider(g,
                                      sliderBounds.getX(),
                                      sliderBounds.getY(),
                                      sliderBounds.getWidth(),
                                      sliderBounds.getHeight(),
                                      jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0),
                                      startAng,
                                      endAng,
                                      *this);

}

juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
    auto bounds = getLocalBounds();

    auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());

    size -= getTextHeight() * 1.5;
    juce::Rectangle<int> r;
    r.setSize(size, size);
    r.setCentre(bounds.getCentreX(), bounds.getCentreY());
    r.setY(2);

    return r;
}

//============================================================

ResponseCurveComponent::ResponseCurveComponent(EQ_Hubert_MoszAudioProcessor& p) : audioProcessor(p)
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->addListener(this);
    }

    startTimerHz(60);
}

ResponseCurveComponent::~ResponseCurveComponent()
{
    const auto& params = audioProcessor.getParameters();
    for (auto param : params)
    {
        param->removeListener(this);
    }
}

void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
    parametersChanged.set(true);
}

void ResponseCurveComponent::timerCallback()
{
    if (parametersChanged.compareAndSetBool(false, true))
    {
        auto chainSettings = getChainSettings(audioProcessor.apvts);

        auto peak1Coefficients = makePeak1Filter(chainSettings, audioProcessor.getSampleRate());
        auto peak2Coefficients = makePeak2Filter(chainSettings, audioProcessor.getSampleRate());
        auto peak3Coefficients = makePeak3Filter(chainSettings, audioProcessor.getSampleRate());
        updateCoefficients(monoChain.get<ChainPositions::Peak1>().coefficients, peak1Coefficients);
        updateCoefficients(monoChain.get<ChainPositions::Peak2>().coefficients, peak2Coefficients);
        updateCoefficients(monoChain.get<ChainPositions::Peak3>().coefficients, peak3Coefficients);

        auto lowCutCoefficients = makeLowCutFilter(chainSettings, audioProcessor.getSampleRate());
        auto highCutCoefficients = makeHighCutFilter(chainSettings, audioProcessor.getSampleRate());
        updateCutFilter(monoChain.get<ChainPositions::LowCut>(), lowCutCoefficients, chainSettings.lowCutSlope);
        updateCutFilter(monoChain.get<ChainPositions::HighCut>(), highCutCoefficients, chainSettings.highCutSlope);

        repaint();
    }
}

void ResponseCurveComponent::paint(juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    auto responseArea = getLocalBounds();

    auto w = responseArea.getWidth();

    auto& lowcut = monoChain.get<ChainPositions::LowCut>();
    auto& highcut = monoChain.get<ChainPositions::HighCut>();
    auto& peak1 = monoChain.get<ChainPositions::Peak1>();
    auto& peak2 = monoChain.get<ChainPositions::Peak2>();
    auto& peak3 = monoChain.get<ChainPositions::Peak3>();

    auto sampleRate = audioProcessor.getSampleRate();

    std::vector<double> mags;

    mags.resize(w);

    for (int i = 0; i < w; i++)
    {
        double mag = 1.f;
        auto freq = juce::mapToLog10(double(i) / double(w), 20.0, 20000.0);

        if (!monoChain.isBypassed<ChainPositions::Peak1>())
            mag *= peak1.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!monoChain.isBypassed<ChainPositions::Peak2>())
            mag *= peak2.coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!monoChain.isBypassed<ChainPositions::Peak3>())
            mag *= peak3.coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!lowcut.isBypassed<0>())
            mag *= lowcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<1>())
            mag *= lowcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<2>())
            mag *= lowcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!lowcut.isBypassed<3>())
            mag *= lowcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        if (!highcut.isBypassed<0>())
            mag *= highcut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<1>())
            mag *= highcut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<2>())
            mag *= highcut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
        if (!highcut.isBypassed<3>())
            mag *= highcut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

        mags[i] = juce::Decibels::gainToDecibels(mag);
    }

    juce::Path responseCurve;

    const double outputMin = responseArea.getBottom();
    const double outputMax = responseArea.getY();
    auto map = [outputMin, outputMax](double input)
    {
        return juce::jmap(input, -24.0, 24.0, outputMin, outputMax);
    };

    responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));
    for (size_t i = 1; i < mags.size(); i++)
    {
        responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
    }

    g.setColour(juce::Colours::white);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 2.f);

    g.setColour(juce::Colours::white);
    g.strokePath(responseCurve, juce::PathStrokeType(3.f));

}

//==============================================================================
EQ_Hubert_MoszAudioProcessorEditor::EQ_Hubert_MoszAudioProcessorEditor (EQ_Hubert_MoszAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), 

    peak1FreqSlider(*audioProcessor.apvts.getParameter("Peak1 Frequency"), "Hz"),
    peak1GainSlider(*audioProcessor.apvts.getParameter("Peak1 Gain"), "dB"),
    peak1QualitySlider(*audioProcessor.apvts.getParameter("Peak1 Q"), ""),
    peak2FreqSlider(*audioProcessor.apvts.getParameter("Peak2 Frequency"), "Hz"),
    peak2GainSlider(*audioProcessor.apvts.getParameter("Peak2 Gain"), "dB"),
    peak2QualitySlider(*audioProcessor.apvts.getParameter("Peak2 Q"), ""),
    peak3FreqSlider(*audioProcessor.apvts.getParameter("Peak3 Frequency"), "Hz"),
    peak3GainSlider(*audioProcessor.apvts.getParameter("Peak3 Gain"), "dB"),
    peak3QualitySlider(*audioProcessor.apvts.getParameter("Peak3 Q"), ""),
    lowCutSlopeSlider(*audioProcessor.apvts.getParameter("LowCut Slope"), "dB/Oct"),
    lowCutFreqSlider(*audioProcessor.apvts.getParameter("LowCut Frequency"), "Hz"),
    highCutSlopeSlider(*audioProcessor.apvts.getParameter("HighCut Slope"), "db/Oct"),
    highCutFreqSlider(*audioProcessor.apvts.getParameter("HighCut Frequency"), "Hz"),
    outputGainSlider(*audioProcessor.apvts.getParameter("Output Gain"), "dB"),

    responseCurveComponent(audioProcessor),
    
    peak1FreqSliderAttachment(audioProcessor.apvts, "Peak1 Frequency", peak1FreqSlider),
    peak1GainSliderAttachment(audioProcessor.apvts, "Peak1 Gain", peak1GainSlider),
    peak1QualitySliderAttachment(audioProcessor.apvts, "Peak1 Q", peak1QualitySlider),
    peak2FreqSliderAttachment(audioProcessor.apvts, "Peak2 Frequency", peak2FreqSlider),
    peak2GainSliderAttachment(audioProcessor.apvts, "Peak2 Gain", peak2GainSlider),
    peak2QualitySliderAttachment(audioProcessor.apvts, "Peak2 Q", peak2QualitySlider),
    peak3FreqSliderAttachment(audioProcessor.apvts, "Peak3 Frequency", peak3FreqSlider),
    peak3GainSliderAttachment(audioProcessor.apvts, "Peak3 Gain", peak3GainSlider),
    peak3QualitySliderAttachment(audioProcessor.apvts, "Peak3 Q", peak3QualitySlider),
    lowCutSlopeSliderAttachment(audioProcessor.apvts, "LowCut Slope", lowCutSlopeSlider),
    lowCutFreqSliderAttachment(audioProcessor.apvts, "LowCut Frequency", lowCutFreqSlider),
    highCutSlopeSliderAttachment(audioProcessor.apvts, "HighCut Slope", highCutSlopeSlider),
    highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Frequency", highCutFreqSlider),
    outputGainSliderAttachment(audioProcessor.apvts, "Output Gain", outputGainSlider)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.

    for (auto* comp : getComps())
    {
        addAndMakeVisible(comp);
    }

    setSize (800, 600);
}

EQ_Hubert_MoszAudioProcessorEditor::~EQ_Hubert_MoszAudioProcessorEditor()
{
   
}

//==============================================================================
void EQ_Hubert_MoszAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    
    //33 33 33
    g.fillAll(juce::Colour(28u, 28u, 28u));
}

void EQ_Hubert_MoszAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.40);

    //V to najwyzej zakomentowac V
    responseArea.removeFromTop(responseArea.getHeight() * 0.02);
    responseArea.removeFromBottom(responseArea.getHeight() * 0.02);
    responseArea.removeFromLeft(responseArea.getWidth() * 0.02);
    responseArea.removeFromRight(responseArea.getWidth() * 0.02);

    responseCurveComponent.setBounds(responseArea);

    auto logoArea = bounds.removeFromRight(bounds.getWidth() * 0.15);
    auto outputGainArea = logoArea.removeFromTop(logoArea.getHeight() * 0.5);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.2);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.25);
    auto peak1Area = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto peak2Area = bounds.removeFromLeft(bounds.getWidth() * 0.5);
    auto peak3Area = bounds;

    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);

    peak1FreqSlider.setBounds(peak1Area.removeFromTop(peak1Area.getHeight() * 0.5));
    peak1GainSlider.setBounds(peak1Area.removeFromTop(peak1Area.getHeight() * 0.5));
    peak1QualitySlider.setBounds(peak1Area);

    peak2FreqSlider.setBounds(peak2Area.removeFromTop(peak2Area.getHeight() * 0.5));
    peak2GainSlider.setBounds(peak2Area.removeFromTop(peak2Area.getHeight() * 0.5));
    peak2QualitySlider.setBounds(peak2Area);

    peak3FreqSlider.setBounds(peak3Area.removeFromTop(peak3Area.getHeight() * 0.5));
    peak3GainSlider.setBounds(peak3Area.removeFromTop(peak3Area.getHeight() * 0.5));
    peak3QualitySlider.setBounds(peak3Area);

    outputGainSlider.setBounds(outputGainArea);

    //13634 - 13.11 - 14:10
}

std::vector<juce::Component*> EQ_Hubert_MoszAudioProcessorEditor::getComps()
{
    return
    {
        &peak1FreqSlider,
        &peak1GainSlider,
        &peak1QualitySlider,
        &peak2FreqSlider,
        &peak2GainSlider,
        &peak2QualitySlider,
        &peak3FreqSlider,
        &peak3GainSlider,
        &peak3QualitySlider,
        &lowCutFreqSlider,
        &highCutFreqSlider,
        &lowCutSlopeSlider,
        &highCutSlopeSlider,
        &responseCurveComponent,
        &outputGainSlider
    };
}
