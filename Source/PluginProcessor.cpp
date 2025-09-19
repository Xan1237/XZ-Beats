#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
DrumSimulatorAudioProcessor::DrumSimulatorAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ),
#endif
    parameters(*this, nullptr, juce::Identifier("DrumSimulator"),
        {
            std::make_unique<juce::AudioParameterFloat>("kick_gain", "Kick Gain", 0.0f, 2.0f, 1.0f),
            std::make_unique<juce::AudioParameterFloat>("snare_gain", "Snare Gain", 0.0f, 2.0f, 1.0f),
            std::make_unique<juce::AudioParameterFloat>("hihat_gain", "Hi-Hat Gain", 0.0f, 2.0f, 1.0f),
            std::make_unique<juce::AudioParameterFloat>("crash_gain", "Crash Gain", 0.0f, 2.0f, 1.0f),
            std::make_unique<juce::AudioParameterFloat>("tom1_gain", "Tom 1 Gain", 0.0f, 2.0f, 1.0f),
            std::make_unique<juce::AudioParameterFloat>("tom2_gain", "Tom 2 Gain", 0.0f, 2.0f, 1.0f),
            std::make_unique<juce::AudioParameterFloat>("tom3_gain", "Tom 3 Gain", 0.0f, 2.0f, 1.0f),
            std::make_unique<juce::AudioParameterFloat>("ride_gain", "Ride Gain", 0.0f, 2.0f, 1.0f)
        })
{
    // Register basic audio formats
    formatManager.registerBasicFormats();

    // Setup drum names
    setupDrumNames();

    // Get parameter pointers
    gainParameters[KICK] = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("kick_gain"));
    gainParameters[SNARE] = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("snare_gain"));
    gainParameters[HIHAT] = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("hihat_gain"));
    gainParameters[CRASH] = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("crash_gain"));
    gainParameters[TOM1] = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("tom1_gain"));
    gainParameters[TOM2] = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("tom2_gain"));
    gainParameters[TOM3] = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("tom3_gain"));
    gainParameters[RIDE] = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("ride_gain"));

    // Load samples if paths are specified
    for (int i = 0; i < NUM_SOUNDS; ++i)
    {
        if (samplePaths[i].isNotEmpty())
        {
            juce::File sampleFile(samplePaths[i]);
            if (sampleFile.existsAsFile())
                loadSample(i, sampleFile);
        }
    }
}

DrumSimulatorAudioProcessor::~DrumSimulatorAudioProcessor()
{
}

//==============================================================================
const juce::String DrumSimulatorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool DrumSimulatorAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool DrumSimulatorAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool DrumSimulatorAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double DrumSimulatorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DrumSimulatorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
    // so this should be at least 1, even if you're not really implementing programs.
}

int DrumSimulatorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void DrumSimulatorAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String DrumSimulatorAudioProcessor::getProgramName(int index)
{
    return {};
}

void DrumSimulatorAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

//==============================================================================
void DrumSimulatorAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Prepare drum voices
    for (auto& voice : drumVoices)
    {
        if (voice.resamplingSource)
            voice.resamplingSource->prepareToPlay(samplesPerBlock, sampleRate);
    }
}

void DrumSimulatorAudioProcessor::releaseResources()
{
    // Release resources
    for (auto& voice : drumVoices)
    {
        if (voice.resamplingSource)
            voice.resamplingSource->releaseResources();
    }
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool DrumSimulatorAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
    juce::ignoreUnused(layouts);
    return true;
#else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
#endif

    return true;
#endif
}
#endif

void DrumSimulatorAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear any output channels that don't contain input data
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Process MIDI events
    processMidiEvents(midiMessages);

    // Process drum voices
    processDrumVoices(buffer);
}

//==============================================================================
bool DrumSimulatorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* DrumSimulatorAudioProcessor::createEditor()
{
    return new DrumSimulatorAudioProcessorEditor(*this);
}

//==============================================================================
void DrumSimulatorAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DrumSimulatorAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
void DrumSimulatorAudioProcessor::triggerDrum(int drumIndex, float velocity)
{
    if (drumIndex >= 0 && drumIndex < NUM_SOUNDS)
    {
        auto& voice = drumVoices[drumIndex];
        if (voice.hasValidSample())
        {
            voice.trigger(velocity);
            DBG("Triggered drum: " + voice.name + " with velocity: " + juce::String(velocity));
        }
        else
        {
            DBG("No sample loaded for drum: " + voice.name);
        }
    }
}

void DrumSimulatorAudioProcessor::loadSample(int drumIndex, const juce::File& file)
{
    if (drumIndex < 0 || drumIndex >= NUM_SOUNDS)
        return;

    auto& voice = drumVoices[drumIndex];

    auto* reader = formatManager.createReaderFor(file);

    if (reader != nullptr)
    {
        // Load the entire sample into memory
        voice.sampleBuffer.setSize((int)reader->numChannels, (int)reader->lengthInSamples);
        reader->read(&voice.sampleBuffer, 0, (int)reader->lengthInSamples, 0, true, true);

        // Create audio sources for real-time playback if needed
        voice.readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        voice.resamplingSource = std::make_unique<juce::ResamplingAudioSource>(voice.readerSource.get(), false);

        DBG("Loaded sample: " + file.getFileName() + " for drum " + voice.name);
    }
    else
    {
        DBG("Failed to load sample: " + file.getFullPathName());
    }
}

bool DrumSimulatorAudioProcessor::isDrumLoaded(int drumIndex) const
{
    if (drumIndex >= 0 && drumIndex < NUM_SOUNDS)
        return drumVoices[drumIndex].hasValidSample();
    return false;
}

juce::String DrumSimulatorAudioProcessor::getDrumName(int drumIndex) const
{
    if (drumIndex >= 0 && drumIndex < NUM_SOUNDS)
        return drumVoices[drumIndex].name;
    return {};
}

//==============================================================================
void DrumSimulatorAudioProcessor::processDrumVoices(juce::AudioBuffer<float>& buffer)
{
    auto numSamples = buffer.getNumSamples();
    auto numChannels = buffer.getNumChannels();

    for (int drumIndex = 0; drumIndex < NUM_SOUNDS; ++drumIndex)
    {
        auto& voice = drumVoices[drumIndex];

        if (!voice.isPlaying || !voice.hasValidSample())
            continue;

        auto gain = gainParameters[drumIndex] ? gainParameters[drumIndex]->get() : 1.0f;
        auto effectiveGain = gain * voice.velocity * voice.gain;

        auto& sampleBuffer = voice.sampleBuffer;
        auto sampleChannels = sampleBuffer.getNumChannels();
        auto sampleLength = sampleBuffer.getNumSamples();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            if (voice.currentSampleIndex >= sampleLength)
            {
                voice.stop();
                break;
            }

            for (int channel = 0; channel < numChannels; ++channel)
            {
                auto sampleChannel = juce::jmin(channel, sampleChannels - 1);
                auto sampleValue = sampleBuffer.getSample(sampleChannel, voice.currentSampleIndex) * effectiveGain;
                buffer.addSample(channel, sample, sampleValue);
            }

            voice.currentSampleIndex++;
        }
    }
}

void DrumSimulatorAudioProcessor::processMidiEvents(const juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();

        if (message.isNoteOn())
        {
            int midiNote = message.getNoteNumber();
            float velocity = message.getFloatVelocity();

            // Find which drum corresponds to this MIDI note
            for (int i = 0; i < NUM_SOUNDS; ++i)
            {
                if (midiNoteMapping[i] == midiNote)
                {
                    triggerDrum(i, velocity);
                    break;
                }
            }
        }
    }
}

void DrumSimulatorAudioProcessor::setupDrumNames()
{
    drumVoices[KICK].name = "KICK";
    drumVoices[SNARE].name = "SNARE";
    drumVoices[HIHAT].name = "HI-HAT";
    drumVoices[CRASH].name = "CRASH";
    drumVoices[TOM1].name = "TOM 1";
    drumVoices[TOM2].name = "TOM 2";
    drumVoices[TOM3].name = "TOM 3";
    drumVoices[RIDE].name = "RIDE";
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DrumSimulatorAudioProcessor();
}