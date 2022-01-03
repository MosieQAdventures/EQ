/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

enum Slope
{
    Slope_12,
    Slope_24,
    Slope_36,
    Slope_48
};

struct ChainSettings
{
    float peak1Freq{ 0 }, peak1GainInDecibels{ 0 }, peak1Quality{ 1.f };
    float peak2Freq{ 0 }, peak2GainInDecibels{ 0 }, peak2Quality{ 1.f };
    float peak3Freq{ 0 }, peak3GainInDecibels{ 0 }, peak3Quality{ 1.f };
    float lowCutFreq{ 0 }, highCutFreq{ 0 };
    float outputGain{ 0 };

    Slope lowCutSlope{ Slope::Slope_12 }, highCutSlope{ Slope::Slope_12 };
};

void setMyParameters(juce::AudioProcessorValueTreeState& apvts); // test
void changeParameterValue(juce::AudioProcessorValueTreeState& apvts, std::string param_name, float new_value); //test

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts);

using Filter = juce::dsp::IIR::Filter<float>;

using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

using Gain = juce::dsp::Gain<float>;

using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, Filter, Filter, CutFilter/*, Gain*/>;

enum ChainPositions
{
    LowCut,
    Peak1,
    Peak2,
    Peak3,
    HighCut/*,
    OutputGain*/
};

using Coefficients = Filter::CoefficientsPtr;
void updateCoefficients(Coefficients& ld, const Coefficients& replacements);

Coefficients makePeak1Filter(const ChainSettings& chainSettings, double sampleRate);
Coefficients makePeak2Filter(const ChainSettings& chainSettings, double sampleRate);
Coefficients makePeak3Filter(const ChainSettings& chainSettings, double sampleRate);

template<int Index, typename ChainType, typename CoefficientType>
void update(ChainType& chain,
    const CoefficientType& coefficients)
{
    updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
    chain.template setBypassed<Index>(false);
}

template<typename ChainType, typename CoefficientType>
void updateCutFilter(ChainType& chain,
    const CoefficientType& coefficients,
    const Slope& slope)
{
    chain.template setBypassed<0>(true);
    chain.template setBypassed<1>(true);
    chain.template setBypassed<2>(true);
    chain.template setBypassed<3>(true);

    switch (slope)
    {
    case Slope_48:
    {
        update<3>(chain, coefficients);
    }
    case Slope_36:
    {
        update<2>(chain, coefficients);
    }
    case Slope_24:
    {
        update<1>(chain, coefficients);
    }
    case Slope_12:
    {
        update<0>(chain, coefficients);
    }
    }
}

inline auto makeLowCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.lowCutFreq, 
        sampleRate, 
        2 * (chainSettings.lowCutSlope + 1));
}

inline auto makeHighCutFilter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(chainSettings.highCutFreq, 
        sampleRate, 
        2 * (chainSettings.highCutSlope + 1));
}

//==============================================================================
/**
*/
class EQ_Hubert_MoszAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    EQ_Hubert_MoszAudioProcessor();
    ~EQ_Hubert_MoszAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout 
        createParameterLayout();
    juce::AudioProcessorValueTreeState apvts {*this, nullptr, "Parameters", createParameterLayout()};

private:

    MonoChain leftChain, rightChain;

    void updatePeak1Filter(const ChainSettings& chainSettings);
    void updatePeak2Filter(const ChainSettings& chainSettings);
    void updatePeak3Filter(const ChainSettings& chainSettings);

    void updateLowCutFilters(const ChainSettings& chainSettings);
    void updateHighCutFilters(const ChainSettings& chainSettings);

    void updateFilters();

    //----------------- testing

    void create_txt_file();
    void networkClient();

    //=============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQ_Hubert_MoszAudioProcessor)
};
