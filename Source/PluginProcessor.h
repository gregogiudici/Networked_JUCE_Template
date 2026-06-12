#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "Network/NetUMPThread.h"

//==============================================================================
class NetworkedAudioProcessor  : public juce::AudioProcessor
{
public:
    //==============================================================================
    NetworkedAudioProcessor();
    ~NetworkedAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    // Network MIDI 2.0

    juce::String getRemoteIp() const;
    int getRemotePort() const;
    int getLocalPort() const;
    void setRemotePeer(const juce::String &ip, int remotePort, int localPort = 5004, bool isInitiator = true);
    bool connectToRemotePeer();
    int getSessionStatus();

    // Access the AudioProcessorValueTreeState
    juce::AudioProcessorValueTreeState &getAPVTS();

private:

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState apvts = {*this, nullptr, "Params", createParameterLayout()};

    //==============================================================================
    // MIDI message handling
    juce::MidiMessageCollector midiMessageCollector;
    // Network UMP thread
    NetUMPThread netUMPThread{midiMessageCollector};
    // UMP dispatcher
    juce::ump::BytestreamToUMPDispatcher umpDispatcher;



    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (NetworkedAudioProcessor)
};