/*
  ==============================================================================

 Created by Shaikat Hossain 10-02-2018

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

static constexpr int maxNumVoices = 16;

//==============================================================================
AudioProcessorValueTreeState::ParameterLayout SamplerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> params;

    {
        using FloatParamPair = std::pair<Identifier, AudioParameterFloat*&>;

        for (auto p : { FloatParamPair (Parameters::roomSize, roomSize),
                        FloatParamPair (Parameters::damping,  damping),
                        FloatParamPair (Parameters::width,    width),
                        FloatParamPair (Parameters::attack,   adsrParams[0]),
                        FloatParamPair (Parameters::decay,    adsrParams[1]),
                        FloatParamPair (Parameters::sustain,  adsrParams[2]),
                        FloatParamPair (Parameters::release,  adsrParams[3]) })
        {
            auto& info = Parameters::parameterInfoMap[p.first];
            auto param = std::make_unique<AudioParameterFloat> (p.first.toString(), info.labelName, NormalisableRange<float>(), info.defaultValue);

            p.second = param.get();
            params.push_back (std::move (param));
        }
    }

    {
        auto& info = Parameters::parameterInfoMap[Parameters::currentSample];
        auto param = std::make_unique<AudioParameterChoice> (Parameters::currentSample.toString(), info.labelName,
                                                             Parameters::getSampleFilenames(), static_cast<int> (info.defaultValue));

        currentSample = param.get();
        params.push_back (std::move (param));
    }

    {
        auto& info = Parameters::parameterInfoMap[Parameters::reverbEnabled];
        auto param = std::make_unique<AudioParameterBool> (Parameters::reverbEnabled.toString(), info.labelName,
                                                           static_cast<bool> (roundToInt (info.defaultValue)));

        reverbEnabled = param.get();
        params.push_back (std::move (param));
    }

    return { params.begin(), params.end() };
}

//==============================================================================
SamplerAudioProcessor::SamplerAudioProcessor()
     : AudioProcessor (BusesProperties().withOutput ("Output", AudioChannelSet::stereo(), true)),
       state (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    for (int i = 0; i < maxNumVoices; ++i)
        synth.addVoice (new SamplerVoice());

    formatManager.registerBasicFormats();
}

SamplerAudioProcessor::~SamplerAudioProcessor()
{
}

//==============================================================================
void SamplerAudioProcessor::setCurrentProgram (int /*index*/)
{
}

const String SamplerAudioProcessor::getProgramName (int /*index*/)
{
    return {};
}

void SamplerAudioProcessor::changeProgramName (int /*index*/, const String&)
{
}

//==============================================================================
void SamplerAudioProcessor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    midiKeyboardState.reset();
    synth.setCurrentPlaybackSampleRate (sampleRate);
    reverb.setSampleRate (sampleRate);
}

void SamplerAudioProcessor::releaseResources()
{
}

void SamplerAudioProcessor::reset()
{
    reverb.reset();
}

bool SamplerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    return true;
}

void SamplerAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    if (sampleNeedsUpdating.test_and_set())
    {
        triggerAsyncUpdate();
        sampleIsLoaded.clear();
    }

    sampleNeedsUpdating.clear();

    if (adsrParametersNeedUpdating.test_and_set())
    {
        if (auto* samplerSound = dynamic_cast<SamplerSound*> (sound.get()))
            samplerSound->setEnvelopeParameters ({ *adsrParams[0], *adsrParams[1], *adsrParams[2], *adsrParams[3] });
    }

    adsrParametersNeedUpdating.clear();

    reverbParameters.roomSize = *roomSize;
    reverbParameters.damping  = *damping;
    reverbParameters.width    = *width;

    reverb.setParameters (reverbParameters);

    if (! sampleIsLoaded.test_and_set())
    {
        sampleIsLoaded.clear();
        buffer.clear();
        return;
    }

    const auto numSamples = buffer.getNumSamples();

    midiKeyboardState.processNextMidiBuffer (midiMessages, 0, numSamples, true);
    synth.renderNextBlock (buffer, midiMessages, 0, numSamples);

    if (*reverbEnabled)
    {
        if (! needToResetReverb)
            needToResetReverb = true;

        if (buffer.getNumChannels() == 1)
            reverb.processMono (buffer.getWritePointer (0), numSamples);
        else
            reverb.processStereo (buffer.getWritePointer (0), buffer.getWritePointer (1), numSamples);
    }
    else if (needToResetReverb)
    {
        reverb.reset();
        needToResetReverb = false;
    }

    midiMessages.clear();
}

//==============================================================================
AudioProcessorEditor* SamplerAudioProcessor::createEditor()
{
    return new SamplerAudioProcessorEditor (*this);
}

//==============================================================================
void SamplerAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    std::unique_ptr<XmlElement> xmlState (state.copyState().createXml());

    if (xmlState.get() != nullptr)
        copyXmlToBinary (*xmlState, destData);
}

void SamplerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        state.replaceState (ValueTree::fromXml (*xmlState));
}

//==============================================================================
static int getMIDINoteRootForSample (const String& sampleName)
{
    if      (sampleName.contains ("cowbell"))  return 75;
    else if (sampleName.contains ("guitar"))   return 74;
    else if (sampleName.contains ("laser"))    return 74;
    else if (sampleName.contains ("singing"))  return 65;

    jassertfalse;
    return 60;
}

void SamplerAudioProcessor::handleAsyncUpdate()
{
    auto index = currentSample->getIndex();

    if (auto* reader = formatManager.createReaderFor (Parameters::createInputStreamForSampleFile (index)))
    {
        std::unique_ptr<AudioFormatReader> formatReader (reader);

        BigInteger midiNotes;
        midiNotes.setRange (0, 127, true);

        SynthesiserSound::Ptr newSound = new SamplerSound ("Voice", *formatReader, midiNotes, getMIDINoteRootForSample (BinaryData::namedResourceList[index]),
                                                           *adsrParams[0], *adsrParams[3], 10.0);

        synth.removeSound (0);
        sound = newSound;
        synth.addSound (sound);

        adsrParametersNeedUpdating.test_and_set();
        sampleIsLoaded.test_and_set();
    }
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SamplerAudioProcessor();
}
