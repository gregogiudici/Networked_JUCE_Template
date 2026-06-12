#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NetworkedAudioProcessor::NetworkedAudioProcessor()
    : AudioProcessor(BusesProperties()
#if !JucePlugin_IsMidiEffect
#if !JucePlugin_IsSynth
                         .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
                         .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
                         ),
      midiMessageCollector(),
      umpDispatcher(0, juce::ump::PacketProtocol::MIDI_1_0, 1024)
{
}

NetworkedAudioProcessor::~NetworkedAudioProcessor()
{
}

//==============================================================================
const juce::String NetworkedAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool NetworkedAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool NetworkedAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool NetworkedAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double NetworkedAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int NetworkedAudioProcessor::getNumPrograms()
{
    return 1; // NB: some hosts don't cope very well if you tell them there are 0 programs,
              // so this should be at least 1, even if you're not really implementing programs.
}

int NetworkedAudioProcessor::getCurrentProgram()
{
    return 0;
}

void NetworkedAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String NetworkedAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void NetworkedAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void NetworkedAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    juce::ignoreUnused(samplesPerBlock);
    midiMessageCollector.reset(sampleRate);
}

void NetworkedAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

bool NetworkedAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if !JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}

void NetworkedAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                           juce::MidiBuffer &midiMessages)
{
    juce::ignoreUnused(midiMessages);

    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // MIDI message handling from Network
    midiMessageCollector.removeNextBlockOfMessages(midiMessages, buffer.getNumSamples());

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // This is the place where you'd normally do the guts of your plugin's
    // audio processing...
    // Make sure to reset the state if your inner loop is processing
    // the samples and the outer loop is handling the channels.
    // Alternatively, you can process the samples with the channels
    // interleaved by keeping the same state.
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto *channelData = buffer.getWritePointer(channel);
        juce::ignoreUnused(channelData);
        // ..do something to the data...
    }

    // Take the messages in the MidiBuffer and send them over the Network
    for (const auto &metadata : midiMessages)
    {
        umpDispatcher.dispatch(metadata.asSpan(),
                               0.0, // We do not care about the timestamp
                               [this](const juce::ump::View &packet, double /*ts*/)
                               {
                                   netUMPThread.sendMidiMessage(packet);
                               });
    }
}

//==============================================================================
bool NetworkedAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor *NetworkedAudioProcessor::createEditor()
{
    return new NetworkedAudioProcessorEditor(*this);
}

//==============================================================================
void NetworkedAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    juce::ignoreUnused(destData);
}

void NetworkedAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    juce::ignoreUnused(data, sizeInBytes);
}

//==============================================================================
// Network MIDI 2.0

juce::String NetworkedAudioProcessor::getRemoteIp() const
{
    return netUMPThread.getRemoteIp();
}

int NetworkedAudioProcessor::getRemotePort() const
{
    return netUMPThread.getRemotePort();
}

int NetworkedAudioProcessor::getLocalPort() const
{
    return netUMPThread.getLocalPort();
}

void NetworkedAudioProcessor::setRemotePeer(const juce::String &ip, int remotePort, int localPort, bool isInitiator)
{
    netUMPThread.configure(ip, (unsigned short)remotePort, (unsigned short)localPort, isInitiator);
}

bool NetworkedAudioProcessor::connectToRemotePeer()
{
    if (netUMPThread.isThreadRunning())
        netUMPThread.stopSession();
    return netUMPThread.startSession();
}

int NetworkedAudioProcessor::getSessionStatus()
{
    return netUMPThread.getSessionStatus();
}

juce::AudioProcessorValueTreeState &NetworkedAudioProcessor::getAPVTS()
{
    return apvts;
}

juce::AudioProcessorValueTreeState::ParameterLayout NetworkedAudioProcessor::createParameterLayout()
{
    // Create the parameter layout for the AudioProcessorValueTreeState
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Add parameters to the layout
    // ...

    return layout;
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new NetworkedAudioProcessor();
}