#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_basics/synthesisers/juce_Synthesiser.h>
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
        
        // Convert MIDI note to frequency
        auto cyclesPerSecond = juce::MidiMessage::getMidiNoteInHertz(midiNoteNumber);
    }

    void stopNote (float velocity, bool allowTailOff) override
    {
        if (allowTailOff)
        {
            // Start release phase
            isTargetNoteActive = false; 
        }
        else
        {
            // Shut off instantly
            clearCurrentNote();
        }
    }

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer, int startSample, int numSamples) override;

    void pitchWheelMoved (int newPitchWheelValue) override {}
    void controllerMoved (int controllerNumber, int newControllerValue) override {}

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