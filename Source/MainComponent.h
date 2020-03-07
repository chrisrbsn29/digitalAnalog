/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include "../JuceLibraryCode/JuceHeader.h"
#include "Maximilian.h"

//==============================================================================
struct SynthSound   : public SynthesiserSound
{
public:
    SynthSound();
    
    bool appliesToNote    (int) override;
    bool appliesToChannel (int) override;
};

//==============================================================================
struct SynthVoice   : public SynthesiserVoice

{
public:
    
    SynthVoice( int samplesPerBlockExpected );
    bool canPlaySound (SynthesiserSound* sound) override;
    void startNote (int midiNoteNumber, float velocity,
                    SynthesiserSound*, int /*currentPitchWheelPosition*/) override;
    void stopNote (float /*velocity*/, bool allowTailOff) override;
    void renderNextBlock (AudioSampleBuffer& outputBuffer, int startSample, int numSamples) override;
    void pitchWheelMoved (int newPitchWheelValue) override;
    void controllerMoved (int, int) override;
    forcedinline float getNextSample() noexcept;
    
    int samplesPerBlock;
    double currentSampleRate = 0.0, currentAngle = 0.0, angleDelta = 0.0;
    float currentIndex = 0.0f, tableDelta = 0.0f;
    void updateAngleDelta();
    
private:
    double frequency;
    AudioSampleBuffer wavetable;
    maxiEnv env1;
    
};
//==============================================================================
    
    class SynthAudioSource   : public AudioSource
{
public:
    SynthAudioSource (MidiKeyboardState& keyState);
    
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    MidiMessageCollector* getMidiCollector();
    void createWavetables();
    void setWaveForm(int index);
    
private:
    MidiKeyboardState& keyboardState;
    Synthesiser synth;
    MidiMessageCollector midiCollector;
    const unsigned int tableSize = 1 << 7;
    float level = 0.0f;
    int samplesPerBlock;
    
    AudioSampleBuffer sineTable;
    AudioSampleBuffer squareTable;
    AudioSampleBuffer sawTable;
    AudioSampleBuffer triangleTable;
    OwnedArray<AudioSampleBuffer> tableArray;
    
};

//==============================================================================
/*
 This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public AudioAppComponent,
                        private ComboBox::Listener,
                        private MidiInputCallback,
                        private MidiKeyboardStateListener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (Graphics& g) override;
    void resized() override;
    void setMidiInput (int index);
    void setWaveForm(int index);
    void handleIncomingMidiMessage (MidiInput* source, const MidiMessage& message) override;
    void handleNoteOn (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void handleNoteOff (MidiKeyboardState*, int midiChannel, int midiNoteNumber, float velocity) override;
    void comboBoxChanged (ComboBox* box) override;
    void addMessageToBuffer (const MidiMessage& message);
    
    ComboBox midiInputList;
    ComboBox waveFormList;
    AudioDeviceManager deviceManager;
    int lastInputIndex = 0;
    bool isAddingFromMidiInput = false;
    

private:
    //==============================================================================
    // Your private member variables go here...
    MidiKeyboardState keyboardState;
    MidiKeyboardComponent keyboardComponent;
    SynthAudioSource synthAudioSource;

    Label midiInputListLabel;
    double prevSampleRate;
    MidiBuffer midiBuffer;
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
