/*
  ==============================================================================

 Created by Shaikat Hossain 10-02-2018

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
SamplerAudioProcessorEditor::SamplerAudioProcessorEditor (SamplerAudioProcessor& p)
    : AudioProcessorEditor (&p),
      processor (p),
      midiKeyboard (processor.getKeyboardState(), MidiKeyboardComponent::horizontalKeyboard)
{
    addAndMakeVisible (midiKeyboard);

    thumbnail.reset (new AudioThumbnail (64, processor.getAudioFormatManager(), thumbnailCache));
    thumbnail->addChangeListener (this);

    for (auto paramID : { Parameters::roomSize, Parameters::damping, Parameters::width,
                          Parameters::attack, Parameters::decay, Parameters::sustain, Parameters::release })
        addFloatParameter (paramID);

    addReverbEnabledParameter();
    addFileSelectorParameter();

    addParameterListeners();

    updateThumbnail (roundToInt (*processor.getAPVTS().getRawParameterValue (Parameters::currentSample)));

    setOpaque (true);
    setSize (500, 600);
}

SamplerAudioProcessorEditor::~SamplerAudioProcessorEditor()
{
    removeParameterListeners();
    thumbnail->removeChangeListener (this);
    thumbnail->clear();
}

//==============================================================================
void SamplerAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    g.setColour (Colours::lightgrey);
    g.fillRect (thumbnailBounds);

    g.setColour (Colours::darkgrey);

    if (thumbnail->getNumChannels() == 0)
        g.drawFittedText ("No file loaded", thumbnailBounds, Justification::centred, 1, 1.0f);
    else
        thumbnail->drawChannels (g, thumbnailBounds, 0.0, thumbnail->getTotalLength(), 1.0f);
}

void SamplerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds().reduced (10);

    auto fileSelectorSlice = bounds.removeFromTop (35);
    fileSelector.setBounds (fileSelectorSlice.removeFromLeft (250));

    midiKeyboard.setBounds (bounds.removeFromBottom (75));
    bounds.removeFromBottom (10);

    static constexpr int sliderWidth = 120;
    static constexpr int sliderGap = 5;

    auto paramSlice = bounds.removeFromBottom (250);
    auto reverbParamsSlice = paramSlice.removeFromTop (roundToInt (paramSlice.getHeight() / 2.0f)).reduced (0, 5);

    reverbEnabledToggle.setBounds (reverbParamsSlice.removeFromLeft (120));
    reverbParamsSlice.removeFromLeft (sliderGap);

    auto getSliderForParameter = [this] (const Identifier& paramID) -> RotarySliderWithLabel*
    {
        for (auto* s : parameterSliders)
            if (s->identifier == paramID)
                return s;

        jassertfalse;
        return nullptr;
    };

    for (auto paramID : { Parameters::roomSize, Parameters::damping, Parameters::width })
    {
        getSliderForParameter (paramID)->setBounds (reverbParamsSlice.removeFromLeft (sliderWidth));
        reverbParamsSlice.removeFromLeft (sliderGap);
    }

    auto adsrParamsSlice = paramSlice.reduced ((paramSlice.getWidth() - (sliderWidth * 4)) / 2, 5);

    for (auto paramID : { Parameters::attack, Parameters::decay, Parameters::sustain, Parameters::release })
    {
        getSliderForParameter (paramID)->setBounds (adsrParamsSlice.removeFromLeft (sliderWidth));
        adsrParamsSlice.removeFromLeft (sliderGap);
    }

    thumbnailBounds = bounds.reduced (0, 10);
}

//==============================================================================
void SamplerAudioProcessorEditor::updateThumbnail (int newIndex)
{
    if (auto* reader = processor.getAudioFormatManager().createReaderFor (Parameters::createInputStreamForSampleFile (newIndex)))
        thumbnail->setReader (reader, reader->lengthInSamples);
}

//==============================================================================
void SamplerAudioProcessorEditor::parameterChanged (const String& parameterID, float newValue)
{
    if (parameterID == Parameters::attack.toString() || parameterID == Parameters::decay.toString()
        || parameterID == Parameters::sustain.toString() || parameterID == Parameters::release.toString())
    {
        processor.setADSRParametersNeedUpdating();
    }
    else if (parameterID == Parameters::currentSample.toString())
    {
        processor.setSampleNeedsUpdating();
        updateThumbnail (roundToInt (newValue));
    }
}

void SamplerAudioProcessorEditor::changeListenerCallback (ChangeBroadcaster* source)
{
    if (source == thumbnail.get())
        repaint();
}

void SamplerAudioProcessorEditor::addParameterListeners()
{
    auto& state = processor.getAPVTS();

    state.addParameterListener (Parameters::attack.toString(),  this);
    state.addParameterListener (Parameters::decay.toString(),   this);
    state.addParameterListener (Parameters::sustain.toString(), this);
    state.addParameterListener (Parameters::release.toString(), this);

    state.addParameterListener (Parameters::currentSample.toString(), this);
}

void SamplerAudioProcessorEditor::removeParameterListeners()
{
    auto& state = processor.getAPVTS();

    state.removeParameterListener (Parameters::attack.toString(),  this);
    state.removeParameterListener (Parameters::decay.toString(),   this);
    state.removeParameterListener (Parameters::sustain.toString(), this);
    state.removeParameterListener (Parameters::release.toString(), this);

    state.removeParameterListener (Parameters::currentSample.toString(), this);
}

void SamplerAudioProcessorEditor::addFloatParameter (const Identifier& paramID)
{
    auto* sliderComp = parameterSliders.add (new RotarySliderWithLabel (paramID));
    addAndMakeVisible (sliderComp);

    parameterSliderAttachments.add (new SliderAttachment (processor.getAPVTS(), paramID.toString(), sliderComp->slider));
}

void SamplerAudioProcessorEditor::addReverbEnabledParameter()
{
    reverbEnabledToggle.setButtonText ("Reverb Enabled");
    addAndMakeVisible (reverbEnabledToggle);

    reverbEnabledToggleAttachment.reset (new ButtonAttachment (processor.getAPVTS(), Parameters::reverbEnabled.toString(), reverbEnabledToggle));
}

void SamplerAudioProcessorEditor::addFileSelectorParameter()
{
    fileSelector.addItemList (Parameters::getSampleFilenames(), 1);
    addAndMakeVisible (fileSelector);

    fileSelectorAttachment.reset (new ComboBoxAttachment (processor.getAPVTS(), Parameters::currentSample.toString(), fileSelector));
}
