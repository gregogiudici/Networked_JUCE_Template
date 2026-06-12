# JUCE Network MIDI 2.0 Template

# This template is intended for developers who want to extend an existing JUCE plugin or
# standalone audio application with Network MIDI 2.0 functionality using NetUMP, while
# preserving hard real-time audio constraints.

cmake_minimum_required(VERSION 3.22)

project(JUCE_NETWORK_MIDI2_TEMPLATE VERSION 0.0.1)

# IMPORTANT: If you've installed JUCE somehow (via a package manager, or directly using the CMake install
# target), you'll need to tell this project that it depends on the installed copy of JUCE. 
# find_package(JUCE CONFIG REQUIRED)        # If you've installed JUCE to your system
# or
# add_subdirectory(JUCE)

# NetUMP library for Network MIDI 2.0
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Modules/NetUMP)
# jucey_bonjour module for mDNS/Bonjour
juce_add_module(${CMAKE_CURRENT_SOURCE_DIR}/Modules/jucey_bonjour/jucey_bonjour)


# `juce_add_plugin` adds a static library target with the name passed as the first argument
# (`JuceNetworkMidi2Template` here). This target is a normal CMake target, but has a lot of
# extra properties set up by default. As well as this shared code static library, this function
# adds targets for each of the formats specified by the FORMATS arguments. This function accepts
# many optional arguments. Check the readme at `docs/CMake API.md` in the JUCE repo for the full list.

juce_add_plugin(JuceNetworkMidi2Template
    # VERSION ...                               # Set this if the plugin version is different to the project version
    # ICON_BIG ...                              # ICON_* arguments specify a path to an image file to use as an icon for the Standalone
    # ICON_SMALL ...
    # COMPANY_NAME ...                          # Specify the name of the plugin's author
    IS_SYNTH TRUE                               
    NEEDS_MIDI_INPUT TRUE              
    NEEDS_MIDI_OUTPUT TRUE              
    # IS_MIDI_EFFECT TRUE/FALSE                 # Is this plugin a MIDI effect?
    # EDITOR_WANTS_KEYBOARD_FOCUS TRUE/FALSE    # Does the editor need keyboard focus?
    # COPY_PLUGIN_AFTER_BUILD TRUE/FALSE        # Should the plugin be installed to a default location after building?
    PLUGIN_MANUFACTURER_CODE Greg
    PLUGIN_CODE Net0
    FORMATS AU VST3 Standalone
    PRODUCT_NAME "JUCE Network MIDI 2.0 Template")


# `juce_generate_juce_header` will create a JuceHeader.h for a given target, which will be generated
# into your build tree. This should be included with `#include <JuceHeader.h>`. The include path for
# this header will be automatically added to the target. The main function of the JuceHeader is to
# include all your JUCE module headers; if you're happy to include module headers directly, you
# probably don't need to call this.

# juce_generate_juce_header(JuceNetworkMidi2Template)


# `target_sources` adds source files to a target. We pass the target that needs the sources as the
# first argument, then a visibility parameter for the sources which should normally be PRIVATE.
# Finally, we supply a list of source files that will be built into the target. This is a standard
# CMake command.

target_sources(JuceNetworkMidi2Template
    PRIVATE
        Source/PluginEditor.cpp
        Source/PluginProcessor.cpp
        # Network MIDI 2.0 and mDNS/Bonjour
        Source/Network/NetworkComponent.cpp
)


# `target_compile_definitions` adds some preprocessor definitions to our target. In a Projucer
# project, these might be passed in the 'Preprocessor Definitions' field. JUCE modules also make use
# of compile definitions to switch certain features on/off, so if there's a particular feature you
# need that's not on by default, check the module header for the correct flag to set here. These
# definitions will be visible both to your code, and also the JUCE module code, so for new
# definitions, pick unique names that are unlikely to collide! This is a standard CMake command.

target_compile_definitions(JuceNetworkMidi2Template
    PUBLIC
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_plugin` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_plugin` call
        JUCE_VST3_CAN_REPLACE_VST2=0

        # Template-specific flags for optional network MIDI 2.0 integration points
        NETMIDI2_TEMPLATE=1
)


# If your target needs extra binary assets, you can add them here. The first argument is the name of
# a new static library target that will include all the binary resources. There is an optional
# `NAMESPACE` argument that can specify the namespace of the generated binary data class. Finally,
# the SOURCES argument should be followed by a list of source files that should be built into the
# static library. These source files can be of any kind (wav data, images, fonts, icons etc.).
# Conversion to binary-data will happen when your target is built.

# juce_add_binary_data(BinaryData SOURCES ...)


# `target_link_libraries` links libraries and JUCE modules to other libraries or executables. Here,
# we're linking our executable target to the JUCE modules required by the template. 
# Inter-module dependencies are resolved automatically, so `juce_core`, `juce_events` and so on
# will also be linked automatically. If we'd generated a binary data target above, we would need
# to link to it here too. This is a standard CMake command.

# For mDNS/Bonjour support
if(UNIX AND NOT APPLE)
    target_link_libraries(JuceNetworkMidi2Template PRIVATE dns_sd)
endif()

target_link_libraries(JuceNetworkMidi2Template
    PRIVATE
        # AudioPluginData           # If we'd created a binary data target, we'd link to it here
        juce::juce_audio_utils
        NetUMP
        jucey_bonjour
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)