#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class DrumSimulatorAudioProcessorEditor : public juce::AudioProcessorEditor,
    public juce::KeyListener,
    public juce::Timer,
    public juce::Button::Listener,
    public juce::Slider::Listener
{
public:
    DrumSimulatorAudioProcessorEditor(DrumSimulatorAudioProcessor&);
    ~DrumSimulatorAudioProcessorEditor() override;

    //==============================================================================
    void paint(juce::Graphics&) override;
    void resized() override;

    //==============================================================================
    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

    //==============================================================================
    void timerCallback() override;

    //==============================================================================
    void buttonClicked(juce::Button* button) override;
    void sliderValueChanged(juce::Slider* slider) override;

private:
    //==============================================================================
    struct DrumPad
    {
        std::unique_ptr<juce::TextButton> button;
        std::unique_ptr<juce::Slider> gainSlider;
        std::unique_ptr<juce::Label> gainLabel;
        std::unique_ptr<juce::TextButton> loadButton;
        juce::Rectangle<int> bounds;
        juce::Colour padColor;
        juce::String name;
        juce::KeyPress keyBinding;
        bool isPressed = false;
        int drumIndex;

        DrumPad() = default;
        ~DrumPad() = default;

        // Delete copy operations
        DrumPad(const DrumPad&) = delete;
        DrumPad& operator=(const DrumPad&) = delete;

        // Enable move operations
        DrumPad(DrumPad&&) = default;
        DrumPad& operator=(DrumPad&&) = default;
    };

    //==============================================================================
    void setupDrumPads();
    void setupDrumPad(int index, const juce::String& name, juce::Colour color, juce::KeyPress key);
    void drawDrumPad(juce::Graphics& g, const DrumPad& pad);
    void triggerDrumPad(int index);
    void loadSampleForDrum(int drumIndex);

    //==============================================================================
    DrumSimulatorAudioProcessor& audioProcessor;

    std::array<DrumPad, DrumSimulatorAudioProcessor::NUM_SOUNDS> drumPads;

    // UI Components
    std::unique_ptr<juce::Label> titleLabel;
    std::unique_ptr<juce::Label> instructionsLabel;
    std::unique_ptr<juce::GroupComponent> drumPadsGroup;
    std::unique_ptr<juce::GroupComponent> controlsGroup;

    // Parameter attachments
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::array<std::unique_ptr<SliderAttachment>, DrumSimulatorAudioProcessor::NUM_SOUNDS> gainAttachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DrumSimulatorAudioProcessorEditor)
};