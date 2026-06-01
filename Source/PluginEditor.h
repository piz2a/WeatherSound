#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

using namespace juce;

//==============================================================================
/**
*/
class WeatherSoundAudioProcessorEditor  : public AudioProcessorEditor, private Slider::Listener  // [2]
{
public:
    WeatherSoundAudioProcessorEditor (WeatherSoundAudioProcessor&);
    ~WeatherSoundAudioProcessorEditor() override;

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    void sliderValueChanged (Slider* slider) override;  // [3]
    auto getResource(const juce::String& url) const -> std::optional<juce::WebBrowserComponent::Resource>;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    WeatherSoundAudioProcessor& audioProcessor;

    Slider frequencySlider;
    TextButton bypassButton;
    Label frequencyLabel {"FrequencyLabel", "Frequency"};
    WebSliderRelay freqRelay { "freqHz" };
    WebSliderParameterAttachment freqAttachment {
        *audioProcessor.getState().getParameter("freqHz"), freqRelay, nullptr
    };
    WebSliderRelay resonanceRelay { "resonance" };
    WebSliderParameterAttachment resonanceAttachment {
        *audioProcessor.getState().getParameter("resonance"), resonanceRelay, nullptr
    };
    WebSliderRelay cloudCoverageRelay { "cloudCoverage" };
    WebSliderParameterAttachment cloudCoverageAttachment {
        *audioProcessor.getState().getParameter("cloudCoverage"), cloudCoverageRelay, nullptr
    };
    WebSliderRelay humidityRelay { "humidity" };
    WebSliderParameterAttachment humidityAttachment {
        *audioProcessor.getState().getParameter("humidity"), humidityRelay, nullptr
    };
    WebSliderRelay temperatureRelay { "temperature" };
    WebSliderParameterAttachment temperatureAttachment {
        *audioProcessor.getState().getParameter("temperature"), temperatureRelay, nullptr
    };
    WebSliderRelay uvIndexRelay { "uvIndex" };
    WebSliderParameterAttachment uvIndexAttachment {
        *audioProcessor.getState().getParameter("uvIndex"), uvIndexRelay, nullptr
    };
    WebSliderRelay windSpeedRelay { "windSpeed" };
    WebSliderParameterAttachment windSpeedAttachment {
        *audioProcessor.getState().getParameter("windSpeed"), windSpeedRelay, nullptr
    };
    WebSliderRelay windDirectionRelay { "windDirection" };
    WebSliderParameterAttachment windDirectionAttachment {
        *audioProcessor.getState().getParameter("windDirection"), windDirectionRelay, nullptr
    };
    WebSliderRelay visibilityRelay { "visibility" };
    WebSliderParameterAttachment visibilityAttachment {
        *audioProcessor.getState().getParameter("visibility"), visibilityRelay, nullptr
    };
    // Relays and attachments for the mix parameters
    WebSliderRelay cloudCoverageMixRelay { "cloudCoverageMix" };
    WebSliderParameterAttachment cloudCoverageMixAttachment {
        *audioProcessor.getState().getParameter("cloudCoverageMix"), cloudCoverageMixRelay, nullptr
    };
    WebSliderRelay humidityMixRelay { "humidityMix" };
    WebSliderParameterAttachment humidityMixAttachment {
        *audioProcessor.getState().getParameter("humidityMix"), humidityMixRelay, nullptr
    };
    WebSliderRelay temperatureMixRelay { "temperatureMix" };
    WebSliderParameterAttachment temperatureMixAttachment {
        *audioProcessor.getState().getParameter("temperatureMix"), temperatureMixRelay, nullptr
    };
    WebSliderRelay uvIndexMixRelay { "uvIndexMix" };
    WebSliderParameterAttachment uvIndexMixAttachment {
        *audioProcessor.getState().getParameter("uvIndexMix"), uvIndexMixRelay, nullptr
    };
    WebSliderRelay windSpeedMixRelay { "windSpeedMix" };
    WebSliderParameterAttachment windSpeedMixAttachment {
        *audioProcessor.getState().getParameter("windSpeedMix"), windSpeedMixRelay, nullptr
    };
    WebSliderRelay windDirectionMixRelay { "windDirectionMix" };
    WebSliderParameterAttachment windDirectionMixAttachment {
        *audioProcessor.getState().getParameter("windDirectionMix"), windDirectionMixRelay, nullptr
    };
    WebSliderRelay visibilityMixRelay { "visibilityMix" };
    WebSliderParameterAttachment visibilityMixAttachment {
        *audioProcessor.getState().getParameter("visibilityMix"), visibilityMixRelay, nullptr
    };
    WebToggleButtonRelay bypassRelay { "bypass" };
    WebToggleButtonParameterAttachment bypassAttachment {
        *audioProcessor.getState().getParameter("bypass"), bypassRelay, nullptr
    };

    WebBrowserComponent webComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (WeatherSoundAudioProcessorEditor)
};
