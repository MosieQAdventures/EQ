/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

struct LookAndFeel : juce::LookAndFeel_V4
{
    void drawRotarySlider(juce::Graphics&,
        int x, int y, int width, int height,
        float sliderPosProportional,
        float rotaryStartAngle,
        float rotaryEndAngle,
        juce::Slider&) override;
};

struct RotarySliderWithLabels : juce::Slider 
{
    RotarySliderWithLabels(juce::RangedAudioParameter& rap, const juce::String& unitSuffix) : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag,     juce::Slider::TextEntryBoxPosition::NoTextBox),
    param(&rap),
    suffix(unitSuffix)
    {
        setLookAndFeel(&lnf);
    }

    ~RotarySliderWithLabels()
    {
        setLookAndFeel(nullptr);
    }

    struct LabelPos
    {
        float pos;
        juce::String label;
    };

    juce::Array<LabelPos> labels;

    void paint(juce::Graphics& g) override;;
    juce::Rectangle<int> getSliderBounds() const;
    int getTextHeight() const { return 14; }
    juce::String getDisplayString() const;

private:
    LookAndFeel lnf;


    juce::RangedAudioParameter* param;
    juce::String suffix;

};

 // 20459
struct ResponseCurveComponent : juce::Component, juce::AudioProcessorParameter::Listener, juce::Timer
{
    ResponseCurveComponent(EQ_Hubert_MoszAudioProcessor&);
    ~ResponseCurveComponent();

    void parameterValueChanged(int parameterIndex, float newValue) override;

    void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override { }

    void timerCallback() override;

    void paint(juce::Graphics& g) override;

private:
    EQ_Hubert_MoszAudioProcessor& audioProcessor;
    juce::Atomic<bool> parametersChanged{ false };

    MonoChain monoChain;
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

    RotarySliderWithLabels peak1FreqSlider,
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
                        highCutSlopeSlider,
                        outputGainSlider; 

    ResponseCurveComponent responseCurveComponent;

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
                highCutSlopeSliderAttachment,
                outputGainSliderAttachment;


    std::vector<juce::Component*> getComps();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQ_Hubert_MoszAudioProcessorEditor)
};
