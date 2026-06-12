#pragma once

#include "PluginProcessor.h"
#include "Network/NetworkComponent.h"

//==============================================================================
class NetworkedAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit NetworkedAudioProcessorEditor (NetworkedAudioProcessor&);
    ~NetworkedAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    NetworkedAudioProcessor& processorRef;

    // Network component for setup and discovery
    NetworkComponent networkComponent;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NetworkedAudioProcessorEditor)
};