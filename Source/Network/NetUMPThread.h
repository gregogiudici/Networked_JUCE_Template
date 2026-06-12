#pragma once

#include <juce_core/juce_core.h>
#include <NetUMP/NetUMP.h>

/** Returns the Message Type nibble (bits 31-28) of the first UMP word. */
static inline uint8_t umpMessageType(uint32_t w0) { return static_cast<uint8_t>((w0 >> 28) & 0x0F); }

/** Returns the MIDI group nibble (bits 27-24) of the first UMP word. */
static inline uint8_t umpGroup(uint32_t w0) { return static_cast<uint8_t>((w0 >> 24) & 0x0F); }

/** Returns the status byte for MT=2 (MIDI 1.0 channel voice) messages. */
static inline uint8_t umpMidi1Status(uint32_t w0) { return static_cast<uint8_t>((w0 >> 16) & 0xFF); }

/** Returns data byte 1 for MT=2 messages. */
static inline uint8_t umpMidi1Data1(uint32_t w0) { return static_cast<uint8_t>((w0 >> 8) & 0xFF); }

/** Returns data byte 2 for MT=2 messages. */
static inline uint8_t umpMidi1Data2(uint32_t w0) { return static_cast<uint8_t>(w0 & 0xFF); }

class NetUMPThread final : public juce::Thread
{
public:
    //==========================================================================
    /** @param collector   The MidiMessageCollector owned by your AudioProcessor.
        @param threadName  Optional name shown in profilers / debuggers.       */
    explicit NetUMPThread(juce::MidiMessageCollector &collector,
                          const juce::String &threadName = "NetUMP Thread")
        : juce::Thread(threadName), midiCollector(collector), handler(&NetUMPThread::staticUMPCallback, this)
    {
        handler.SetConnectionCallback(
            [](const char *name, unsigned int /*size*/)
            {
                // DBG("NetUMP connected: " << juce::String(name));
            });

        handler.SetDisconnectCallback(
            []()
            {
                // DBG("NetUMP disconnected.");
            });
    }

    ~NetUMPThread() override
    {
        stopSession();
    }

    /** Send MIDI messages to the network. */
    void sendMidiMessage(juce::ump::View packet)
    {
        handler.SendUMPMessage(packet.data());
    }

    juce::String getRemoteIp() const
    {
        juce::IPAddress ip(cfg.destIP);
        return ip.toString();
    }

    int getRemotePort() const
    {
        return cfg.destPort;
    }

    int getLocalPort() const
    {
        return cfg.localPort;
    }

    /** Set the local endpoint name advertised to remote peers. */
    void setEndpointName(const juce::String &name)
    {
        // CNetUMPHandler takes a non-const char* – copy into a local buffer
        juce::CharPointer_UTF8 utf8 = name.toUTF8();
        std::strncpy(endpointNameBuf, utf8, MAX_UMP_ENDPOINT_NAME_LEN - 1);
        endpointNameBuf[MAX_UMP_ENDPOINT_NAME_LEN - 1] = '\0';
        handler.SetEndpointName(endpointNameBuf);
    }

    /** Set the Product Instance ID (optional). */
    void setProductInstanceID(const juce::String &piid)
    {
        juce::CharPointer_UTF8 utf8 = piid.toUTF8();
        std::strncpy(piidBuf, utf8, MAX_UMP_PRODUCT_INSTANCE_ID_LEN - 1);
        piidBuf[MAX_UMP_PRODUCT_INSTANCE_ID_LEN - 1] = '\0';
        handler.SetProductInstanceID(piidBuf);
    }

    /** Store connection parameters. Call before startThread().
        @param destIP      Remote peer IP
        @param destPort    Remote peer UDP port.
        @param localPort   Local UDP port to bind.
        @param isInitiator true = this side sends the invitation; false = listener.  */
    void configure(juce::String destIP,
                   unsigned short destPort,
                   unsigned short localPort,
                   bool isInitiator)
    {
        juce::IPAddress host(destIP);

        // Convert to unsigned int (packed 32-bit, MSB first)
        unsigned int asUint =
            (static_cast<unsigned int>(host.address[0]) << 24) | // 127
            (static_cast<unsigned int>(host.address[1]) << 16) | // 0
            (static_cast<unsigned int>(host.address[2]) << 8) |  // 0
            (static_cast<unsigned int>(host.address[3]));        // 1

        DBG("destIP " << destIP << " as unsigned int: " << juce::String(asUint));

        // Reverse: unsigned int back to IPAddress
        juce::IPAddress fromUint(asUint);
        cfg.destIP = asUint;
        cfg.destPort = destPort;
        cfg.localPort = localPort;
        cfg.isInitiator = isInitiator;

        DBG(juce::String(cfg.destIP));
        DBG(juce::String(cfg.destPort));
        DBG(juce::String(cfg.localPort));
    }

    /** Enable / disable Forward Error Correction on the transmit side.
        Must be called before (or after, if session is not active) startThread(). */
    void setErrorCorrectionMode(unsigned int mode) // ERROR_CORRECTION_NONE / FEC
    {
        handler.SelectErrorCorrectionMode(mode);
    }

    /** Opens the UDP socket and starts the network thread.
        @return false if the UDP socket could not be created.                  */
    bool startSession()
    {
        int result = handler.InitiateSession(cfg.destIP,
                                             cfg.destPort,
                                             cfg.localPort,
                                             cfg.isInitiator);
        if (result != 0)
        {
            DBG("NetUMPThread: InitiateSession failed (result=" << result << ")");
            return false;
        }

        startThread(juce::Thread::Priority::high);
        return true;
    }

    /** Closes the session and stops the thread.  Safe to call from any thread. */
    void stopSession()
    {
        if (isThreadRunning())
        {
            signalThreadShouldExit();
            stopThread(200 /*ms timeout*/);
        }
        handler.CloseSession();
    }

    /** 	
    * SESSION_CLOSED			0	// No action
	* SESSION_CLOSE			    1	// Session should close in emergency
	* SESSION_INVITE			2	// Sending invitation to remote partner
	* SESSION_WAIT_INVITE		4	// Wait to be invited by remote station
	* SESSION_OPENED			8	// Session is opened, just generate background traffic now
     */
    int getSessionStatus() { return handler.GetSessionStatus(); }

    bool isSessionOpen() { return getSessionStatus() == 8; }

    /** Returns true (once) when the connection drops unexpectedly. */
    bool readAndResetConnectionLost() { return handler.ReadAndResetConnectionLost(); }

    /** Returns true (once) when the remote peer sends BYE. */
    bool remotePeerClosedSession() { return handler.RemotePeerClosedSession(); }

private:
    void run() override
    {
        using namespace std::chrono;

        constexpr int INTERVAL_US = 1000; // 1 ms

        while (!threadShouldExit())
        {
            auto t0 = high_resolution_clock::now();

            handler.RunSession();

            auto elapsed = duration_cast<microseconds>(high_resolution_clock::now() - t0).count();
            int remaining = INTERVAL_US - static_cast<int>(elapsed);

            if (remaining > 100)
                wait(remaining / 1000);
            // If we're already past the deadline, loop immediately
        }
    }

    //==========================================================================
    // UMP receive callback (called from the run() thread context)
    //==========================================================================

#ifdef __TARGET_WIN__
    static void CALLBACK staticUMPCallback(void *instance, uint32_t *dataBlock)
#else
    static void staticUMPCallback(void *instance, uint32_t *dataBlock)
#endif
    {
        static_cast<NetUMPThread *>(instance)->handleIncomingUMP(dataBlock);
    }

    /** Decode the first UMP word and push a juce::MidiMessage to the collector.              */
    void handleIncomingUMP(uint32_t *dataBlock)
    {
        const uint32_t w0 = dataBlock[0];
        const uint8_t mt = umpMessageType(w0);

        // MT = 0x02 : MIDI 1.0 Channel Voice Message (single word)
        if (mt == 0x02)
        {
            DBG("MIDI 1.0 Channel Voice Message: " << (int)umpMidi1Status(w0));
            const uint8_t status = umpMidi1Status(w0);
            const uint8_t d1 = umpMidi1Data1(w0);
            const uint8_t d2 = umpMidi1Data2(w0);

            juce::MidiMessage msg(status, d1, d2, juce::Time::getMillisecondCounterHiRes());
            midiCollector.addMessageToQueue(msg);
            return;
        }

        // MT = 0x01 : System Common / System Real-Time (single word)
        if (mt == 0x01)
        {
            const uint8_t status = umpMidi1Status(w0);
            juce::MidiMessage msg(status, juce::Time::getMillisecondCounterHiRes());
            midiCollector.addMessageToQueue(msg);
            return;
        }

        // MT = 0x04 : MIDI 2.0 Channel Voice (two words) – basic translation
        if (mt == 0x04)
        {
            handleMidi2CVMessage(w0, dataBlock[1]);
            return;
        }
    }

    /** Minimal MIDI 2.0 → MIDI 1.0 downscale for the most common messages.
        This keeps compatibility with hosts expecting MIDI 1.0.               */
    void handleMidi2CVMessage(uint32_t w0, uint32_t w1)
    {
        const uint8_t opcode = static_cast<uint8_t>((w0 >> 20) & 0x0F);
        const uint8_t channel = static_cast<uint8_t>((w0 >> 16) & 0x0F);

        switch (opcode)
        {
        case 0x09: // Note On
        {
            const uint8_t note = static_cast<uint8_t>((w0 >> 8) & 0x7F);
            // MIDI 2.0 velocity is 16-bit in w1[31:16]; scale to 7-bit
            const uint8_t velocity = static_cast<uint8_t>((w1 >> 25) & 0x7F);
            midiCollector.addMessageToQueue(
                juce::MidiMessage::noteOn(channel + 1, note, velocity));
            break;
        }
        case 0x08: // Note Off
        {
            const uint8_t note = static_cast<uint8_t>((w0 >> 8) & 0x7F);
            const uint8_t velocity = static_cast<uint8_t>((w1 >> 25) & 0x7F);
            midiCollector.addMessageToQueue(
                juce::MidiMessage::noteOff(channel + 1, note, velocity));
            break;
        }
        case 0x0B: // Control Change
        {
            const uint8_t cc = static_cast<uint8_t>((w0 >> 8) & 0x7F);
            // MIDI 2.0 CC value is 32-bit; scale to 7-bit
            const uint8_t value = static_cast<uint8_t>(w1 >> 25);
            midiCollector.addMessageToQueue(
                juce::MidiMessage::controllerEvent(channel + 1, cc, value));
            break;
        }
        case 0x0C: // Program Change
        {
            const uint8_t program = static_cast<uint8_t>((w1 >> 24) & 0x7F);
            midiCollector.addMessageToQueue(
                juce::MidiMessage::programChange(channel + 1, program));
            break;
        }
        case 0x0D: // Channel Pressure
        {
            const uint8_t pressure = static_cast<uint8_t>(w1 >> 25);
            midiCollector.addMessageToQueue(
                juce::MidiMessage::channelPressureChange(channel + 1, pressure));
            break;
        }
        case 0x0E: // Pitch Bend
        {
            // MIDI 2.0 pitch bend is 32-bit unsigned; map to 14-bit MIDI 1.0
            const int pb14 = static_cast<int>(w1 >> 18) & 0x3FFF;
            midiCollector.addMessageToQueue(
                juce::MidiMessage::pitchWheel(channel + 1, pb14));
            break;
        }
        default:
            break; // Poly pressure, RPN, etc.
        }
    }

    struct Config
    {
        unsigned int destIP = 2130706433; // 127.0.0.1
        unsigned short destPort = 5001;
        unsigned short localPort = 5000;
        bool isInitiator = true;
    };

    juce::MidiMessageCollector &midiCollector;
    CNetUMPHandler handler;
    Config cfg;

    // Mutable name buffers (CNetUMPHandler takes non-const char*)s
    char endpointNameBuf[MAX_UMP_ENDPOINT_NAME_LEN] = {};
    char piidBuf[MAX_UMP_PRODUCT_INSTANCE_ID_LEN] = {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NetUMPThread)
};