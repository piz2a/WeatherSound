#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include"FMSynth.h"

enum class FmPreset 
{
    ClassicWurly,
    LatelyBass,
    Pluck,
    BrassSwell,
    AmbientBell
};

class SynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote (int /*midiNoteNumber*/) override  { return true; }
    bool appliesToChannel (int /*midiChannel*/) override { return true; }
};

// The actual rendering engine for a single voice
class SynthVoice : public juce::SynthesiserVoice
{
public:
    bool canPlaySound (juce::SynthesiserSound* sound) override
    {
        return dynamic_cast<SynthSound*> (sound) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity, 
                juce::SynthesiserSound* sound, int currentPitchWheelPosition) override
    {
        isTargetNoteActive = true;
        level = velocity;
        
        auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);

        // 1. Configure the ratios and envelopes for the current preset
        applyPresetParameters(); 

        // 2. Fire the operators!
        for (auto& op : operators)
        {
            op.startNote(static_cast<float>(cyclesPerSecond), velocity);
        }
    }

    void stopNote (float velocity, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            // Tell all operators to begin their ADSR release phase
            for (auto& op : operators)
            {
                op.stopNote(); 
            }
        }
        else
        {
            clearCurrentNote();
        }
    }

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void pitchWheelMoved (int newPitchWheelValue) override {}
    void controllerMoved (int controllerNumber, int newControllerValue) override {}
    void setCurrentPlaybackSampleRate (double newRate) override
{
    // Call the base class implementation first
    juce::SynthesiserVoice::setCurrentPlaybackSampleRate(newRate);
    
    // Pass the sample rate down to all 4 operators
    for (auto& op : operators)
    {
        op.prepareToPlay(newRate);
    }
}

    void updateEnvelopeParams (juce::ADSR::Parameters adsr);
    void updateModDepthA (float mDepth);
    void updateModDepthB (float mDepth);
    void updateLevel (float lv);

private:
    float level = 0.0f;
    float phase = 0.0f;
    bool isTargetNoteActive = false;

    float modDepthStackA = 1.5f;
    float modDepthStackB = 0.8;
    std::array<FmOperator, 4> operators;
    FmPreset currentPreset = FmPreset::ClassicWurly;

    float renderAlgorithmSample(); 
    void applyPresetParameters();
};