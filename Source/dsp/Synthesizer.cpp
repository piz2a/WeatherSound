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

void SynthVoice::updatePreset(FmPreset preset)
{
    currentPreset = preset;
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
        case FmPreset::LatelyBass:
        {
            // Algorithm 1: A 4-Operator Cascade (4 -> 3 -> 2 -> 1)
            const float depth4to3 = 1.5f;
            const float depth3to2 = 1.2f;
            const float depth2to1 = 1.0f;

            // Op 4 (Top Modulator) - Calculates its sample first (uses internal feedback)
            float op4Output = operators[3].getNextSample(0.0f);
            
            // Op 3 receives Op 4
            float op3Output = operators[2].getNextSample(op4Output * depth4to3);

            // Op 2 receives Op 3
            float op2Output = operators[1].getNextSample(op3Output * depth3to2);
            
            // Op 1 (Carrier) receives Op 2
            float op1Output = operators[0].getNextSample(op2Output * depth2to1);

            // Only Operator 1 produces actual audio output
            output = op1Output * level;
            break;
        }
        case FmPreset::Pluck:
        {
            // Algorithm: 1 Carrier (Op 1) modulated by 3 Parallel Modulators (Ops 2, 3, 4)
            
            // Hardcoded depths for the different physical components
            const float pickDepth = 2.0f;
            const float twangDepth = 1.5f;
            const float bodyDepth = 1.0f;

            // The Modulators calculate their outputs independently (no incoming phase mod)
            float op4Output = operators[3].getNextSample(0.0f);
            float op3Output = operators[2].getNextSample(0.0f);
            float op2Output = operators[1].getNextSample(0.0f);

            // Sum the modulation signals together
            float totalModulation = (op4Output * pickDepth) + 
                                    (op3Output * twangDepth) + 
                                    (op2Output * bodyDepth);

            // The single Carrier receives the massively complex combined modulation
            float op1Output = operators[0].getNextSample(totalModulation);

            // Only the Carrier produces audio
            output = op1Output * level;
            break;
        }
        case FmPreset::BrassSwell:
        {
            // 1 Carrier (Op 1) modulated by 3 Parallel Modulators (Ops 2, 3, 4)
            // We use different modulation depths to weight the harmonics.
            const float mod2Depth = 1.5f; // Fundamental thickness
            const float mod3Depth = 1.0f; // Octave brightness
            const float mod4Depth = 0.5f; // High harmonic bite

            float op4Output = operators[3].getNextSample(0.0f);
            float op3Output = operators[2].getNextSample(0.0f);
            float op2Output = operators[1].getNextSample(0.0f);

            // Sum the modulators
            float totalModulation = (op4Output * mod4Depth) + 
                                    (op3Output * mod3Depth) + 
                                    (op2Output * mod2Depth);

            // Feed them into the Carrier
            float op1Output = operators[0].getNextSample(totalModulation);

            output = op1Output * level;
            break;
        }

        case FmPreset::AmbientBell:
        {
            // Algorithm 6: 3 Parallel Carriers (Ops 1, 2, 3) modulated by 1 Modulator (Op 4)
            const float bellModIndex = 1.2f;

            // Op 4 calculates its phase first
            float op4Output = operators[3].getNextSample(0.0f);
            float sharedModulation = op4Output * bellModIndex;

            // All three carriers receive the EXACT SAME modulation signal from Op 4
            float op3Output = operators[2].getNextSample(sharedModulation);
            float op2Output = operators[1].getNextSample(sharedModulation);
            float op1Output = operators[0].getNextSample(sharedModulation);

            // Mix down the three carriers. 
            // We multiply by 0.33f so summing three full-volume sine waves doesn't clip.
            output = (op1Output + op2Output + op3Output) * 0.33f * level;
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
    else if (currentPreset == FmPreset::LatelyBass)
    {
        // Operator 1 (Carrier): The Fundamental Sub Bass
        operators[0].setRatio(1.0f);
        // Fast attack, medium decay, medium sustain, quick release
        operators[0].setEnvelopeParameters({ 0.005f, 1.5f, 0.4f, 0.1f });

        // Operator 2 (Modulator 1): The Primary Growl
        operators[1].setRatio(1.0f);
        // Decays faster than the carrier to make the sound "darker" over time
        operators[1].setEnvelopeParameters({ 0.005f, 0.8f, 0.0f, 0.1f });

        // Operator 3 (Modulator 2): The Metallic Harmonic
        // Setting this to 2.0 generates the characteristic 5th/octave overtones
        operators[2].setRatio(2.0f); 
        // Shorter decay for a punchy mid-transient
        operators[2].setEnvelopeParameters({ 0.005f, 0.4f, 0.0f, 0.1f });

        // Operator 4 (Modulator 3): The Initial Bite/Click
        operators[3].setRatio(1.0f);
        // Push the feedback up to deform the sine wave into a harsher shape
        operators[3].setFeedback(0.7f); 
        // Lightning-fast decay, this is just the "pick" or "slap" of the bass
        operators[3].setEnvelopeParameters({ 0.001f, 0.15f, 0.0f, 0.1f });
        operators[0].setFeedback(0.0f);
        operators[1].setFeedback(0.0f);
        operators[2].setFeedback(0.0f);
    }
    else if (currentPreset == FmPreset::Pluck)
    {
        //Harmonics
        // Operator 1 (Carrier): The Fundamental String Body
        operators[0].setRatio(1.0f);
        // Instant attack, long decay, NO sustain.
        operators[0].setEnvelopeParameters({ 0.001f, 2.5f, 0.0f, 0.2f }); 

        //  Harmonic Warmth / Wood Resonance
        operators[1].setRatio(1.0f);
        // Decays slightly faster than the fundamental so the tone dulls as it rings
        operators[1].setEnvelopeParameters({ 0.001f, 1.2f, 0.0f, 0.2f }); 


        operators[2].setRatio(3.14f); 
        // Fast decay, only present at the start of the note
        operators[2].setEnvelopeParameters({ 0.001f, 0.3f, 0.0f, 0.1f });

        // Operator 4 (Modulator 3): The Plectrum / Nail Click
        // High, highly dissonant frequency multiple
        operators[3].setRatio(8.7f); 
        // Adding a bit of feedback to Op 4 turns the sine into a noisy impulse
        operators[3].setFeedback(0.4f); 
        // Lightning-fast decay. This is strictly a wideband transient click.
        operators[3].setEnvelopeParameters({ 0.001f, 0.05f, 0.0f, 0.1f }); 
        
        // Ensure feedback is reset for the harmonic operators
        operators[0].setFeedback(0.0f);
        operators[1].setFeedback(0.0f);
        operators[2].setFeedback(0.0f);
    }
    else if (currentPreset == FmPreset::BrassSwell)
    {
        // --- THE SWELL (Manipulating Time) ---
        // Brass is all about the "lip buzz" getting brighter over time.
        // Operator 1 (Carrier): Fundamental Tone
        operators[0].setRatio(1.0f);
        // Carrier opens fairly quickly on key press
        operators[0].setEnvelopeParameters({ 0.05f, 2.0f, 0.8f, 0.4f }); 

        // Operator 2 (Modulator 1): Low Harmonics
        operators[1].setRatio(1.0f);
        // Slower attack. The "swell" begins.
        operators[1].setEnvelopeParameters({ 0.2f, 1.5f, 0.6f, 0.4f }); 

        // Operator 3 (Modulator 2): Mid Harmonics (Octave)
        operators[2].setRatio(2.0f);
        // Even slower attack. The brass gets brighter as the note is held.
        operators[2].setEnvelopeParameters({ 0.35f, 1.5f, 0.4f, 0.4f }); 

        // Operator 4 (Modulator 3): High Harmonics / Bite
        operators[3].setRatio(3.0f); 
        operators[3].setFeedback(0.2f); // Add a little noise/grit for the breath
        // The last harmonic to arrive. 
        operators[3].setEnvelopeParameters({ 0.5f, 1.0f, 0.2f, 0.4f }); 
        
        operators[0].setFeedback(0.0f);
        operators[1].setFeedback(0.0f);
        operators[2].setFeedback(0.0f);
    }
    else if (currentPreset == FmPreset::AmbientBell)
    {
        // Bells require inharmonicity. We avoid whole numbers and use 
        // irrational constants to create a metallic cluster of frequencies.

        // Operator 4 (Modulator): The Shared Inharmonic Generator
        operators[3].setRatio(1.414f);
        operators[3].setFeedback(0.1f);
        operators[3].setEnvelopeParameters({ 0.005f, 4.0f, 0.0f, 4.0f });

        // Operator 3 (Carrier 1): High Chime
        operators[2].setRatio(3.14f); // Pi
        operators[2].setEnvelopeParameters({ 0.005f, 6.0f, 0.0f, 5.0f });

        // Operator 2 (Carrier 2): Mid Ring
        operators[1].setRatio(1.732f); // Square root of 3
        operators[1].setEnvelopeParameters({ 0.005f, 5.0f, 0.0f, 4.0f });

        // Operator 1 (Carrier 3): Deep Fundamental Drone
        // Dropping the ratio to 0.5 puts this carrier an octave below the MIDI note
        operators[0].setRatio(0.5f); 
        // Slower attack for an ambient pad feel, massive release
        operators[0].setEnvelopeParameters({ 0.05f, 8.0f, 0.0f, 6.0f }); 

        operators[0].setFeedback(0.0f);
        operators[1].setFeedback(0.0f);
        operators[2].setFeedback(0.0f);
    }
}