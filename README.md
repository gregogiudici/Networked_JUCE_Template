# JUCE Network MIDI 2.0 Template

A JUCE template for building **networked audio plugins and applications** with **Network MIDI 2.0** support. It can be used as a starter point for creating your own networked JUCE application, but it is also designed to make it simple to integrate network functionality into an already existing JUCE plugin or standalone project.

This template targets developers working on hard real-time audio software with JUCE who want to add **network-aware MIDI workflows** to a project that already supports local audio and MIDI processing. The main idea is to keep the networking layer modular, reusable, and easy to transplant into another JUCE codebase.

> NOTE: This template requires at least JUCE 8.0.12

## Why Network MIDI 2.0?

[**Network MIDI 2.0**](https://midi.org/network-midi-2-0-udp-overview) is the transport layer used by this template to discover compatible endpoints and exchange MIDI messages over a local network. 

This makes it possible to support workflows such as:

- Sending local MIDI data to a remote endpoint.
- Receiving remote MIDI into a local JUCE application.
- Building hybrid local + remote performance setups.
- Experimenting with distributed MIDI systems built on top of a reusable JUCE codebase.

## HOW TO USE THIS TEMPLATE

### 📁 Repository Structure

```bash
Networked_JUCE_Template/
├── Modules/
│   ├── NetUMP
│   ├── jucey_bonjour
├── Source/
│   ├── PluginProcessor.cpp
│   ├── PluginProcessor.h
│   ├── PluginEditor.cpp
│   ├── PluginEditor.h
│   └── Network/
│       ├── NetUMPThread.h
│       ├── NetworkComponent.cpp
│       └── NetworkComponent.h
├── CMakeLists.txt
└── README.md
```

### 🚀 Quick Start

I suggest you to start building this template and taking confidence with Network MIDI 2.0.

```bash
git clone https://github.com/gregogiudici/Networked_JUCE_Template.git
cd Networked_JUCE_Template
git submodule update --init --recursive
cmake -B build
cmake --build build
```

#### NOTE: Build for Linux

To compile for Linux, make sure you have all the dependencies required to build JUCE.

If you are lazy:

```bash
sudo apt update
# For JUCE
sudo apt install libasound2-dev libjack-jackd2-dev \
    ladspa-sdk \
    libcurl4-openssl-dev  \
    libfreetype-dev libfontconfig1-dev \
    libx11-dev libxcomposite-dev libxcursor-dev libxext-dev libxinerama-dev libxrandr-dev libxrender-dev \
    libwebkit2gtk-4.1-dev \
    libglu1-mesa-dev mesa-common-dev
# For dns-sd
sudo apt-get install libavahi-compat-libdnssd-dev
```

## 1. New project from scratch

If you are beginning a new project, this template can be used as the initial repository for a **networked plugin**, **networked standalone application**, or another JUCE-based musical system that needs network MIDI transport from the beginning.
Typical examples include:

- Networked instruments.
- Distributed performance tools.
- Remote-controllable MIDI applications.
- Hybrid local/remote music systems.
- AI-based music tools that need to exchange MIDI over the network.

A typical workflow for a new project using this template is:

1. Clone or copy this repository.
2. Rename the project and product metadata in `CMakeLists.txt`.
3. Replace the default processor/editor logic with your own code.
4. Keep or adapt the provided networking classes.
5. Extend the MIDI routing and session logic to fit your use case.

This is the most direct use of the repository as a true **template**.

## 2. Extending an already made plugin

This template is also meant to show that it is **straightforward to add network functionality to an already ready plugin**.
In many cases, an existing JUCE project already has:

- stable DSP code,
- a working plugin processor/editor structure,
- parameter management,
- state serialization,
- packaging for Standalone, VST3, and AU.

When this is already in place, a developer often does **not** need to redesign the entire project. Instead, they can reuse the network-related code and integration pattern from this template, add the necessary dependencies, and connect the new network layer to the existing MIDI/event flow.

A typical workflow for extending a project is:

1. Add the required dependencies to your existing project.
2. Copy the network-related source files from the folder Network of this template into your project.
3. Link the new files and libraries in your `CMakeLists.txt`.
4. Instantiate the network session/manager objects in the appropriate application layer.
5. Route incoming and outgoing MIDI/UMP messages to the existing internal architecture.

This template is meant to make that migration path easier and more structured.
You can find some examples in:

- [**Networked_SoundFont_Player**](https://github.com/gregogiudici/Networked_SoundFont_Player)
- [**Networked_AI_Improviser**](https://github.com/gregogiudici/Networked_AI_Improviser)

## Acknowledgements

- [**NetUMP**](https://github.com/gregogiudici/NetUMP/) (forked from [bbouchez](https://github.com/bbouchez/NetUMP/)) for Network MIDI 2.0 communication.
- [**jucey_bonjour**](https://github.com/gregogiudici/jucey_bonjour) (forked from [Anthony-Nicholls](https://github.com/Anthony-Nicholls/jucey_bonjour)) for Network Service Discovery.
- [**JUCE**](https://juce.com/) for audio, MIDI, and cross-platform application infrastructure.
