/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#include "MainComponent.h"
#include "Maximilian.h"

//==============================================================================
SynthSound::SynthSound(){}

bool SynthSound::appliesToNote( int )    { return true; }
bool SynthSound::appliesToChannel( int ) { return true; }


//==============================================================================
SynthVoice::SynthVoice( int samplesPerBlockExpected ){
    samplesPerBlock = samplesPerBlockExpected;

}



bool SynthVoice::canPlaySound (SynthesiserSound* sound){
    return dynamic_cast<SynthSound*> (sound) != nullptr;
}

void SynthVoice::startNote (int midiNoteNumber, float velocity,
                            SynthesiserSound*, int /*currentPitchWheelPosition*/) {
    
    frequency = MidiMessage::getMidiNoteInHertz (midiNoteNumber);
    currentSampleRate = getSampleRate();
    
    //for sine
    currentSampleRate = getSampleRate();
    updateAngleDelta();


}
void SynthVoice::stopNote (float /*velocity*/, bool allowTailOff){
    clearCurrentNote();
    angleDelta = 0.0;
}

void SynthVoice::renderNextBlock (AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    //implementation of sine wave
    
    if (angleDelta != 0.0){
        auto level = 0.125f;
    
        for (auto sample = 0; sample < numSamples; ++sample)
        {
            auto currentSample = (float) std::sin (currentAngle);
            currentAngle += angleDelta;
            for (auto i = outputBuffer.getNumChannels(); --i >= 0;){
                
                outputBuffer.addSample(i, sample, currentSample*level);
                
            }
        }
        
    }
}

void SynthVoice::pitchWheelMoved (int newPitchWheelValue){}

void SynthVoice::controllerMoved (int, int){}

//just for temp sine wave
void SynthVoice::updateAngleDelta()
{
    auto cyclesPerSample = frequency / currentSampleRate;
    angleDelta = cyclesPerSample * 2.0 * MathConstants<double>::pi;
}

forcedinline float SynthVoice::getNextSample() noexcept
{
    auto tableSize = wavetable.getNumSamples();
    
    auto index0 = (unsigned int) currentIndex;
    auto index1 = index0 == (tableSize - 1) ? (unsigned int) 0 : index0 + 1;
    
    auto frac = currentIndex - (float) index0;
    
    auto* table = wavetable.getReadPointer (0);
    auto value0 = table[index0];
    auto value1 = table[index1];
    
    auto currentSample = value0 + frac * (value1 - value0);
    
    if ((currentIndex += tableDelta) > tableSize)
        currentIndex -= tableSize;
        
        return currentSample;
}

//==============================================================================
SynthAudioSource::SynthAudioSource (MidiKeyboardState& keyState)
: keyboardState (keyState){}

void SynthAudioSource::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    for (auto i = 0; i < 12; ++i){ //12 note polyphonic synth
        synth.addVoice (new SynthVoice( samplesPerBlockExpected ));
    }
    
    synth.addSound (new SynthSound());
    synth.setCurrentPlaybackSampleRate (sampleRate);
    midiCollector.reset (sampleRate);
}

void SynthAudioSource::releaseResources(){}

void SynthAudioSource::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
    
    MidiBuffer incomingMidi;
    incomingMidi.clear();
    midiCollector.removeNextBlockOfMessages (incomingMidi, bufferToFill.numSamples);
    keyboardState.processNextMidiBuffer (incomingMidi, bufferToFill.startSample,
                                         bufferToFill.numSamples, true);
    
    synth.renderNextBlock (*bufferToFill.buffer, incomingMidi,
                           bufferToFill.startSample, bufferToFill.numSamples);
    
    
}

void SynthAudioSource::createWavetables()
{
    for (AudioSampleBuffer table: {sineTable, squareTable, sawTable, triangleTable}){
        tableArray.add(&table);
    }

    int harmonics[8];
    float harmonicWeights[8];
    int oddHarms[] =  { 1, 3, 5, 7, 9, 11, 13, 15};
    int everyHarm[] = { 1, 2, 3, 4, 5, 6, 7, 8};
    float sineWeights [] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float squareWeights[] = { 1.0f, 0.33f, 0.2f, 0.14285f, 0.111f, 0.0909091f, 0.0769231f, 0.0666667f};
    float sawWeights[] = { 1.0f, 0.5f, 0.33f, 0.25f, 0.02f, 0.166667f, 0.14285f, 0.125f};
    float triangleWeights[] = { 1.0f, 0.111f, 0.04f, 0.02040816f, 0.012345679f, 0.00826446f, 0.00591716f, 0.004444444f};

    
    for(AudioSampleBuffer* tablePtr : tableArray){
        for( int i: harmonics){
            if( tablePtr == &sineTable){
                harmonics[i] = oddHarms[i];
                harmonicWeights[i] = sineWeights[i];
            }
            else if (tablePtr == &squareTable){
                harmonics[i] = oddHarms[i];
                harmonicWeights[i] = squareWeights[i];
            }
            else if (tablePtr == &sawTable){
                harmonics[i] = everyHarm[i];
                harmonicWeights[i] = sawWeights[i];
            }
            else if (tablePtr == &triangleTable){
                harmonics[i] = oddHarms[i];
                harmonicWeights[i] = triangleWeights[i];
            }
        }

        tablePtr->setSize (1, tableSize + 1);
        tablePtr->clear();

    
        auto* samples = tablePtr->getWritePointer (0);
    
        jassert (numElementsInArray (harmonics) == numElementsInArray (harmonicWeights));
    
        for (auto harmonic = 0; harmonic < numElementsInArray (harmonics); ++harmonic)
        {
            auto angleDelta = MathConstants<double>::twoPi / (double) (tableSize - 1) * harmonics[harmonic];
            auto currentAngle = 0.0;
        
            for (auto i = 0; i < tableSize; ++i)
            {
                auto sample = std::sin (currentAngle);
                samples[i] += (float) sample * harmonicWeights[harmonic];
                currentAngle += angleDelta;
            }
        }
    
        samples[tableSize] = samples[0];
        
    }
}

//todo, set synthvoices to read tablearray[index]
void SynthAudioSource::setWaveForm (int index)
{
  
}

//=============================================================================

void MainComponent::setMidiInput (int index)
{
    auto list = MidiInput::getDevices();
    
    deviceManager.removeMidiInputCallback (list[lastInputIndex], synthAudioSource.getMidiCollector());
    
    auto newInput = list[index];
    
    if (! deviceManager.isMidiInputEnabled (newInput))
        deviceManager.setMidiInputEnabled (newInput, true);
    
    deviceManager.addMidiInputCallback (newInput, synthAudioSource.getMidiCollector());
    midiInputList.setSelectedId (index + 1, dontSendNotification);
    
    lastInputIndex = index;
}

//let's see if this actually works lol
void MainComponent::setWaveForm (int index)
{
    synthAudioSource.setWaveForm(index);
}

void MainComponent::handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message)
{
    const ScopedValueSetter<bool> scopedInputFlag (isAddingFromMidiInput, true);
    keyboardState.processNextMidiEvent (message);
    //postMessageToList (message, source->getName());
}

void MainComponent::handleNoteOn (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/)
{
    if (! isAddingFromMidiInput)
    {
        MidiMessage m (MidiMessage::noteOn (midiChannel, midiNoteNumber, 1.0f));
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        addMessageToBuffer (m);
    }
}

void MainComponent::handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float /*velocity*/)
{
    if (! isAddingFromMidiInput)
    {
        MidiMessage m (MidiMessage::noteOff (midiChannel, midiNoteNumber));
        m.setTimeStamp (Time::getMillisecondCounterHiRes() * 0.001);
        addMessageToBuffer (m);
    }
}

void MainComponent::comboBoxChanged (ComboBox* box)
{
    if (box == &midiInputList)
        setMidiInput (midiInputList.getSelectedItemIndex());
    if (box == &waveFormList)
        setWaveForm(waveFormList.getSelectedItemIndex());
}

void MainComponent::addMessageToBuffer (const MidiMessage& message)
{
    auto timestamp = message.getTimeStamp();
    auto sampleNumber =  (int) (timestamp * prevSampleRate);
    midiBuffer.addEvent (message, sampleNumber);
}

MidiMessageCollector* SynthAudioSource::getMidiCollector()
{
    return &midiCollector;
}

//==============================================================================
MainComponent::MainComponent():
        keyboardComponent (keyboardState, MidiKeyboardComponent::horizontalKeyboard),
        synthAudioSource (keyboardState)

{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (800, 600);
    
    //add labels
    addAndMakeVisible (midiInputListLabel);
    midiInputListLabel.setText ("MIDI Input:", dontSendNotification);
    midiInputListLabel.attachToComponent (&midiInputList, true);
    addAndMakeVisible (midiInputList);
    
    //add midi input combo box
    midiInputList.setTextWhenNoChoicesAvailable ("No MIDI Inputs Enabled");
    auto midiInputs = MidiInput::getDevices();
    midiInputList.addItemList (midiInputs, 1);
    midiInputList.onChange = [this] { setMidiInput (midiInputList.getSelectedItemIndex()); };
    // find the first enabled device and use that by default
    for (auto midiInput : midiInputs)
    {
        if (deviceManager.isMidiInputEnabled (midiInput))
        {
            setMidiInput (midiInputs.indexOf (midiInput));
            break;
        }
    }
    // if no enabled devices were found just use the first one in the list
    if (midiInputList.getSelectedId() == 0)
        setMidiInput (0);
    
    
    addAndMakeVisible(keyboardComponent);
    keyboardState.addListener (this);
    
    
    // Some platforms require permissions to open input channels so request that here
    if (RuntimePermissions::isRequired (RuntimePermissions::recordAudio)
        && ! RuntimePermissions::isGranted (RuntimePermissions::recordAudio))
    {
        RuntimePermissions::request (RuntimePermissions::recordAudio,
                                     [&] (bool granted) { if (granted)  setAudioChannels (2, 2); });
    }
    else
    {
        // Specify the number of input and output channels that we want to open
        setAudioChannels (2, 2);
    }
}

MainComponent::~MainComponent()
{
    // This shuts down the audio device and clears the audio source.
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay (int samplesPerBlockExpected, double sampleRate)
{
    synthAudioSource.prepareToPlay (samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill)
{
    bufferToFill.clearActiveBufferRegion();
    
    synthAudioSource.getNextAudioBlock (bufferToFill);
    
    for (auto channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
    {
        // Get a pointer to the start sample in the buffer for this audio output channel
        auto* buffer = bufferToFill.buffer->getWritePointer (channel, bufferToFill.startSample);
        
        for (auto sample = 0; sample < bufferToFill.numSamples; ++sample){
            if( buffer[sample] > 1.0f ) buffer[sample] = 1.0f;
            if( buffer[sample] < -1.0f ) buffer[sample] = -1.0f;
            
        }
    }
}

void MainComponent::releaseResources()
{
    synthAudioSource.releaseResources ();
}

//==============================================================================
void MainComponent::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    // You can add your drawing code here!
}

void MainComponent::resized()
{
    // This is called when the MainContentComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
    midiInputList.setBounds(100, 25, getWidth() - 150, 20);
    keyboardComponent.setBounds (10, 140, getWidth() - 20, 120);
}
