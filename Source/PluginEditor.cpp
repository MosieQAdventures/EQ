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

    g.setColour (juce::Colours::white);
    g.setFont (10.0f);
    g.drawFittedText ("Parameter!", getLocalBounds(), juce::Justification::centred, 1);
}

void EQ_Hubert_MoszAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    auto bounds = getLocalBounds();
    auto responseArea = bounds.removeFromTop(bounds.getHeight() * 0.50);
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
