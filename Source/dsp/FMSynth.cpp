#include "FMSynth.h"
#include <cmath> // For std::sin

void FmOperator::prepareToPlay(double sampleRate)
{
    currentSampleRate = sampleRate;
    
    envelope.setSampleRate(sampleRate);
    
    currentPhase = 0.0f;
    lastOutput = 0.0f;
}

void FmOperator::startNote(float fundamentalFrequency, float velocity)
{
    // calculate the actual frequency of this specific operator 
    // based on the incoming MIDI pitch and the operator's ratio.
    float operatorFrequency = fundamentalFrequency * ratio;
    
    // how much the phase should advance per sample tick.
    phaseIncrement = (operatorFrequency * juce::MathConstants<float>::twoPi) / static_cast<float>(currentSampleRate);
    
    //reset phase for a punchy, consistent attack transient. 
    currentPhase = 0.0f;
    lastOutput = 0.0f;

    //Trigger the envelope
    envelope.noteOn();
}

void FmOperator::stopNote()
{
    // Begin the release phase of the ADSR
    envelope.noteOff();
}

float FmOperator::getNextSample(float phaseModulation)
{
    // Get the current amplitude envelope value (0.0 to 1.0)
    float currentEnvelope = envelope.getNextSample();

    // Calculate the sine wave.
    float modulatedPhase = currentPhase + phaseModulation + (lastOutput * feedback);
    float currentSample = std::sin(modulatedPhase);

    //Advance the internal phase for the next tick
    currentPhase += phaseIncrement;
    
    // Wrap the phase. 
    if (currentPhase >= juce::MathConstants<float>::twoPi)
    {
        currentPhase -= juce::MathConstants<float>::twoPi;
    }

    //Apply the envelope and store the output for the next tick's feedback calculation
    lastOutput = currentSample * currentEnvelope;
    
    return lastOutput;
}

void FmOperator::setRatio(float newRatio)
{
    ratio = newRatio;
}

void FmOperator::setEnvelopeParameters(juce::ADSR::Parameters newParams)
{
    envelope.setParameters(newParams);
}

void FmOperator::setFeedback(float feedbackAmount)
{
    // Feedback in FM can blow up the signal quickly if pushed too high.

    feedback = feedbackAmount; 
}

bool FmOperator::isActive() const
{
    // The operator is active as long as the envelope hasn't finished its release phase
    return envelope.isActive();
}