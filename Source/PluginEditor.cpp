#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
NetworkedAudioProcessorEditor::NetworkedAudioProcessorEditor (NetworkedAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p), networkComponent (p)
{
    juce::ignoreUnused (processorRef);

    addAndMakeVisible (networkComponent);

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (600, 500);
}

NetworkedAudioProcessorEditor::~NetworkedAudioProcessorEditor()
{
}

//==============================================================================
void NetworkedAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void NetworkedAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    networkComponent.setBounds (getLocalBounds());
}