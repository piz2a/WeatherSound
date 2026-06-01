#pragma once
#include "Synthesizer.h"

void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, 
int startSample, int numSamples)
{

}

float SynthVoice::renderAlgorithmSample() 
{
    float output = 0.0f;

    switch (currentPreset) 
    {
        case FmPreset::ClassicWurly:
        {
            // Stack A: Op 4 modulates Op 3
            float op4Out = operators[3].getNextSample(0.0f);
            float op3Out = operators[2].getNextSample(op4Out * modulationDepth); // Op 3 is a carrier

            // Stack B: Op 2 modulates Op 1
            float op2Out = operators[1].getNextSample(0.0f);
            float op1Out = operators[0].getNextSample(op2Out * modulationDepth); // Op 1 is a carrier
            
            // Mix the carriers
            output = (op1Out + op3Out) * 0.5f;
            break;
        }
        // ... other cases for other algorithms
    }
    return output;
}