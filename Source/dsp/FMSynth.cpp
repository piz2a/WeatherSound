#pragma once
#include"FMSynth.h"

void FmOperator::prepareToPlay(double sampleRate)
{
    currentSampleRate = sampleRate;
}

void FmOperator::setEnvelopeParameters(juce::ADSR::Parameters newParams)
{
    envelope.setParameters(newParams);
}

float FmOperator::getNextSample(float phaseModulation)
{
    // 1. Get the current envelope amplitude (0.0 to 1.0)
    float currentEnvelope = envelope.getNextSample();

    // 2. Calculate the sample using Phase Modulation.
    float currentSample = std::sin(currentPhase + phaseModulation);

    // 3. Advance the phase for the next sample tick
    currentPhase += phaseIncrement;
    
    // 4. Wrap the phase to keep it within 0 and 2*PI. 
    if (currentPhase >= juce::MathConstants<float>::twoPi)
    {
        currentPhase -= juce::MathConstants<float>::twoPi;
    }

    // 5. Scale the final output by the envelope amplitude
    lastOutput = currentSample * currentEnvelope;
    
    return lastOutput;
}

