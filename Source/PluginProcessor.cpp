#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
WeatherSoundAudioProcessor::WeatherSoundAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       ), state(*this, nullptr, "parameters", createParameters())
#endif
{
    fmSynth.addSound(new SynthSound());

    // polyphony
    for (int i = 0; i < 8; ++i)
    {
        fmSynth.addVoice(new SynthVoice());
    }
}

WeatherSoundAudioProcessor::~WeatherSoundAudioProcessor()
{
}

//==============================================================================
const String WeatherSoundAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool WeatherSoundAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool WeatherSoundAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool WeatherSoundAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double WeatherSoundAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int WeatherSoundAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int WeatherSoundAudioProcessor::getCurrentProgram()
{
    return 0;
}

void WeatherSoundAudioProcessor::setCurrentProgram (int index)
{
}

const String WeatherSoundAudioProcessor::getProgramName (int index)
{
    return {};
}

void WeatherSoundAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void WeatherSoundAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    filters.resize(getTotalNumOutputChannels());
    for (auto& filter : filters) {
        filter.prepare(sampleRate);
        filter.setCutoffFrequency(1000.0f);
        filter.setQ(0.707f);
        filter.setCoefficients();  // mandatory
    }

    fmSynth.setCurrentPlaybackSampleRate(sampleRate);
    for (int i = 0; i < fmSynth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SynthVoice*>(fmSynth.getVoice(i)))
        {
            voice->setCurrentPlaybackSampleRate(sampleRate);
        }
    }

  freqParam = state.getRawParameterValue("freqHz");
  resonanceParam = state.getRawParameterValue("resonance");
    cloudCoverageParam = state.getRawParameterValue("cloudCoverage");
    humidityParam = state.getRawParameterValue("humidity");
    temperatureParam = state.getRawParameterValue("temperature");
    uvIndexParam = state.getRawParameterValue("uvIndex");
    windSpeedParam = state.getRawParameterValue("windSpeed");
    windDirectionParam = state.getRawParameterValue("windDirection");
    visibilityParam = state.getRawParameterValue("visibility");
    bypassParam = state.getRawParameterValue("bypass");
    smoothedFreq.reset(sampleRate, 0.05); // 50ms 동안 부드럽게 변화
    smoothedFreq.setCurrentAndTargetValue(freqParam->load());
}

void WeatherSoundAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool WeatherSoundAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
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

void WeatherSoundAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    auto numSamples = buffer.getNumSamples();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    // If your algorithm only overwrites some of the output channels, make sure to
    // keep this code to avoid leaving garbage in the remaining output channels.
    for (auto i = 0; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    float temperature = temperatureParam->load();
    FmPreset preset = getPreset(temperature);
    // Iterate through all available voices in the synth manager

    for (int i = 0; i < fmSynth.getNumVoices(); ++i)
    {
        if (auto* voice = dynamic_cast<SynthVoice*>(fmSynth.getVoice(i)))
        {
            voice->updatePreset(preset);
            voice->updateLevel(0.5);
        }
    }
    fmSynth.renderNextBlock(buffer, midiMessages, 0, numSamples);

    const float freq = cloudCoverageParam->load();
    smoothedFreq.setTargetValue(freq);
    float currentFreq = smoothedFreq.getNextValue();
    smoothedFreq.skip(numSamples - 1);

    const float res = uvIndexParam->load();
    const float q = 0.707f * Decibels::decibelsToGain (res);  // Convert dB to linear gain. 0 dB = 0.707
    smoothedQ.setTargetValue(q);
    float currentQ = smoothedQ.getNextValue();
    smoothedQ.skip(numSamples - 1);
    const bool shouldBeBypassed = static_cast<bool>(bypassParam->load());

    /*static float lastFreq = -1.0f;
    if (std::abs(freq - lastFreq) > 0.001f) {
        for (auto& filter : filters) {
            filter.setCutoffFrequency(freq);
            // shouldBeBypassed is not used yet
            filter.setCoefficients();
        }
        lastFreq = freq;
    }*/

    for (int channel = 0; channel < totalNumOutputChannels; ++channel) {
        auto* channelData = buffer.getWritePointer(channel);

        filters[channel].setCutoffFrequency(currentFreq);
        filters[channel].setQ(currentQ);
        filters[channel].setCoefficients();

        if (!shouldBeBypassed) {
            filters[channel].process(channelData, numSamples);
        } else {
            // bypass
        }
    }

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
//    for (int channel = 0; channel < totalNumInputChannels; ++channel)
//    {
//        auto* channelData = buffer.getWritePointer (channel);
//
//        // ..do something to the data...
//    }
}

//==============================================================================
bool WeatherSoundAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* WeatherSoundAudioProcessor::createEditor()
{
    return new WeatherSoundAudioProcessorEditor (*this);
}

//==============================================================================
void WeatherSoundAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void WeatherSoundAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

FmPreset WeatherSoundAudioProcessor::getPreset(float temperature)
{
    if (temperature > -20.0 && temperature <= 0.0) return FmPreset::ClassicWurly;
    if (temperature > 0.0 && temperature <= 15.0) return FmPreset::LatelyBass;
    if (temperature > 15.0 && temperature <= 25.0) return FmPreset::Pluck;
    if (temperature > 25.0 && temperature <= 35.0) return FmPreset::AmbientBell;
    if (temperature > 35.0) return FmPreset::BrassSwell;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new WeatherSoundAudioProcessor();
}

AudioProcessorValueTreeState::ParameterLayout WeatherSoundAudioProcessor::createParameters()
{
    
        StringArray places = {"Bogota",  "Seoul",     "Kalamazoo", "Tokyo",
                              "Ushuaia", "Cape Town", "Austin",    "Dubai"};
    return {
    std::make_unique<AudioParameterFloat>(ParameterID{"freqHz", 1},
                                            "Frequency", 20.0f, 22050.0f, 1500.0f),
     std::make_unique<AudioParameterFloat>(ParameterID{"resonance", 1},
                                            "Resonance", 0.0f, 10.0f, 0.0f),
        std::make_unique<AudioParameterFloat> (  // why use make_unique? because the createParameters function needs to return a ParameterLayout object, which is a vector of unique pointers to RangedAudioParameter objects. By using make_unique, we can create a new AudioParameterFloat object and automatically wrap it in a unique pointer, which is then added to the ParameterLayout vector.
            ParameterID { "cloudCoverage", 1 },
            "Cloud Coverage",
            0.0f,
            100.0f,
            50.0f
        ),
        std::make_unique<AudioParameterFloat> (
            ParameterID { "humidity", 1 },
            "Humidity",
            0.0f,
            100.0f,
            50.0f
        ),
        std::make_unique<AudioParameterFloat> (
            ParameterID { "temperature", 1 },
            "Temperature",
            -20.0f,
            40.0f,
            20.0f
        ),
        std::make_unique<AudioParameterFloat> (
            ParameterID { "uvIndex", 1 },
            "UV Index",
            0.0f,
            11.0f,
            5.0f
        ),
        std::make_unique<AudioParameterFloat> (
            ParameterID { "windSpeed", 1 },
            "Wind Speed",
            0.0f,
            30.0f,
            15.0f
        ),
        std::make_unique<AudioParameterFloat> (
            ParameterID { "windDirection", 1 },
            "Wind Direction",
            0.0f,
            360.0f,
            180.0f
        ),
        std::make_unique<AudioParameterFloat> (
            ParameterID { "visibility", 1 },
            "Visibility",
            0.0f,
            10.0f,
            5.0f
        ),
        std::make_unique<AudioParameterFloat> (
            ParameterID { "cloudCoverageMix", 1 },
            "Cloud Coverage Mix",
            0.0f,
            100.0f,
            50.0f
        ),
        std::make_unique<AudioParameterFloat> (
            ParameterID { "humidityMix", 1 },
            "Humidity Mix",
            0.0f,
            100.0f,
            50.0f
        ),
        std::make_unique<AudioParameterFloat> (
            ParameterID { "temperatureMix", 1 },
            "Temperature Mix",
            0.0f,
            100.0f,
            50.0f
        ),
        std::make_unique<AudioParameterFloat> (
            ParameterID { "uvIndexMix", 1 },
            "UV Index Mix",
            0.0f,
            100.0f,
            50.0f
        ),
        std::make_unique<AudioParameterFloat> (
            ParameterID { "windSpeedMix", 1 },
            "Wind Speed Mix",
            0.0f,
            100.0f,
            50.0f
        ),
        std::make_unique<AudioParameterFloat> (
            ParameterID { "windDirectionMix", 1 },
            "Wind Direction Mix",
            0.0f,
            100.0f,
            50.0f
        ),
        std::make_unique<AudioParameterFloat> (
            ParameterID { "visibilityMix", 1 },
            "Visibility Mix",
            0.0f,
            100.0f,
            50.0f
        ),
        std::make_unique<AudioParameterBool> (
            ParameterID { "bypass", 1 },
            "Bypass",
            false
        ),
        std::make_unique<AudioParameterChoice>(ParameterID{"location", 1}, "Location",
                    places, 0)
    };
}