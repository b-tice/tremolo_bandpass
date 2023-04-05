// Project: SoapFilter Example
//
// Written by: Brian Tice, 4/4/2023

#include "daisysp.h"            // Open Source DSP Library
#include "daisy_pod.h"          // libDaisy, hardware abstraction library for
                                // the daisy platform

#define PI 3.141592653589793

#define TRM 2   // Tremolo Test State. This is just a tremolo with depth and rate
                // LED's are pink in this state. POT1 is rate, POT2 is depth.
                
#define BNP 1   // Band Pass SVF filter Test State. LED's are GREEN in this state.
                // A test of the built in SVF.
                // POT1 is center frequency, POT2 is q.

#define SAB 0   // Second Order All Pass Bandpass Filter 
                // LED's are BLUE in this state.
                // POT1 is center frequency, POT2 is q. 
            
using namespace daisysp;
using namespace daisy;

static DaisyPod pod;

static Tremolo                                   trem;
static Svf                                       filt;
static Tone                                      tone;
static Soap                                      soap;
int                                              mode = SAB;

//Helper functions
void Controls();

void GetTremoloSample(float &outl, float &outr, float inl, float inr);

void GetBandPassSample(float &outl, float &outr, float inl, float inr);

void GetSoapSample(float &outl, float &outr, float inl, float inr);

void AudioCallback(AudioHandle::InterleavingInputBuffer  in,
                   AudioHandle::InterleavingOutputBuffer out,
                   size_t                                size)
{
    float outl, outr, inl, inr;

    Controls();

    //audio
    for(size_t i = 0; i < size; i += 2)
    {
        inl = in[i];
        inr = in[i + 1];

        switch(mode)
        {

            case TRM: GetTremoloSample(outl, outr, inl, inr); break;
            case BNP: GetBandPassSample(outl, outr, inl, inr); break;
            case SAB: GetSoapSample(outl, outr, inl, inr); break;
                      
            default: outl = outr = 0;
        }

        // left out
        out[i] = outl;

        // right out
        out[i + 1] = outr;
    }
}

int main(void)
{
    // initialize pod hardware and oscillator daisysp module
    float sample_rate;

    // Inits and sample rate
    pod.Init();
    pod.SetAudioBlockSize(4);
    sample_rate = pod.AudioSampleRate();

    trem.Init(sample_rate);
    filt.Init(sample_rate);
    tone.Init(sample_rate);
    soap.Init(sample_rate); 

    // tremolo parameters
    trem.SetFreq(2);
    trem.SetDepth(0.75);

    // SVF bandpass filter settings
    filt.SetFreq(300.0);
    filt.SetRes(0.85);
    filt.SetDrive(0.8);

    // Soap filter settings
    soap.SetCenterFreq(400.0);
    soap.SetFilterBandwidth(50.0);

    // start callback
    pod.StartAdc();
    pod.StartAudio(AudioCallback);

    while(1) {}
}

void UpdateKnobs(float &k1, float &k2)
{
    k1 = pod.knob1.Process();
    k2 = pod.knob2.Process();

    switch(mode)
    {

        case TRM:
            trem.SetFreq(k1*3);
            trem.SetDepth(k2);
        case BNP:
            filt.SetFreq(k1*3000);
        case SAB:         
            soap.SetCenterFreq(k1*1000.0);           
            soap.SetFilterBandwidth(k2*100.0);
    }
}

void UpdateEncoder()
{
    mode = mode + pod.encoder.Increment();
    mode = (mode % 3 + 3) % 3;
}

void UpdateLeds(float k1, float k2)
{
    pod.led1.Set(
        k1 * (mode == 2), k1 * (mode == 1), k1 * (mode == 0 || mode == 2));
    pod.led2.Set(
        k2 * (mode == 2), k2 * (mode == 1), k2 * (mode == 0 || mode == 2));

    pod.UpdateLeds();
}

void Controls()
{
    float k1, k2;
   
    pod.ProcessAnalogControls();
    pod.ProcessDigitalControls();

    UpdateKnobs(k1, k2);
    UpdateEncoder();
    UpdateLeds(k1, k2);
}

void GetTremoloSample(float &outl, float &outr, float inl, float inr)
{
    outl = trem.Process(inl);
    outr = outl;
}

void GetBandPassSample(float &outl, float &outr, float inl, float inr)
{

    filt.Process(inl);
    outl = filt.Band();
    outr = outl;

}

void GetSoapSample(float &outl, float &outr, float inl, float inr) 
{   
    // Add in the Soap Bandpass
    outl = soap.Process(inl) * 3;
    outr = outl;
}





