#include "Synthesizer.h"

void SynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer, 
int startSample, int numSamples)
{
    if (!operators[0].isActive()) 
    {
        clearCurrentNote();
        return;
    }

    // Process sample by sample
    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Get the fully routed FM output for this exact microsecond
        float currentSample = renderAlgorithmSample(); 

        // Additively mix it into all channels (stereo/mono)
        for (int channel = 0; channel < outputBuffer.getNumChannels(); ++channel)
        {
            outputBuffer.addSample(channel, startSample + sample, currentSample);
        }
    }
}

void SynthVoice::updateModDepthA(float mDepth)
{
    modDepthStackA = mDepth;
}

void SynthVoice::updateModDepthB(float mDepth)
{
    modDepthStackB = mDepth;
}

void SynthVoice::updateLevel(float lvl)
{
    level = lvl;
}

void SynthVoice::updateEnvelopeParams(juce::ADSR::Parameters adsr)
{
    
}

float SynthVoice::renderAlgorithmSample() 
{
    float output = 0.0f;

    switch (currentPreset) 
    {
        case FmPreset::ClassicWurly:
        {
            // Modulator (Op 4) calculates its sample first with no incoming modulation
            float op4Output = operators[3].getNextSample(0.0f);
            
            // Carrier (Op 3) receives Op 4's output, scaled by the modulation depth
            float op3Output = operators[2].getNextSample(op4Output * modDepthStackA);

            // Modulator (Op 2) 
            float op2Output = operators[1].getNextSample(0.0f);
            
            // Carrier (Op 1) receives Op 2's output, scaled by the depth
            float op1Output = operators[0].getNextSample(op2Output * modDepthStackB);


            // mix
            // both carriers (Op 1 and Op 3) produce actual audio. 
            // we sum them together and divide by 2.0f so they don't clip the audio engine.
            output = (op1Output + op3Output) * 0.5f * level;
            break;
        }
        default:
            break;
    }

    return output;
}

void SynthVoice::applyPresetParameters()
{
    if (currentPreset == FmPreset::ClassicWurly)
    {
        // --- STACK B: The Sustained "Body" ---
        operators[0].setRatio(1.0f); // Carrier 1
        operators[1].setRatio(1.0f); // Modulator 2

        // A warm, sustained envelope (Attack, Decay, Sustain, Release)
        juce::ADSR::Parameters bodyEnv { 0.01f, 3.0f, 0.0f, 0.5f }; 
        operators[0].setEnvelopeParameters(bodyEnv);
        operators[1].setEnvelopeParameters(bodyEnv);


        // --- STACK A: The Transient "Tine" ---
        operators[2].setRatio(1.0f);  // Carrier 3
        operators[3].setRatio(14.0f); // Modulator 4 (High frequency ping!)

        // A sharp, percussive envelope with NO sustain
        juce::ADSR::Parameters tineEnv { 0.001f, 0.1f, 0.0f, 0.1f };
        operators[2].setEnvelopeParameters(tineEnv);
        operators[3].setEnvelopeParameters(tineEnv);
    }
}