/*
  ==============================================================================

 Created by Shaikat Hossain 10-02-2018

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include <map>

//==============================================================================
namespace Parameters
{
    static const Identifier currentSample  { "currentSample" };

    static const Identifier reverbEnabled  { "reverbEnabled" };
    static const Identifier roomSize       { "roomSize" };
    static const Identifier damping        { "damping" };
    static const Identifier width          { "width" };

    static const Identifier attack         { "attack" };
    static const Identifier decay          { "decay" };
    static const Identifier sustain        { "sustain" };
    static const Identifier release        { "release" };

    struct ParameterInfo
    {
        String labelName;
        float defaultValue;
    };

    static std::map<Identifier, ParameterInfo> parameterInfoMap
    {
        { currentSample, { "Current Sample", 3.0f } },

        { reverbEnabled, { "Reverb Enabled", 1.0f } },
        { roomSize,      { "Room Size",      0.75f } },
        { damping,       { "Damping",        0.5f } },
        { width,         { "Width",          1.0f } },

        { attack,        { "Attack",         0.1f } },
        { decay,         { "Decay",          0.1f } },
        { sustain,       { "Sustain",        1.0f } },
        { release,       { "Release",        0.1f } }
    };

    //==============================================================================
    static inline StringArray getSampleFilenames()
    {
        StringArray filenames;

        for (int i = 0; i < BinaryData::namedResourceListSize; ++i)
            filenames.add (BinaryData::getNamedResourceOriginalFilename (BinaryData::namedResourceList[i]));

        return filenames;
    }

    static inline InputStream* createInputStreamForSampleFile (int index)
    {
        jassert (isPositiveAndBelow (index, BinaryData::namedResourceListSize));

        auto fileName = BinaryData::namedResourceList[index];

        int dataSize = 0;
        auto* data = BinaryData::getNamedResource (fileName, dataSize);

        return new MemoryInputStream (data, dataSize, false);
    }
}

//==============================================================================
class SamplerAudioProcessor  : public AudioProcessor,
                               private AsyncUpdater
{
private:
    AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

public:
    //==============================================================================
    SamplerAudioProcessor();
    ~SamplerAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void reset() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                 { return true; }

    //==============================================================================
    const String getName() const override           { return JucePlugin_Name; }

    bool acceptsMidi() const override               { return true; }
    bool producesMidi() const override              { return false; }
    bool isMidiEffect() const override              { return false; }
    double getTailLengthSeconds() const override    { return 0.0; }

    //==============================================================================
    int getNumPrograms() override                   { return 1; }
    int getCurrentProgram() override                { return 0; }

    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    MidiKeyboardState& getKeyboardState()           { return midiKeyboardState; }
    AudioProcessorValueTreeState& getAPVTS()        { return state; }
    AudioFormatManager& getAudioFormatManager()     { return formatManager; }

    //==============================================================================
    void setADSRParametersNeedUpdating()            { adsrParametersNeedUpdating.test_and_set(); }
    void setSampleNeedsUpdating()                   { sampleNeedsUpdating.test_and_set(); }

private:
    void handleAsyncUpdate() override;

    //==============================================================================
    AudioFormatManager formatManager;
    MidiKeyboardState midiKeyboardState;

    Synthesiser synth;
    SynthesiserSound::Ptr sound;

    Reverb reverb;
    Reverb::Parameters reverbParameters;
    bool needToResetReverb             = false;
    AudioParameterBool*  reverbEnabled = nullptr;
    AudioParameterFloat* roomSize      = nullptr;
    AudioParameterFloat* damping       = nullptr;
    AudioParameterFloat* width         = nullptr;

    std::atomic_flag adsrParametersNeedUpdating  { true };
    AudioParameterFloat* adsrParams[4];

    std::atomic_flag sampleNeedsUpdating  { true };
    std::atomic_flag sampleIsLoaded       { false };
    AudioParameterChoice* currentSample = nullptr;

    AudioProcessorValueTreeState state;

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerAudioProcessor)
};
