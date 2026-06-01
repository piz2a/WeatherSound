#pragma once
#include <juce_audio_basics/juce_audio_basics.h>

enum class FmPreset 
{
    ClassicWurly,
    LatelyBass,
    Pluck,
    BrassSwell,
    AmbientBell
};

class FmOperator 
{
public:
    FmOperator() = default;

    // Standard lifecycle
    void prepareToPlay(double sampleRate);
    void startNote(float fundamentalFrequency, float velocity);
    void stopNote();

    // The core DSP function. 
    // phaseModulation is the audio output of the modulating operator.
    float getNextSample(float phaseModulation = 0.0f);

    // Preset configuration methods
    void setRatio(float newRatio);
    void setEnvelopeParameters(juce::ADSR::Parameters newParams);
    void setFeedback(float feedbackAmount);
    
    // Voice stealing / release check
    bool isActive() const;

private:
    juce::ADSR envelope;
    
    double currentSampleRate = 44100.0;
    float currentPhase = 0.0f;
    float phaseIncrement = 0.0f;
    
    // Operator-specific settings
    float ratio = 1.0f;
    float feedback = 0.0f;
    float lastOutput = 0.0f; // Needed if you want self-feedback
};