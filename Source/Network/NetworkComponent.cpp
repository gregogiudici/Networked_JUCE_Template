#include "NetworkComponent.h"

NetworkComponent::NetworkComponent(NetworkedAudioProcessor &processor)
    : processorRef(processor)
{

    processorRef.getAPVTS().state.addListener(this);

    addAndMakeVisible(titleLabel);

    discoveryGroup.setText("Discovery");
    discoveryGroup.setTextLabelPosition(juce::Justification::centredLeft);

    configurationGroup.setText("Configuration");
    configurationGroup.setTextLabelPosition(juce::Justification::centredLeft);

    addAndMakeVisible(discoveryGroup);
    addAndMakeVisible(configurationGroup);

    addAndMakeVisible(discoveredServicesLabel);
    addAndMakeVisible(discoveredServicesCombo);
    addAndMakeVisible(discoverButton);
    addAndMakeVisible(clearButton);

    addAndMakeVisible(remoteIpLabel);
    addAndMakeVisible(remoteIpEditor);

    addAndMakeVisible(remotePortLabel);
    addAndMakeVisible(remotePortEditor);

    addAndMakeVisible(localPortLabel);
    addAndMakeVisible(localPortEditor);

    addAndMakeVisible(connectionLabel);
    addAndMakeVisible(connectButton);

    addAndMakeVisible(initiatorToggleButton);

    initiatorToggleButton.setClickingTogglesState(true);
    initiatorToggleButton.setToggleState(true,
                                         juce::dontSendNotification);

    titleLabel.setJustificationType(juce::Justification::centredLeft);
    titleLabel.setFont(juce::Font(22.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    discoveredServicesLabel.setJustificationType(juce::Justification::centredLeft);
    remoteIpLabel.setJustificationType(juce::Justification::right);
    remotePortLabel.setJustificationType(juce::Justification::right);
    localPortLabel.setJustificationType(juce::Justification::right);

    discoveredServicesLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    remoteIpLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    remotePortLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    localPortLabel.setColour(juce::Label::textColourId, juce::Colours::white);

    connectionLabel.setJustificationType(juce::Justification::centred);
    connectionLabel.setColour(juce::Label::textColourId, juce::Colours::black);
    connectionLabel.setColour(juce::Label::backgroundColourId, juce::Colours::lightcoral);
    connectionLabel.setColour(juce::Label::outlineColourId, juce::Colours::transparentBlack);
    connectionLabel.setFont(juce::Font(20.0f, juce::Font::bold));

    juce::Font editorFont(16.0f);

    remoteIpEditor.setInputRestrictions(64, "0123456789.");
    remoteIpEditor.setTextToShowWhenEmpty("127.0.0.1", juce::Colours::grey);
    remoteIpEditor.setJustification(juce::Justification::centred);
    remoteIpEditor.applyFontToAllText(editorFont);
    remoteIpEditor.setIndents(0, 0);

    remotePortEditor.setInputRestrictions(5, "0123456789");
    remotePortEditor.setTextToShowWhenEmpty("5000", juce::Colours::grey);
    remotePortEditor.setJustification(juce::Justification::centred);
    remotePortEditor.applyFontToAllText(editorFont);
    remotePortEditor.setIndents(0, 0);

    localPortEditor.setInputRestrictions(5, "0123456789");
    localPortEditor.setTextToShowWhenEmpty("5001", juce::Colours::grey);
    localPortEditor.setJustification(juce::Justification::centred);
    localPortEditor.applyFontToAllText(editorFont);
    localPortEditor.setIndents(0, 0);

    discoveredServicesCombo.setTextWhenNothingSelected("Select discovered device");
    discoveredServicesCombo.setTooltip("Select a discovered Bonjour service");
    discoverButton.setTooltip("Browse the local network for advertised peers");
    clearButton.setTooltip("Clear discovered devices");
    connectButton.setTooltip("Apply network settings and connect");

    remoteIpEditor.onReturnKey = [this]
    {
        processorRef.getAPVTS().state.setProperty(juce::Identifier(remoteIpProperty), remoteIpEditor.getText(), nullptr);
    };
    remotePortEditor.onReturnKey = [this]
    {
        processorRef.getAPVTS().state.setProperty(juce::Identifier(remotePortProperty), remotePortEditor.getText(), nullptr);
    };

    localPortEditor.onReturnKey = [this]
    {
        processorRef.getAPVTS().state.setProperty(juce::Identifier(localPortProperty), localPortEditor.getText(), nullptr);
    };

    remoteIpEditor.onFocusLost = [this]
    {
        processorRef.getAPVTS().state.setProperty(juce::Identifier(remoteIpProperty), remoteIpEditor.getText(), nullptr);
    };
    remotePortEditor.onFocusLost = [this]
    {
        processorRef.getAPVTS().state.setProperty(juce::Identifier(remotePortProperty), remotePortEditor.getText(), nullptr);
    };
    localPortEditor.onFocusLost = [this]
    {
        processorRef.getAPVTS().state.setProperty(juce::Identifier(localPortProperty), localPortEditor.getText(), nullptr);
    };

    initiatorToggleButton.onClick = [this]
    {
        if (initiatorToggleButton.getToggleState())
            processorRef.getAPVTS().state.setProperty(juce::Identifier(initiatorProperty), "True", nullptr);
        else
            processorRef.getAPVTS().state.setProperty(juce::Identifier(initiatorProperty), "False", nullptr);
    };

    discoveredServicesCombo.onChange = [this]
    {
        const auto index = discoveredServicesCombo.getSelectedItemIndex();
        if (!juce::isPositiveAndBelow(index, discoveredPeers.size()))
            return;

        processorRef.getAPVTS().state.setPropertyExcludingListener(
            this,
            juce::Identifier("connectedDeviceIndex"),
            index,
            nullptr);

        const auto &peer = discoveredPeers.getReference(index);

        // Remove any white space
        auto host = peer.ipAddress.trim();

        // Remove trailing dot
        if (host.endsWithChar('.'))
            host = host.dropLastCharacters(1);

        // Resolve hostName to IP address as requested
        juce::String resolvedIPv4 = host;
        {
            unsigned long outIp = 0;

            const bool isDottedIPv4 = [&host]()
            {
                int parts = 0;
                int start = 0;

                while (start < host.length())
                {
                    auto dot = host.indexOfChar(start, '.');
                    auto token = (dot >= 0) ? host.substring(start, dot) : host.substring(start);

                    if (token.isEmpty() || !token.containsOnly("0123456789"))
                        return false;

                    const int value = token.getIntValue();
                    if (value < 0 || value > 255)
                        return false;

                    ++parts;

                    if (dot < 0)
                        break;

                    start = dot + 1;
                }

                return parts == 4;
            }();

            if (isDottedIPv4)
            {
                resolvedIPv4 = host;
            }
            else if (ResolveHostNameIPv4Ex(host.toRawUTF8(),
                                           (unsigned short)peer.remotePort,
                                           &outIp)) // From <Network.h>
            {
                struct in_addr addr;
                addr.s_addr = htonl(outIp);

                if (const char *ipText = inet_ntoa(addr))
                    resolvedIPv4 = juce::String(ipText);
            }
        }

        remoteIpEditor.setText(resolvedIPv4, juce::dontSendNotification);
        remotePortEditor.setText(juce::String(peer.remotePort), juce::dontSendNotification);

        commitRemotePeerSettings();
    };

    discoverButton.onClick = [this]
    {
        stopDiscovery();
        startDiscovery();
    };

    clearButton.onClick = [this]
    {
        clearDiscoveredPeers();
    };

    connectButton.onClick = [this]
    {
        commitRemotePeerSettings();
        processorRef.connectToRemotePeer();
        updateConnectButtonColour();
    };

    refreshFromProcessor();
    updateConnectButtonColour();
    startTimerHz(5);
}

NetworkComponent::~NetworkComponent()
{
    stopTimer();
    stopDiscovery();
    processorRef.getAPVTS().state.removeListener(this);
}

void NetworkComponent::paint(juce::Graphics &g)
{
    // g.fillAll(juce::Colours::black);
}

void NetworkComponent::resized()
{
    auto area = getLocalBounds().reduced(22);

    // Header
    auto header = area.removeFromTop(36);
    titleLabel.setBounds(header.removeFromLeft(180));
    header.removeFromLeft(12);
    connectionLabel.setBounds(header.removeFromRight(180));

    area.removeFromTop(14);

    // Discovery group on top
    auto discoveryArea = area.removeFromTop(90);
    discoveryGroup.setBounds(discoveryArea);

    {
        auto content = discoveryArea.reduced(12);
        content.removeFromTop(22); // leave space for GroupComponent title line
        content.removeFromTop(8);

        auto discoveryRow = content.removeFromTop(34);
        discoveredServicesLabel.setBounds(discoveryRow.removeFromLeft(150));
        discoveryRow.removeFromLeft(8);
        discoverButton.setBounds(discoveryRow.removeFromRight(110));
        discoveryRow.removeFromRight(8);
        clearButton.setBounds(discoveryRow.removeFromRight(80));
        discoveryRow.removeFromRight(8);
        discoveredServicesCombo.setBounds(discoveryRow);
    }

    area.removeFromTop(12);

    // Bottom button area reserved first
    auto bottomButtonArea = area.removeFromBottom(86);
    area.removeFromBottom(8);

    // Configuration group underneath
    configurationGroup.setBounds(area);

    {
        auto content = area.reduced(12);
        content.removeFromTop(22); // leave space for GroupComponent title line
        // content.removeFromTop(8);

        const int rowHeight = 32;
        const int rowGap = 8;
        const int labelWidth = 100;

        auto row1 = content.removeFromTop(rowHeight);
        remoteIpLabel.setBounds(row1.removeFromLeft(labelWidth));
        row1.removeFromLeft(8);
        remoteIpEditor.setBounds(row1);

        content.removeFromTop(rowGap);

        auto row2 = content.removeFromTop(rowHeight);
        remotePortLabel.setBounds(row2.removeFromLeft(labelWidth));
        row2.removeFromLeft(8);
        remotePortEditor.setBounds(row2);

        content.removeFromTop(rowGap);

        auto row3 = content.removeFromTop(rowHeight);
        localPortLabel.setBounds(row3.removeFromLeft(labelWidth));
        row3.removeFromLeft(8);
        localPortEditor.setBounds(row3);

        content.removeFromTop(rowGap);

        auto row4 = content.removeFromTop(rowHeight);
        row4.removeFromLeft(labelWidth);
        row4.removeFromLeft(8);
        initiatorToggleButton.setBounds(row4);
    }

    connectButton.setBounds(bottomButtonArea);
}

void NetworkComponent::refreshFromProcessor()
{
    remoteIpEditor.setText(processorRef.getRemoteIp(), juce::dontSendNotification);
    remotePortEditor.setText(juce::String(processorRef.getRemotePort()), juce::dontSendNotification);
    localPortEditor.setText(juce::String(processorRef.getLocalPort()), juce::dontSendNotification);

    updateConnectButtonColour();
}

void NetworkComponent::setConnectionStatusLabelVisible(bool shouldBeVisible)
{
    connectionLabel.setVisible(shouldBeVisible);
}

void NetworkComponent::timerCallback()
{
    updateConnectButtonColour();
}

void NetworkComponent::commitRemotePeerSettings()
{
    const auto ip = remoteIpEditor.getText().trim();
    const auto remotePort = remotePortEditor.getText().getIntValue();
    const auto localPort = localPortEditor.getText().getIntValue();
    const auto isInitiator = initiatorToggleButton.getToggleState();

    if (ip.isNotEmpty() && remotePort > 0 && localPort > 0)
        processorRef.setRemotePeer(ip, remotePort, localPort, isInitiator);
}

void NetworkComponent::updateConnectButtonColour()
{
    juce::String statusText = "Disconnected";
    juce::Colour statusColour = juce::Colours::lightcoral;

    auto status = processorRef.getSessionStatus();

    switch (status)
    {
    case 0:
        statusText = "Disconnected";
        statusColour = juce::Colours::lightcoral;
        break;
    case 1:
        statusText = "Close Emergency";
        statusColour = juce::Colours::lightpink;
        break;
    case 2:
        statusText = "Inviting...";
        statusColour = juce::Colours::lightblue;
        break;
    case 4:
        statusText = "Listening...";
        statusColour = juce::Colours::lightyellow;
        break;
    case 8:
        statusText = "Connected";
        statusColour = juce::Colours::limegreen;
        break;
    }

    processorRef.getAPVTS().state.setProperty(connectionStatusProperty, status, nullptr);

    connectionLabel.setText(statusText, juce::dontSendNotification);
    connectionLabel.setColour(juce::Label::backgroundColourId, statusColour);

    connectionLabel.repaint();

    discoveredServicesLabel.setText("Discovered Devices [" + juce::String(discoveredPeers.size()) + "]", juce::dontSendNotification);
    discoveredServicesLabel.repaint();
    connectionLabel.repaint();
}

void NetworkComponent::updateDiscoveredServicesCombo()
{
    const auto previouslySelected = discoveredServicesCombo.getText();

    discoveredServicesCombo.clear(juce::dontSendNotification);

    int itemId = 1;
    for (const auto &peer : discoveredPeers)
        discoveredServicesCombo.addItem(makePeerDisplayText(peer), itemId++);

    int restoredIndex = -1;

    for (int i = 0; i < discoveredPeers.size(); ++i)
    {
        if (makePeerDisplayText(discoveredPeers.getReference(i)) == previouslySelected)
        {
            restoredIndex = i;
            break;
        }
    }

    if (restoredIndex >= 0)
        discoveredServicesCombo.setSelectedItemIndex(restoredIndex, juce::dontSendNotification);

    juce::Array<juce::var> items;
    for (int i = 0; i < discoveredServicesCombo.getNumItems(); ++i)
        items.add(discoveredServicesCombo.getItemText(i));

    processorRef.getAPVTS().state.setProperty(discoveredListProperty, juce::var(items), nullptr);
}

void NetworkComponent::clearDiscoveredPeers()
{
    discoveredPeers.clear();
    discoveredServicesCombo.clear(juce::dontSendNotification);
}

void NetworkComponent::addOrUpdateDiscoveredPeer(const DiscoveredPeer &peer)
{
    for (int i = 0; i < discoveredPeers.size(); ++i)
    {
        if (discoveredPeers.getReference(i).serviceName == peer.serviceName)
        {
            discoveredPeers.getReference(i) = peer;
            updateDiscoveredServicesCombo();
            return;
        }
    }

    discoveredPeers.add(peer);
    updateDiscoveredServicesCombo();
}

void NetworkComponent::removeDiscoveredPeerByName(const juce::String &serviceName)
{
    for (int i = discoveredPeers.size(); --i >= 0;)
    {
        if (discoveredPeers.getReference(i).serviceName == serviceName)
            discoveredPeers.remove(i);
    }

    updateDiscoveredServicesCombo();
}

juce::String NetworkComponent::makePeerDisplayText(const DiscoveredPeer &peer)
{
    if (peer.ipAddress.isNotEmpty() && peer.remotePort > 0)
        return peer.serviceName + "  (" + peer.ipAddress + ":" + juce::String(peer.remotePort) + ")";

    if (peer.hostName.isNotEmpty())
        return peer.serviceName + "  (" + peer.hostName + ")";

    return peer.serviceName;
}

void NetworkComponent::startDiscovery()
{
    stopDiscovery();
    clearDiscoveredPeers();

    discoveryService = std::make_unique<jucey::BonjourService>(serviceType);

    auto onServiceDiscovered =
        [this](const jucey::BonjourService &service,
               bool isAvailable,
               bool isMoreComing,
               const juce::Result &result)
    {
        juce::ignoreUnused(isMoreComing);

        if (!result.wasOk())
            return;

        const auto serviceName = service.getName();
        const auto type = service.getType();
        const auto domain = service.getDomain();

        if (!isAvailable)
        {
            juce::MessageManager::callAsync([this, serviceName]
                                            { removeDiscoveredPeerByName(serviceName); });

            return;
        }

        resolveService(serviceName, type, domain);
    };

    discoveryService->discoverAsync(onServiceDiscovered);
}

void NetworkComponent::stopDiscovery()
{
    activeResolvers.clear();
    discoveryService.reset();
}

void NetworkComponent::resolveService(const juce::String &serviceName,
                                      const juce::String &serviceType,
                                      const juce::String &domain)
{
    auto resolver = std::make_unique<jucey::BonjourService>(serviceType, serviceName, domain);

    auto onResolved =
        [this, serviceName](const jucey::BonjourService &service,
                            const juce::String &hostTarget,
                            int rawPort,
                            const juce::Result &result)
    {
        juce::ignoreUnused(service);

        if (!result.wasOk())
            return;

        auto cleanedHost = hostTarget.trim();

        if (cleanedHost.endsWithChar('.'))
            cleanedHost = cleanedHost.dropLastCharacters(1);

        const int resolvedPort = static_cast<int>(ntohs(static_cast<uint16_t>(rawPort)));

        DBG("Resolved service: " + serviceName + " host=" + cleanedHost + " rawPort=" + juce::String(rawPort) + " resolvedPort=" + juce::String(resolvedPort));
        DiscoveredPeer peer;
        peer.serviceName = serviceName;
        peer.hostName = cleanedHost;
        peer.ipAddress = cleanedHost;
        peer.remotePort = resolvedPort; // or 5004 if you want fixed port

        juce::MessageManager::callAsync([this, peer]
                                        { addOrUpdateDiscoveredPeer(peer); });
    };
    auto r = resolver->resolveAsync(onResolved);

    if (r.failed())
    {
        DBG("resolveAsync failed for " + serviceName + ": " + r.getErrorMessage());
        return;
    }

    activeResolvers.push_back(std::move(resolver));
}

void NetworkComponent::valueTreePropertyChanged(juce::ValueTree &tree,
                                                const juce::Identifier &property)
{

    if (tree != processorRef.getAPVTS().state)
        return;

    if (property == juce::Identifier(connectedIndexProperty))
    {
        const int index = (int)processorRef.getAPVTS().state.getProperty(connectedIndexProperty, -1);
        if (juce::isPositiveAndBelow(index, discoveredPeers.size()))
            discoveredServicesCombo.setSelectedItemIndex(index, juce::dontSendNotification);
        discoveredServicesCombo.onChange();
        processorRef.connectToRemotePeer();
    }
    else if (property == juce::Identifier(discoveryStatusProperty))
    {
        const bool isDiscovering = processorRef.getAPVTS().state.getProperty(discoveryStatusProperty, false);
        if (isDiscovering)
        {
            startDiscovery();
        }
        else
        {
            stopDiscovery();
        }
    }
    else if (property == juce::Identifier(remoteIpProperty) || property == juce::Identifier(remotePortProperty) || property == juce::Identifier(localPortProperty) || property == juce::Identifier(initiatorProperty))
    {
        // Update the UI components with the new values
        remoteIpEditor.setText(processorRef.getAPVTS().state.getProperty(remoteIpProperty, "127.0.0.1"), juce::dontSendNotification);
        remotePortEditor.setText(processorRef.getAPVTS().state.getProperty(remotePortProperty, "5004"), juce::dontSendNotification);
        localPortEditor.setText(processorRef.getAPVTS().state.getProperty(localPortProperty, "5000"), juce::dontSendNotification);
        initiatorToggleButton.setToggleState(processorRef.getAPVTS().state.getProperty(initiatorProperty, false), juce::dontSendNotification);
        commitRemotePeerSettings();
        processorRef.connectToRemotePeer();
        updateConnectButtonColour();
    }
}