#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_events/juce_events.h>
#include <juce_core/juce_core.h>
#include <jucey_bonjour/jucey_bonjour.h>
#include <NetUMP/Network.h>
#include "../PluginProcessor.h"

class NetworkComponent : public juce::Component,
                         private juce::Timer,
                         private juce::ValueTree::Listener
{
public:
    struct DiscoveredPeer
    {
        juce::String serviceName;
        juce::String hostName;
        juce::String ipAddress;
        int remotePort = 0;
    };

    explicit NetworkComponent(NetworkedAudioProcessor &processor);
    ~NetworkComponent() override;

    void paint(juce::Graphics &g) override;
    void resized() override;

    void refreshFromProcessor();
    void setConnectionStatusLabelVisible(bool shouldBeVisible);

private:
    static constexpr const char *discoveredListProperty = "discoveredList";
    static constexpr const char *connectedIndexProperty = "connectedDeviceIndex";
    static constexpr const char *connectionStatusProperty = "connectionStatus";
    static constexpr const char *discoveryStatusProperty = "discoveryStatus";
    static constexpr const char *remoteIpProperty = "remoteIP";
    static constexpr const char *remotePortProperty = "remotePort";
    static constexpr const char *localPortProperty = "localPort";
    static constexpr const char *initiatorProperty = "initiator";
    static constexpr const char *serviceType = "_midi2._udp";

    // ValueTree::Listener
    void valueTreePropertyChanged(juce::ValueTree &treeWhosePropertyHasChanged,
                                  const juce::Identifier &property) override;
    void valueTreeChildAdded(juce::ValueTree &, juce::ValueTree &) override {}
    void valueTreeChildRemoved(juce::ValueTree &, juce::ValueTree &, int) override {}
    void valueTreeChildOrderChanged(juce::ValueTree &, int, int) override {}
    void valueTreeParentChanged(juce::ValueTree &) override {}

    // Timer
    void timerCallback() override;

    // Utility
    void commitRemotePeerSettings();
    void updateConnectButtonColour();
    void updateDiscoveredServicesCombo();
    void startDiscovery();
    void stopDiscovery();
    void clearDiscoveredPeers();
    void addOrUpdateDiscoveredPeer(const DiscoveredPeer &peer);
    void removeDiscoveredPeerByName(const juce::String &serviceName);
    void resolveService(const juce::String &serviceName,
                        const juce::String &serviceType,
                        const juce::String &domain);

    static juce::String makePeerDisplayText(const DiscoveredPeer &peer);

    NetworkedAudioProcessor &processorRef;

    juce::Label titleLabel{{}, "Network MIDI 2.0"};

    juce::GroupComponent discoveryGroup { {}, "Discovery" };
    juce::Label discoveredServicesLabel{{}, "Discovered Devices [0]"};
    juce::ComboBox discoveredServicesCombo{"Discovered Devices"};
    juce::TextButton discoverButton{"Rescan"};
    juce::TextButton clearButton{"Clear"};

    juce::GroupComponent configurationGroup { {}, "Configuration" };
    juce::Label remoteIpLabel{{}, "Remote IP"};
    juce::TextEditor remoteIpEditor{"Remote IP"};
    juce::Label remotePortLabel{{}, "Remote Port"};
    juce::TextEditor remotePortEditor{"Remote Port"};
    juce::Label localPortLabel{{}, "Local Port"};
    juce::TextEditor localPortEditor{"Local Port"};
    juce::ToggleButton initiatorToggleButton{"Session Initiator"};

    juce::Label connectionLabel{{}, "Disconnected"};
    juce::TextButton connectButton{"Connect"};

    std::unique_ptr<jucey::BonjourService> discoveryService;
    juce::Array<DiscoveredPeer> discoveredPeers;
    std::vector<std::unique_ptr<jucey::BonjourService>> activeResolvers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetworkComponent)
};