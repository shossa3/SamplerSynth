/*
  ==============================================================================

Created by Shaikat Hossain 10-02-2018
 
  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"

//==============================================================================
class SamplerAudioProcessorEditor  : public AudioProcessorEditor,
                                     private AudioProcessorValueTreeState::Listener,
                                     private ChangeListener
{
public:
    SamplerAudioProcessorEditor (SamplerAudioProcessor&);
    ~SamplerAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

    //==============================================================================
    void updateThumbnail (int);

private:
    void parameterChanged (const String&, float) override;
    void changeListenerCallback (ChangeBroadcaster*) override;

    void addParameterListeners();
    void removeParameterListeners();

    void addFloatParameter (const Identifier&);
    void addReverbEnabledParameter();
    void addFileSelectorParameter();

    //==============================================================================
    struct RotarySliderWithLabel    : public Component
    {
        RotarySliderWithLabel (const Identifier& paramID)
            : identifier (paramID),
              slider (identifier.toString()),
              label  (identifier.toString(), Parameters::parameterInfoMap[paramID].labelName)
        {
            slider.setSliderStyle (Slider::Rotary);
            slider.setTextBoxStyle (Slider::TextBoxBelow, false, slider.getTextBoxWidth(), slider.getTextBoxHeight());
            addAndMakeVisible (slider);

            label.setJustificationType (Justification::centred);
            addAndMakeVisible (label);

        }

        void resized() override
        {
            auto bounds = getLocalBounds();

            label.setBounds (bounds.removeFromTop (25));
            slider.setBounds (bounds);
        }

        Identifier identifier;
        Slider slider;
        Label label;
    };

    //==============================================================================
    SamplerAudioProcessor& processor;
    MidiKeyboardComponent midiKeyboard;

    Rectangle<int> thumbnailBounds;
    std::unique_ptr<AudioThumbnail> thumbnail;
    AudioThumbnailCache thumbnailCache { 5 };

    using SliderAttachment = AudioProcessorValueTreeState::SliderAttachment;

    OwnedArray<RotarySliderWithLabel> parameterSliders;
    OwnedArray<SliderAttachment> parameterSliderAttachments;

    using ButtonAttachment = AudioProcessorValueTreeState::ButtonAttachment;

    ToggleButton reverbEnabledToggle;
    std::unique_ptr<ButtonAttachment> reverbEnabledToggleAttachment;

    using ComboBoxAttachment = AudioProcessorValueTreeState::ComboBoxAttachment;

    ComboBox fileSelector;
    std::unique_ptr<ComboBoxAttachment> fileSelectorAttachment;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerAudioProcessorEditor)
};
