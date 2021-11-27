/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EQ_Hubert_MoszAudioProcessorEditor::EQ_Hubert_MoszAudioProcessorEditor (EQ_Hubert_MoszAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), 
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
    highCutFreqSliderAttachment(audioProcessor.apvts, "HighCut Frequency", highCutFreqSlider)
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
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.50);
    auto outputGainArea = responseArea.removeFromRight(responseArea.getWidth() * 0.1);

    //V to najwyzej zakomentowac V
    responseArea.removeFromTop(responseArea.getHeight() * 0.02);
    responseArea.removeFromBottom(responseArea.getHeight() * 0.02);
    responseArea.removeFromLeft(responseArea.getWidth() * 0.02);

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

    g.setColour(juce::Colours::black);
    g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 2.f);

    g.setColour(juce::Colours::white);
    g.strokePath(responseCurve, juce::PathStrokeType(3.f));

}

void EQ_Hubert_MoszAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.50);
    auto outputGainArea = responseArea.removeFromRight(responseArea.getWidth() * 0.1);

    auto lowCutArea = bounds.removeFromLeft(bounds.getWidth() * 0.25);
    auto highCutArea = bounds.removeFromRight(bounds.getWidth() * 0.33);
    auto peak1Area = bounds.removeFromLeft(bounds.getWidth() * 0.33);
    auto peak2Area = bounds.removeFromLeft(bounds.getWidth() * 0.5);
    auto peak3Area = bounds;

    lowCutFreqSlider.setBounds(lowCutArea.removeFromTop(lowCutArea.getHeight() * 0.5));
    lowCutSlopeSlider.setBounds(lowCutArea);
    highCutFreqSlider.setBounds(highCutArea.removeFromTop(highCutArea.getHeight() * 0.5));
    highCutSlopeSlider.setBounds(highCutArea);

    peak1FreqSlider.setBounds(peak1Area.removeFromTop(peak1Area.getHeight() * 0.33));
    peak1GainSlider.setBounds(peak1Area.removeFromTop(peak1Area.getHeight() * 0.5));
    peak1QualitySlider.setBounds(peak1Area);

    peak2FreqSlider.setBounds(peak2Area.removeFromTop(peak2Area.getHeight() * 0.33));
    peak2GainSlider.setBounds(peak2Area.removeFromTop(peak2Area.getHeight() * 0.5));
    peak2QualitySlider.setBounds(peak2Area);

    peak3FreqSlider.setBounds(peak3Area.removeFromTop(peak3Area.getHeight() * 0.33));
    peak3GainSlider.setBounds(peak3Area.removeFromTop(peak3Area.getHeight() * 0.5));
    peak3QualitySlider.setBounds(peak3Area);

    //peak2, peak3

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
        &highCutSlopeSlider
    };
}
