#include "PluginProcessor.h"
#include "PluginEditor.h"

class BorderlessButtonLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground(juce::Graphics& g,
        juce::Button& button,
        const juce::Colour& backgroundColour,
        bool isMouseOverButton,
        bool isButtonDown) override
    {
        // Only fill with background colour, no border
        g.setColour(backgroundColour);
        g.fillRoundedRectangle(button.getLocalBounds().toFloat(), 5.0f);
    }
};
//==============================================================================
DrumSimulatorAudioProcessorEditor::DrumSimulatorAudioProcessorEditor(DrumSimulatorAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    // Setup UI components
    titleLabel = std::make_unique<juce::Label>("title", "DRUM SIMULATOR");
    titleLabel->setFont(juce::Font(24.0f, juce::Font::bold));
    titleLabel->setJustificationType(juce::Justification::centred);
    titleLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    addAndMakeVisible(*titleLabel);

    instructionsLabel = std::make_unique<juce::Label>("instructions",
        "Use keys: Q(Kick) W(Snare) E(HiHat) R(Crash) A(Tom1) S(Tom2) D(Tom3) F(Ride) | Click pads to trigger | Load buttons to change samples");
    instructionsLabel->setFont(juce::Font(12.0f));
    instructionsLabel->setJustificationType(juce::Justification::centred);
    instructionsLabel->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    addAndMakeVisible(*instructionsLabel);

  
   

    controlsGroup = std::make_unique<juce::GroupComponent>("controls", "Controls");
    controlsGroup->setColour(juce::GroupComponent::textColourId, juce::Colours::white);
    addAndMakeVisible(*controlsGroup);

    // Setup drum pads
    setupDrumPads();

    // Make this component a key listener
    setWantsKeyboardFocus(true);
    addKeyListener(this);

    // Start timer for visual feedback
    startTimerHz(30);

    // Set size
    setSize(900, 700);
    // Load drum kit PNG from the project folder
    auto drumKitFile = juce::File("E:\\JUCE\\Programs\\XZ Beats\\drumspic\\drumset.jpg");


    if (drumKitFile.existsAsFile())
    {
        drumKitImage = juce::ImageFileFormat::loadFrom(drumKitFile);
    }
    else
    {
        juce::Logger::writeToLog("Drum kit image not found!");
    }
}

DrumSimulatorAudioProcessorEditor::~DrumSimulatorAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

//==============================================================================
void DrumSimulatorAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Background gradient
    auto bounds = getLocalBounds();
    juce::ColourGradient gradient(juce::Colour(0xff2a2a2a), 0, 0,
        juce::Colour(0xff1a1a1a), 0, (float)bounds.getHeight(), false);
    g.setGradientFill(gradient);
    g.fillAll();

    // Draw drum pads
    for (const auto& pad : drumPads)
    {
        drawDrumPad(g, pad);
    }
     g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));

    if (drumKitImage.isValid())
    {
        g.drawImage(drumKitImage, getLocalBounds().toFloat());
    }

    g.setColour(juce::Colours::white);
    g.setFont(15.0f);
    g.drawFittedText("Drum Tester", getLocalBounds(), juce::Justification::centredTop, 1);
}



void DrumSimulatorAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto margin = 10;

    // Title
    titleLabel->setBounds(bounds.removeFromTop(40).reduced(margin));

    // Instructions at bottom
    instructionsLabel->setBounds(bounds.removeFromBottom(30).reduced(margin));

    // Split remaining area
    auto drumPadsArea = bounds.removeFromTop(bounds.getHeight() * 0.65f).reduced(margin);
    auto controlsArea = bounds.reduced(margin);

    // Set group bounds
  

    // Layout drum pads in 2x4 grid
    auto padArea = drumPadsArea.reduced(20);
    int padWidth = padArea.getWidth() / 4 - 10;
    int padHeight = (padArea.getHeight() - 20) / 2 - 10;

    // Top row
    drumPads[DrumSimulatorAudioProcessor::KICK].bounds = juce::Rectangle<int>(390,350, 240, 300);
    drumPads[DrumSimulatorAudioProcessor::SNARE].bounds = juce::Rectangle<int>(300, 300, 120, 120);
    drumPads[DrumSimulatorAudioProcessor::HIHAT].bounds = juce::Rectangle<int>(500, 250, 100, 100);
    drumPads[DrumSimulatorAudioProcessor::CRASH].bounds = juce::Rectangle<int>(700, 200, 120, 120);

    drumPads[DrumSimulatorAudioProcessor::TOM1].bounds = juce::Rectangle<int>(250, 200, 100, 100);
    drumPads[DrumSimulatorAudioProcessor::TOM2].bounds = juce::Rectangle<int>(400, 180, 100, 100);
    drumPads[DrumSimulatorAudioProcessor::TOM3].bounds = juce::Rectangle<int>(550, 160, 100, 100);
    drumPads[DrumSimulatorAudioProcessor::RIDE].bounds = juce::Rectangle<int>(650, 350, 120, 120);

    // Layout drum pad buttons
    for (auto& pad : drumPads)
    {
        if (pad.button)
        {
            auto buttonArea = pad.bounds.reduced(5);
            pad.button->setBounds(buttonArea.removeFromTop(buttonArea.getHeight() * 0.7f));
        }
    }

    // Layout controls in the controls area
    auto controlsInnerArea = controlsArea.reduced(20);
    int sliderWidth = controlsInnerArea.getWidth() / 8 - 5;

    for (int i = 0; i < DrumSimulatorAudioProcessor::NUM_SOUNDS; ++i)
    {
        auto& pad = drumPads[i];
        int x = controlsInnerArea.getX() + i * (sliderWidth + 5);

        if (pad.gainLabel)
            pad.gainLabel->setBounds(x, controlsInnerArea.getY(), sliderWidth, 20);
        if (pad.gainSlider)
            pad.gainSlider->setBounds(x, controlsInnerArea.getY() + 25, sliderWidth, 80);
        if (pad.loadButton)
            pad.loadButton->setBounds(x, controlsInnerArea.getY() + 110, sliderWidth, 30);
    }
}

//==============================================================================
void DrumSimulatorAudioProcessorEditor::setupDrumPads()
{
    setupDrumPad(DrumSimulatorAudioProcessor::KICK, "", juce::Colours::transparentWhite, juce::KeyPress('q'));
    setupDrumPad(DrumSimulatorAudioProcessor::SNARE, "", juce::Colours::orange, juce::KeyPress('w'));
    setupDrumPad(DrumSimulatorAudioProcessor::HIHAT, "", juce::Colours::yellow, juce::KeyPress('e'));
    setupDrumPad(DrumSimulatorAudioProcessor::CRASH, "", juce::Colours::green, juce::KeyPress('r'));
    setupDrumPad(DrumSimulatorAudioProcessor::TOM1, "", juce::Colours::blue, juce::KeyPress('a'));
    setupDrumPad(DrumSimulatorAudioProcessor::TOM2, "", juce::Colours::indigo, juce::KeyPress('s'));
    setupDrumPad(DrumSimulatorAudioProcessor::TOM3, "", juce::Colours::violet, juce::KeyPress('d'));
    setupDrumPad(DrumSimulatorAudioProcessor::RIDE, "", juce::Colours::cyan, juce::KeyPress('f'));
}

void DrumSimulatorAudioProcessorEditor::setupDrumPad(int index, const juce::String& name,
    juce::Colour color, juce::KeyPress key)
{
    auto& pad = drumPads[index];
    pad.name = name;
    pad.padColor = color;
    pad.keyBinding = key;
    pad.drumIndex = index;

    // Create drum pad button
    pad.button = std::make_unique<juce::TextButton>(name);
    pad.button->setButtonText(name + "");
    //pad.button->setLookAndFeel(new BorderlessButtonLookAndFeel());
    pad.button->setColour(juce::TextButton::buttonColourId, juce::Colours::transparentWhite);
    pad.button->setColour(juce::TextButton::buttonOnColourId, juce::Colours::transparentWhite);
    pad.button->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    pad.button->setColour(juce::TextButton::textColourOnId, juce::Colours::white);

    pad.button->addListener(this);
    addAndMakeVisible(*pad.button);

    // Create gain slider
    pad.gainSlider = std::make_unique<juce::Slider>(juce::Slider::LinearVertical, juce::Slider::TextBoxBelow);
    pad.gainSlider->setRange(0.0, 2.0, 0.01);
    pad.gainSlider->setValue(1.0);
    pad.gainSlider->setColour(juce::Slider::thumbColourId, color);
    pad.gainSlider->setColour(juce::Slider::trackColourId, color.withAlpha(0.3f));
    pad.gainSlider->addListener(this);
    addAndMakeVisible(*pad.gainSlider);

    // Create gain label
    pad.gainLabel = std::make_unique<juce::Label>("gain" + juce::String(index), name);
    pad.gainLabel->setJustificationType(juce::Justification::centred);
    pad.gainLabel->setColour(juce::Label::textColourId, juce::Colours::white);
    pad.gainLabel->setFont(juce::Font(10.0f));
    addAndMakeVisible(*pad.gainLabel);

    // Create load button
    pad.loadButton = std::make_unique<juce::TextButton>("Load");
    pad.loadButton->setButtonText("Load");
    pad.loadButton->setColour(juce::TextButton::buttonColourId, juce::Colours::darkgrey);
    pad.loadButton->setColour(juce::TextButton::textColourOffId, juce::Colours::white);
    pad.loadButton->addListener(this);
    addAndMakeVisible(*pad.loadButton);

    // Setup parameter attachment
    juce::String paramId;
    switch (index)
    {
    case DrumSimulatorAudioProcessor::KICK: paramId = "kick_gain"; break;
    case DrumSimulatorAudioProcessor::SNARE: paramId = "snare_gain"; break;
    case DrumSimulatorAudioProcessor::HIHAT: paramId = "hihat_gain"; break;
    case DrumSimulatorAudioProcessor::CRASH: paramId = "crash_gain"; break;
    case DrumSimulatorAudioProcessor::TOM1: paramId = "tom1_gain"; break;
    case DrumSimulatorAudioProcessor::TOM2: paramId = "tom2_gain"; break;
    case DrumSimulatorAudioProcessor::TOM3: paramId = "tom3_gain"; break;
    case DrumSimulatorAudioProcessor::RIDE: paramId = "ride_gain"; break;
    }

    gainAttachments[index] = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.parameters, paramId, *pad.gainSlider);
}

void DrumSimulatorAudioProcessorEditor::drawDrumPad(juce::Graphics& g, const DrumPad& pad)
{
    auto bounds = pad.bounds.toFloat();

    // Draw pad shadow
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(bounds.translated(3, 3), 10.0f);

    // Draw pad background with pressed state
    auto bgColor = pad.isPressed ? pad.padColor.brighter(0.4f) : pad.padColor;
    if (!audioProcessor.isDrumLoaded(pad.drumIndex))
        bgColor = bgColor.withSaturation(0.3f).darker(0.5f);

    g.setColour(bgColor);
    g.fillRoundedRectangle(bounds, 10.0f);

    // Draw border
    g.setColour(pad.padColor.darker(0.3f));
    g.drawRoundedRectangle(bounds, 10.0f, 2.0f);

    // Draw sample status indicator
    if (audioProcessor.isDrumLoaded(pad.drumIndex))
    {
        g.setColour(juce::Colours::lime);
        g.fillEllipse(bounds.getRight() - 15, bounds.getY() + 5, 8, 8);
    }
    else
    {
        g.setColour(juce::Colours::red);
        g.fillEllipse(bounds.getRight() - 15, bounds.getY() + 5, 8, 8);
    }
}

void DrumSimulatorAudioProcessorEditor::triggerDrumPad(int index)
{
    if (index >= 0 && index < DrumSimulatorAudioProcessor::NUM_SOUNDS)
    {
        audioProcessor.triggerDrum(index, 1.0f);
        drumPads[index].isPressed = true;
        repaint();
    }
}

void DrumSimulatorAudioProcessorEditor::loadSampleForDrum(int drumIndex)
{
    juce::FileChooser chooser("Choose a drum sample...",
        juce::File(),
        "*.wav;*.aiff;*.flac;*.ogg;*.mp3");

    chooser.launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, drumIndex](const juce::FileChooser& fc)
        {
            auto file = fc.getResult();
            if (file.existsAsFile())
            {
                audioProcessor.loadSample(drumIndex, file);
                repaint();
            }
        });
}

//==============================================================================
bool DrumSimulatorAudioProcessorEditor::keyPressed(const juce::KeyPress& key, juce::Component* /*originatingComponent*/)
{
    for (int i = 0; i < DrumSimulatorAudioProcessor::NUM_SOUNDS; ++i)
    {
        if (drumPads[i].keyBinding == key)
        {
            triggerDrumPad(i);
            return true;
        }
    }
    return false;
}

void DrumSimulatorAudioProcessorEditor::timerCallback()
{
    // Reset pressed states for visual feedback
    bool needsRepaint = false;
    for (auto& pad : drumPads)
    {
        if (pad.isPressed)
        {
            pad.isPressed = false;
            needsRepaint = true;
        }
    }

    if (needsRepaint)
        repaint();
}

void DrumSimulatorAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    // Check if it's a drum pad button
    for (int i = 0; i < DrumSimulatorAudioProcessor::NUM_SOUNDS; ++i)
    {
        auto& pad = drumPads[i];
        if (button == pad.button.get())
        {
            triggerDrumPad(i);
            return;
        }
        else if (button == pad.loadButton.get())
        {
            loadSampleForDrum(i);
            return;
        }
    }
}

void DrumSimulatorAudioProcessorEditor::sliderValueChanged(juce::Slider* /*slider*/)
{
    // Parameter attachments handle this automatically
}