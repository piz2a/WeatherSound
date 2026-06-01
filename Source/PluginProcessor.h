#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "SimpleFilterChannel.h"
#include "dsp/Synthesizer.h"

using namespace juce;

//==============================================================================
/**
*/
class WeatherSoundAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    WeatherSoundAudioProcessor();
    ~WeatherSoundAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    AudioProcessorValueTreeState& getState() { return state; }

private:
    std::vector<SimpleFilterChannel> filters;
    LinearSmoothedValue<float> smoothedFreq;
    LinearSmoothedValue<float> smoothedQ;
    AudioProcessorValueTreeState state;
    AudioProcessorValueTreeState::ParameterLayout createParameters();
    std::atomic<float>* cloudCoverageParam;
    std::atomic<float>* humidityParam;
    std::atomic<float>* temperatureParam;
    std::atomic<float>* uvIndexParam;
    std::atomic<float>* windSpeedParam;
    std::atomic<float>* windDirectionParam;
    std::atomic<float>* visibilityParam;
    std::atomic<float>* bypassParam;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WeatherSoundAudioProcessor)
};
