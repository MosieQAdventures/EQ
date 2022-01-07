/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "json.hpp"
#include <fstream>
#include <string>
#include <iostream>
#include <WS2tcpip.h>
#include <boost/asio.hpp>
#include <thread>

#pragma comment(lib, "ws2_32.lib")
#define DEFAULT_BUFLEN 4096

// include boost -> Narzedzia -> Menager Pakietow NuGet
// Update-Package -reinstall

//==============================================================================
//test
void networkClient();
//test

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
    std::thread t1{ networkClient };
    t1.detach();
}

EQ_Hubert_MoszAudioProcessor::~EQ_Hubert_MoszAudioProcessor()
{
}

//======== TESTING ============================================================
//globalna zmienna? >.<
nlohmann::json json_parameter_data;
std::string json_from_server_data = 
"{ \"HighCut Frequency\": 20000.0, \"HighCut Slope\" : 0.0, \"LowCut Frequency\" : 20.0, \"LowCut Slope\" : 0.0, \"Peak1 Frequency\" : 200.0, \"Peak1 Gain\" : 0.0, \"Peak1 Q\" : 1.0, \"Peak2 Frequency\" : 1000.0, \"Peak2 Gain\" : 0.0, \"Peak2 Q\" : 1.0, \"Peak3 Frequency\" : 2000.0, \"Peak3 Gain\" : 0.0, \"Peak3 Q\" : 1.0}"; //inicjalizacja zmiennej



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

    juce::dsp::ProcessSpec spec;

    spec.maximumBlockSize = samplesPerBlock;

    spec.numChannels = 1;

    spec.sampleRate = sampleRate;

    setMyParameters(apvts);

    /*outputGain.prepare(spec);
    outputGain.setRampDurationSeconds(0.05);*/

    leftChain.prepare(spec);
    rightChain.prepare(spec);

    updateFilters();
    //networkClient();
    

    //---------------------------------- TESTING

    create_txt_file();
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

    updateFilters();
    
    //outputGain.setGainDecibels(outputGainParam->get());

    juce::dsp::AudioBlock<float> block(buffer);
    //auto context = juce::dsp::ProcessContextReplacing<float>(block);

    auto chainSettings_2 = getChainSettings(apvts);

    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);

    juce::dsp::ProcessContextReplacing<float> leftContext(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

    leftChain.process(leftContext);
    rightChain.process(rightContext);

    create_txt_file(); //tworzenie plikow txt i json

    setMyParameters(apvts);

    //networkClient(); //jeszcze nie wiem gdzie to wsadzic, moze w inny watek??

    //--------------------------------------------------------------------------------

    //applyGain(buffer, outputGain);
}

//==============================================================================
bool EQ_Hubert_MoszAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* EQ_Hubert_MoszAudioProcessor::createEditor()
{
    return new EQ_Hubert_MoszAudioProcessorEditor (*this); //no gui yet 13.11
    //return new juce::GenericAudioProcessorEditor(*this); //gui test stuff
}

//=============================================================================
void EQ_Hubert_MoszAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.

    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void EQ_Hubert_MoszAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.

    auto tree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (tree.isValid())
    {
        setMyParameters(apvts);
        apvts.replaceState(tree);
        updateFilters();
    }
}

ChainSettings getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
    ChainSettings settings;

    settings.lowCutFreq = apvts.getRawParameterValue("LowCut Frequency")->load();
    settings.highCutFreq = apvts.getRawParameterValue("HighCut Frequency")->load();

    settings.peak1Freq = apvts.getRawParameterValue("Peak1 Frequency")->load();
    settings.peak1GainInDecibels = apvts.getRawParameterValue("Peak1 Gain")->load();
    settings.peak1Quality = apvts.getRawParameterValue("Peak1 Q")->load();
    settings.peak2Freq = apvts.getRawParameterValue("Peak2 Frequency")->load();
    settings.peak2GainInDecibels = apvts.getRawParameterValue("Peak2 Gain")->load();
    settings.peak2Quality = apvts.getRawParameterValue("Peak2 Q")->load();
    settings.peak3Freq = apvts.getRawParameterValue("Peak3 Frequency")->load();
    settings.peak3GainInDecibels = apvts.getRawParameterValue("Peak3 Gain")->load();
    settings.peak3Quality = apvts.getRawParameterValue("Peak3 Q")->load();
    settings.lowCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LowCut Slope")->load());
    settings.highCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HighCut Slope")->load());

    settings.outputGain = apvts.getRawParameterValue("Output Gain")->load();
  
    return settings;
}

Coefficients makePeak1Filter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.peak1Freq,
        chainSettings.peak1Quality,
        juce::Decibels::decibelsToGain(chainSettings.peak1GainInDecibels));
}

Coefficients makePeak2Filter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.peak2Freq,
        chainSettings.peak2Quality,
        juce::Decibels::decibelsToGain(chainSettings.peak2GainInDecibels));
}

Coefficients makePeak3Filter(const ChainSettings& chainSettings, double sampleRate)
{
    return juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate,
        chainSettings.peak3Freq,
        chainSettings.peak3Quality,
        juce::Decibels::decibelsToGain(chainSettings.peak3GainInDecibels));
}

void EQ_Hubert_MoszAudioProcessor::updatePeak1Filter(const ChainSettings& chainSettings) 
{
    auto peak1Coefficients = makePeak1Filter(chainSettings, getSampleRate());

    updateCoefficients(leftChain.get<ChainPositions::Peak1>().coefficients, peak1Coefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak1>().coefficients, peak1Coefficients);
}

void EQ_Hubert_MoszAudioProcessor::updatePeak2Filter(const ChainSettings& chainSettings)
{
    auto peak2Coefficients = makePeak2Filter(chainSettings, getSampleRate());

    updateCoefficients(leftChain.get<ChainPositions::Peak2>().coefficients, peak2Coefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak2>().coefficients, peak2Coefficients);
}

void EQ_Hubert_MoszAudioProcessor::updatePeak3Filter(const ChainSettings& chainSettings)
{
    auto peak3Coefficients = makePeak3Filter(chainSettings, getSampleRate());

    updateCoefficients(leftChain.get<ChainPositions::Peak3>().coefficients, peak3Coefficients);
    updateCoefficients(rightChain.get<ChainPositions::Peak3>().coefficients, peak3Coefficients);
}

void updateCoefficients(Coefficients & old, const Coefficients & replacements)
{
    *old = *replacements;
}

void EQ_Hubert_MoszAudioProcessor::updateLowCutFilters(const ChainSettings& chainSettings)
{
    auto lowCutCoefficients = makeLowCutFilter(chainSettings, getSampleRate());

    auto& leftLowCut = leftChain.get<ChainPositions::LowCut>();
    updateCutFilter(leftLowCut, lowCutCoefficients, chainSettings.lowCutSlope);

    auto& rightLowCut = rightChain.get<ChainPositions::LowCut>();
    updateCutFilter(rightLowCut, lowCutCoefficients, chainSettings.lowCutSlope);
}

void EQ_Hubert_MoszAudioProcessor::updateHighCutFilters(const ChainSettings& chainSettings)
{
    auto highCutCoefficients = makeHighCutFilter(chainSettings, getSampleRate());

    auto& leftHighCut = leftChain.get<ChainPositions::HighCut>();
    updateCutFilter(leftHighCut, highCutCoefficients, chainSettings.highCutSlope);

    auto& rightHighCut = rightChain.get<ChainPositions::HighCut>();
    updateCutFilter(rightHighCut, highCutCoefficients, chainSettings.highCutSlope);
}

void EQ_Hubert_MoszAudioProcessor::updateFilters()
{
    auto chainSettings = getChainSettings(apvts);
    
    updateLowCutFilters(chainSettings);
    updatePeak1Filter(chainSettings);
    updatePeak2Filter(chainSettings);
    updatePeak3Filter(chainSettings);
    updateHighCutFilters(chainSettings);
}

juce::AudioProcessorValueTreeState::ParameterLayout EQ_Hubert_MoszAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>("LowCut Frequency", "LowCut Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("HighCut Frequency", "HighCut Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 20000.f));

    // Peak 1
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak1 Frequency", "Peak1 Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 200.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak1 Gain", "Peak1 Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.1f, 1.f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak1 Q", "Peak1 Q", juce::NormalisableRange<float>(0.1f, 10.0f, 0.05f, 0.29f), 1.f));

    // Peak 2
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak2 Frequency", "Peak2 Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 1000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak2 Gain", "Peak2 Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.1f, 1.f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak2 Q", "Peak2 Q", juce::NormalisableRange<float>(0.1f, 10.0f, 0.05f, 0.29f), 1.f));

    // Peak 3
    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak3 Frequency", "Peak3 Frequency", juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 2000.f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak3 Gain", "Peak3 Gain", juce::NormalisableRange<float>(-24.f, 24.f, 0.1f, 1.f), 0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>("Peak3 Q", "Peak3 Q", juce::NormalisableRange<float>(0.1f, 10.0f, 0.05f, 0.29f), 1.f));

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

    //-------

    layout.add(std::make_unique<juce::AudioParameterFloat>("Output Gain", "Output Gain", juce::NormalisableRange<float>(-12.f, 12.f, 0.1f, 1.f), 0.0f));

    return layout;
}

//--------------------

void setMyParameters(juce::AudioProcessorValueTreeState& apvts)
{
    /*
    std::ifstream ifs("C:\\Users\\mosie\\Desktop\\Hubert\\Programming\\EQ_Hubert_Mosz\\AdditionalFiles\\json_parameter_values_test.json"); //plik do odczytu, zmienic na to z serwera co wychodzi
    */
    //nlohmann::json j = nlohmann::json::parse(ifs); //odczyt danych z pliku json
    nlohmann::json j = nlohmann::json::parse(json_from_server_data);

    float new_lowCutFreq = j["LowCut Frequency"];
    float new_lowCutSlope = j["LowCut Slope"];
    float new_peak1Freq = j["Peak1 Frequency"];
    float new_peak1GainInDecibels = j["Peak1 Gain"];
    float new_peak1Quality = j["Peak1 Q"];
    float new_peak2Freq = j["Peak2 Frequency"];
    float new_peak2GainInDecibels = j["Peak2 Gain"];
    float new_peak2Quality = j["Peak2 Q"];
    float new_peak3Freq = j["Peak3 Frequency"];
    float new_peak3GainInDecibels = j["Peak3 Gain"];
    float new_peak3Quality = j["Peak3 Q"];
    float new_highCutFreq = j["HighCut Frequency"];
    float new_highCutSlope = j["HighCut Slope"];


    auto settings = getChainSettings(apvts);

    //change parameter value from json file/data
    changeParameterValue(apvts, "LowCut Frequency", new_lowCutFreq);
    changeParameterValue(apvts, "LowCut Slope", new_lowCutSlope);

    changeParameterValue(apvts, "Peak1 Frequency", new_peak1Freq);
    changeParameterValue(apvts, "Peak1 Gain", new_peak1GainInDecibels);
    changeParameterValue(apvts, "Peak1 Q", new_peak1Quality);

    changeParameterValue(apvts, "Peak2 Frequency", new_peak2Freq);
    changeParameterValue(apvts, "Peak2 Gain", new_peak2GainInDecibels);
    changeParameterValue(apvts, "Peak2 Q", new_peak2Quality);

    changeParameterValue(apvts, "Peak3 Frequency", new_peak3Freq);
    changeParameterValue(apvts, "Peak3 Gain", new_peak3GainInDecibels);
    changeParameterValue(apvts, "Peak3 Q", new_peak3Quality);

    changeParameterValue(apvts, "HighCut Frequency", new_highCutFreq);
    changeParameterValue(apvts, "HighCut Slope", new_highCutSlope);
}

void changeParameterValue(juce::AudioProcessorValueTreeState& apvts, std::string param_name, float new_value)
{
    auto* param = apvts.getParameter(param_name);
    auto oldValue = param->convertTo0to1((float)new_value);
    param->beginChangeGesture();
    param->setValueNotifyingHost(param->convertTo0to1((float)new_value));
    param->endChangeGesture();
}

void EQ_Hubert_MoszAudioProcessor::create_txt_file()
{
    // txt file
    // good
    float lowCutFreq = apvts.getRawParameterValue("LowCut Frequency")->load();
    float lowCutSlope = apvts.getRawParameterValue("LowCut Slope")->load();
    float peak1Freq = apvts.getRawParameterValue("Peak1 Frequency")->load();
    float peak1GainInDecibels = apvts.getRawParameterValue("Peak1 Gain")->load();
    float peak1Quality = apvts.getRawParameterValue("Peak1 Q")->load();
    float peak2Freq = apvts.getRawParameterValue("Peak2 Frequency")->load();
    float peak2GainInDecibels = apvts.getRawParameterValue("Peak2 Gain")->load();
    float peak2Quality = apvts.getRawParameterValue("Peak2 Q")->load();
    float peak3Freq = apvts.getRawParameterValue("Peak3 Frequency")->load();
    float peak3GainInDecibels = apvts.getRawParameterValue("Peak3 Gain")->load();
    float peak3Quality = apvts.getRawParameterValue("Peak3 Q")->load();
    float highCutFreq = apvts.getRawParameterValue("HighCut Frequency")->load();
    float highCutSlope = apvts.getRawParameterValue("HighCut Slope")->load();

    //TEST
    std::ifstream ifs("C:\\Users\\mosie\\Desktop\\Hubert\\Programming\\EQ_Hubert_Mosz\\AdditionalFiles\\json_parameter_values_test.json");
    nlohmann::json j = nlohmann::json::parse(ifs); //odczyt danych z pliku json

    float new_lowCutFreq = j["LowCut Frequency"];
    float new_lowCutSlope = j["LowCut Slope"];
    float new_peak1Freq = j["Peak1 Frequency"];
    float new_peak1GainInDecibels = j["Peak1 Gain"];
    float new_peak1Quality = j["Peak1 Q"];
    float new_peak2Freq = j["Peak2 Frequency"];
    float new_peak2GainInDecibels = j["Peak2 Gain"];
    float new_peak2Quality = j["Peak2 Q"];
    float new_peak3Freq = j["Peak3 Frequency"];
    float new_peak3GainInDecibels = j["Peak3 Gain"];
    float new_peak3Quality = j["Peak3 Q"];
    float new_highCutFreq = j["HighCut Frequency"];
    float new_highCutSlope = j["HighCut Slope"];

    //auto meter_thread_value = meterThread.value.load();
    //TEST

    std::ofstream parameter_text_file("C:\\Users\\mosie\\Desktop\\Hubert\\Programming\\EQ_Hubert_Mosz\\AdditionalFiles\\parameter_values.txt");

    parameter_text_file << "LowCut Frequency: " << std::to_string(lowCutFreq) << " Hz" << std::endl;
    parameter_text_file << "LowCut Slope: " << std::to_string(lowCutSlope) << " Position (db/Oct)" << std::endl;
    parameter_text_file << "Peak1 Frequency: " << std::to_string(peak1Freq) << " Hz" << std::endl;
    parameter_text_file << "Peak1 Gain: " << std::to_string(peak1GainInDecibels) << " dB" << std::endl;
    parameter_text_file << "Peak1 Q: " << std::to_string(peak1Quality) << " " << std::endl;
    parameter_text_file << "Peak2 Frequency: " << std::to_string(peak2Freq) << " Hz" << std::endl;
    parameter_text_file << "Peak2 Gain: " << std::to_string(peak2GainInDecibels) << " dB" << std::endl;
    parameter_text_file << "Peak2 Q: " << std::to_string(peak2Quality) << " " << std::endl;
    parameter_text_file << "Peak3 Frequency: " << std::to_string(peak3Freq) << " Hz" << std::endl;
    parameter_text_file << "Peak3 Gain: " << std::to_string(peak3GainInDecibels) << " dB" << std::endl;
    parameter_text_file << "Peak3 Q: " << std::to_string(peak3Quality) << " " << std::endl;
    parameter_text_file << "HighCut Frequency: " << std::to_string(highCutFreq) << " Hz" << std::endl;
    parameter_text_file << "HighCut Slope: " << std::to_string(highCutSlope) << " Position (db/Oct)" << std::endl;
    
    //test
    /*
    parameter_text_file << "\nTEST VALUES RETRIEVED FROM ANOTHER JSON" << std::endl;
    parameter_text_file << "LowCut Frequency: " << std::to_string(new_lowCutFreq) << " Hz" << std::endl;
    parameter_text_file << "LowCut Slope: " << std::to_string(new_lowCutSlope) << " Position (db/Oct)" << std::endl;
    parameter_text_file << "Peak1 Frequency: " << std::to_string(new_peak1Freq) << " Hz" << std::endl;
    parameter_text_file << "Peak1 Gain: " << std::to_string(new_peak1GainInDecibels) << " dB" << std::endl;
    parameter_text_file << "Peak1 Q: " << std::to_string(new_peak1Quality) << " " << std::endl;
    parameter_text_file << "Peak2 Frequency: " << std::to_string(new_peak2Freq) << " Hz" << std::endl;
    parameter_text_file << "Peak2 Gain: " << std::to_string(new_peak2GainInDecibels) << " dB" << std::endl;
    parameter_text_file << "Peak2 Q: " << std::to_string(new_peak2Quality) << " " << std::endl;
    parameter_text_file << "Peak3 Frequency: " << std::to_string(new_peak3Freq) << " Hz" << std::endl;
    parameter_text_file << "Peak3 Gain: " << std::to_string(new_peak3GainInDecibels) << " dB" << std::endl;
    parameter_text_file << "Peak3 Q: " << std::to_string(new_peak3Quality) << " " << std::endl;
    parameter_text_file << "HighCut Frequency: " << std::to_string(new_highCutFreq) << " Hz" << std::endl;
    parameter_text_file << "HighCut Slope: " << std::to_string(new_highCutSlope) << " Position (db/Oct)" << std::endl;
    */
    //parameter_text_file << "MeterThread Value: " << std::to_string(meter_thread_value) << std::endl;
    parameter_text_file << "TEST\n" << std::setw(4) << json_from_server_data << std::endl;
    //test

    parameter_text_file.close();

    // json file7

    std::ofstream json_text_file("C:\\Users\\mosie\\Desktop\\Hubert\\Programming\\EQ_Hubert_Mosz\\AdditionalFiles\\json_parameter_values.json");

    json_parameter_data = {
        {"LowCut Frequency", lowCutFreq},
        {"LowCut Slope", lowCutSlope},
        {"Peak3 Q", peak3Quality},
        {"Peak1 Frequency", peak1Freq},
        {"Peak1 Gain", peak1GainInDecibels},
        {"Peak1 Q", peak1Quality},
        {"Peak2 Frequency", peak2Freq},
        {"Peak2 Gain", peak2GainInDecibels},
        {"Peak2 Q", peak2Quality},
        {"Peak3 Frequency", peak3Freq},
        {"Peak3 Gain", peak3GainInDecibels},
        {"HighCut Frequency", highCutFreq},
        {"HighCut Slope", highCutSlope}
    };

    json_text_file << std::setw(4) << json_parameter_data << std::endl;

    json_text_file.close();

    // read a JSON file
    // std::ifstream i("file.json");
    // json j;
    // i >> j;
}

void networkClient()
{
    boost::asio::io_service io_service;
    // socket creation
    boost::asio::ip::tcp::socket client_socket(io_service);

    client_socket.connect(boost::asio::ip::tcp::endpoint
    (boost::asio::ip::address::from_string
    ("127.0.0.1"), 54000));

    // Getting username from user
    std::string u_name, reply, response;
    u_name = "";

    // Sending username to another end
    // to initiate the conversation
    //sendData(client_socket, u_name);

    // Infinite loop for chit-chat
    while (true) {
        //ODBIOR DANYCH

        // Fetching response
        //response = getData(client_socket);
        boost::asio::streambuf buf; //wyciagniecie fcji
        read_until(client_socket, buf, "\n");
        std::string response = buffer_cast<const char*>(buf.data());

        // Popping last character "\n"
        response.pop_back();

        // Validating if the connection has to be closed
        if (response == "exit") {
            //std::cout << "Connection terminated" << std::endl;
            break;
        }

        //system("CLS");
        //std::cout << response << std::endl;

        //ustawienie zmiennej?
        json_from_server_data = response;

        //---------------------------------

        //WYSYLKA DANYCH

        // Reading new message from input stream
        //cout << u_name << "";
        //getline(cin, reply); //tutaj czeka na klawisz wiec lipa
        //reply = "ok";

        //sendData(client_socket, reply /*json_parameter_data*/); //poki co nic nie wysylam
        //boost::asio::write(socket, boost::asio::buffer(reply + "\n")); //wyciagniecie fcji

        //cout << str_i << endl;
        //i = i++;

        if (reply == "exit")
            break;
    }
}

/*std::string getData(boost::asio::ip::tcp::socket& socket)
{
    boost::asio::streambuf buf;
    read_until(socket, buf, "\n");
    std::string data = buffer_cast<const char*>(buf.data());
    return data;
}

void sendData(boost::asio::ip::tcp::socket& socket, const std::string& message)
{
    boost::asio::write(socket,
        boost::asio::buffer(message + "\n"));
}*/

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new EQ_Hubert_MoszAudioProcessor();
}

