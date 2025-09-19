#pragma once

#include <JuceHeader.h>

//==============================================================================
class DrumSimulatorAudioProcessor : public juce::AudioProcessor,
    public juce::ValueTree::Listener
{
public:
    //==============================================================================
    DrumSimulatorAudioProcessor();
    ~DrumSimulatorAudioProcessor() override;
    juce::AudioProcessorValueTreeState parameters;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

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
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // Custom methods for drum functionality
    void triggerDrum(int drumIndex, float velocity = 1.0f);
    void loadSample(int drumIndex, const juce::File& file);
    bool isDrumLoaded(int drumIndex) const;
    juce::String getDrumName(int drumIndex) const;

    //==============================================================================
    // ValueTree::Listener
    void valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier&) override {}

    enum DrumSounds
    {
        KICK = 0,
        SNARE,
        HIHAT,
        CRASH,
        TOM1,
        TOM2,
        TOM3,
        RIDE,
        NUM_SOUNDS
    };

private:
    //==============================================================================
    struct DrumVoice
    {
        std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
        std::unique_ptr<juce::ResamplingAudioSource> resamplingSource;
        juce::AudioBuffer<float> sampleBuffer;
        int currentSampleIndex = 0;
        bool isPlaying = false;
        float velocity = 1.0f;
        float gain = 1.0f;
        juce::String name;

        void trigger(float vel)
        {
            velocity = vel;
            currentSampleIndex = 0;
            isPlaying = true;
        }

        void stop()
        {
            isPlaying = false;
            currentSampleIndex = 0;
        }

        bool hasValidSample() const
        {
            return sampleBuffer.getNumSamples() > 0;
        }
    };

    //==============================================================================
    void processDrumVoices(juce::AudioBuffer<float>& buffer);
    void processMidiEvents(const juce::MidiBuffer& midiMessages);
    void setupDrumNames();

    //==============================================================================
    std::array<DrumVoice, NUM_SOUNDS> drumVoices;
    juce::AudioFormatManager formatManager;

    // MIDI note mappings for drum sounds
    std::array<int, NUM_SOUNDS> midiNoteMapping = {
        36, // KICK (C1)
        38, // SNARE (D1)
        42, // HIHAT (F#1)
        49, // CRASH (C#2)
        45, // TOM1 (A1)
        47, // TOM2 (B1)
        48, // TOM3 (C2)
        51  // RIDE (D#2)
    };

    // ***** ADD YOUR SAMPLE PATHS HERE *****
    // Replace these empty strings with paths to your drum samples
    std::array<juce::String, NUM_SOUNDS> samplePaths = {
        "E:\\JUCE\\Programs\\XZ Beats\\SnareSamples\\Snare 5.wav", // KICK - Add path to kick drum sample here (e.g., "C:/Samples/kick.wav")
        "E:\\JUCE\\Programs\\XZ Beats\\SnareSamples\\Snare 5.wav", // SNARE - Add path to snare drum sample here
        "E:\\JUCE\\Programs\\XZ Beats\\SnareSamples\\Snare 5.wav", // HIHAT - Add path to hi-hat sample here
        "E:\\JUCE\\Programs\\XZ Beats\\SnareSamples\\Snare 5.wav", // CRASH - Add path to crash cymbal sample here
        "E:\\JUCE\\Programs\\XZ Beats\\SnareSamples\\Snare 5.wav", // TOM1 - Add path to tom 1 sample here
        "E:\\JUCE\\Programs\\XZ Beats\\SnareSamples\\Snare 5.wav", // TOM2 - Add path to tom 2 sample here
        "E:\\JUCE\\Programs\\XZ Beats\\SnareSamples\\Snare 5.wav", // TOM3 - Add path to tom 3 sample here
        "E:\\JUCE\\Programs\\XZ Beats\\SnareSamples\\Snare 5.wav"  // RIDE - Add path to ride cymbal sample here
    };

    // Parameters
 
    std::array<juce::AudioParameterFloat*, NUM_SOUNDS> gainParameters;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumSimulatorAudioProcessor)
};