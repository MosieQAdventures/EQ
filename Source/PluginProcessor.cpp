/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
EQ_Hubert_MoszAudioProcessor::EQ_Hubert_MoszAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

EQ_Hubert_MoszAudioProcessor::~EQ_Hubert_MoszAudioProcessor()
{
}

//==============================================================================
const juce::String EQ_Hubert_MoszAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool EQ_Hubert_MoszAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool EQ_Hubert_MoszAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool EQ_Hubert_MoszAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double EQ_Hubert_MoszAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int EQ_Hubert_MoszAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int EQ_Hubert_MoszAudioProcessor::getCurrentProgram()
{
    return 0;
}

void EQ_Hubert_MoszAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String EQ_Hubert_MoszAudioProcessor::getProgramName (int index)
{
    return {};
}

void EQ_Hubert_MoszAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void EQ_Hubert_MoszAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void EQ_Hubert_MoszAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool EQ_Hubert_MoszAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void EQ_Hubert_MoszAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);

        // ..do something to the data...
    }
}

//==============================================================================
bool EQ_Hubert_MoszAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EQ_Hubert_MoszAudioProcessor::createEditor()
{
    //return new EQ_Hubert_MoszAudioProcessorEditor (*this);
    return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void EQ_Hubert_MoszAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void EQ_Hubert_MoszAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

juce::AudioProcessorValueTreeState::ParameterLayout EQ_Hubert_MoszAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Frequency", "LowCut Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f), 20.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Frequency", "HighCut Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f), 20000.f));

    // Peak 1
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak1 Frequency", "Peak1 Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f), 1000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak1 Gain", "Peak1 Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.1f, 1.f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak1 Q", "Peak1 Q", juce::NormalisableRange<float>(0.1f, 10.0f, 0.05f, 1.f), 1.f));

    /*
    // Peak 2
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak2 Frequency", "Peak2 Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f), 1000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak2 Gain", "Peak2 Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.1f, 1.f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak2 Q", "Peak2 Q", juce::NormalisableRange<float>(0.1f, 10.0f, 0.05f, 1.f), 1.f));

    // Peak 3
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak3 Frequency", "Peak3 Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 1.f), 1000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak3 Gain", "Peak3 Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.1f, 1.f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak3 Q", "Peak3 Q", juce::NormalisableRange<float>(0.1f, 10.0f, 0.05f, 1.f), 1.f));
    */

    juce::StringArray stringArray; //tworzenie stringow per ocave
    for (int i = 0; i < 4; i++)
    {
        juce::String str;
        str << (12 + i * 12);
        str << " db/Oct";
        stringArray.add(str);
    }

    layout.add(std::make_unique<juce::AudioParameterChoice>("LowCut Slope", "LowCut Slope", stringArray, 0));

    layout.add(std::make_unique<juce::AudioParameterChoice>("HighCut Slope", "HighCut Slope", stringArray, 0));

    return layout;
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EQ_Hubert_MoszAudioProcessor();
}
