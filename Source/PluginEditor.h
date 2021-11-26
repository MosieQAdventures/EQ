/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct CustomRotarySlider : juce::Slider 
{
    CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox)
    {

    }
};

//==============================================================================
/**
*/
class EQ_Hubert_MoszAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    EQ_Hubert_MoszAudioProcessorEditor (EQ_Hubert_MoszAudioProcessor&);
    ~EQ_Hubert_MoszAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    EQ_Hubert_MoszAudioProcessor& audioProcessor;

    CustomRotarySlider peak1FreqSlider,
        peak1GainSlider,
        peak1QualitySlider,
        peak2FreqSlider,
        peak2GainSlider,
        peak2QualitySlider,
        peak3FreqSlider,
        peak3GainSlider,
        peak3QualitySlider,
        lowCutFreqSlider,
        highCutFreqSlider,
        lowCutSlopeSlider,
        highCutSlopeSlider; //tutaj tez dodac kolejne peaki

    using APVTS = juce::AudioProcessorValueTreeState;
    using Attachment = APVTS::SliderAttachment;

    Attachment peak1FreqSliderAttachment,
                peak1GainSliderAttachment,
                peak1QualitySliderAttachment,
                peak2FreqSliderAttachment,
                peak2GainSliderAttachment,
                peak2QualitySliderAttachment,
                peak3FreqSliderAttachment,
                peak3GainSliderAttachment,
                peak3QualitySliderAttachment,
                lowCutFreqSliderAttachment,
                highCutFreqSliderAttachment,
                lowCutSlopeSliderAttachment,
                highCutSlopeSliderAttachment;


    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQ_Hubert_MoszAudioProcessorEditor)
};
