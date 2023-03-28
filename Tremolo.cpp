// Project: Tremolo-Bandpass Project
//
// Written by: Brian Tice, 2/12/2023
//
// Description: This file contains protoype code for a Tremolo-Bandpass 
//              multi effect on the Daisy platform. The idea is to apply 
//              a tremolo effect to only a selected frequency band, notated
//              with both high and low frequency parameters available
//              to the musican via potentiometers. 
//
//              This prototype is based on the example code provided by ElectroSmith: 
//              https://github.com/electro-smith/DaisyExamples/tree/master/pod/MultiEffect
//              While the effects aren't utilized, this code has functions for the rotary encoder, 
//              potentiometers and LED's that are useful.
//              
//              This protoype applies the built in Tremolo function available in daisysp
//
//              It also evaluates two types of bandpass filters:
//
//              1) State Variable Filter - based on the svf provided in daisysp
//
//              2) Bandpass derived from second order all pass - based on Fred Harris design,
//                 code and details explained by Tom Erbe here:   
//                 http://synthnotes.ucsd.edu/wp4/index.php/2019/11/09/second-order-allpass-filter/

//              To Do: 
//                      1) Build out circuit to include 5 potentiometers:
//                          TremoloRate, Tremolo Depth, LowFreq, HighFreq, Wet/Dry.
//                      2) Fine tune parameters for filters and tremolo. 
//                      3) Write a library function for the Bandpass Derived from a
//                         a Second Order All Pass and send to Electro Smith so they 
//                         can evaluate it for inclusion in the open source libraries [DONE]
//                      4) Documentation of parameters 

#include "daisysp.h"            // Open Source DSP Library
#include "daisy_pod.h"          // libDaisy, hardware abstraction library for
                                // the daisy platform

#include "soap.h"

#define PI 3.141592653589793

#define TRM 2   // Tremolo Test State. This is just a tremolo with depth and rate
                // LED's are pink in this state. POT1 is rate, POT2 is depth.
                // The parameters persist in other states, so you can dial in the
                // tremolo here and it will persist in the SAB state

#define BNP 1   // Band Pass SVF filter Test State. LED's are GREEN in this state.
                // A test of the built in SVF.
                // POT1 is center frequency, POT2 is q.

#define SAB 0   // Second Order All Pass Bandpass Filter + Tremolo Test State
                // This is our prototype. LED's are BLUE in this state.
                // POT1 is center frequency, POT2 is q. Tremolo parameters 
                // get tuned in the TRM state.

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

void GetTremoloSoapSample(float &outl, float &outr, float inl, float inr);

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
            case SAB: GetTremoloSoapSample(outl, outr, inl, inr); break;
                      
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

void GetTremoloSoapSample(float &outl, float &outr, float inl, float inr) 
{   
    float orig_ileft = inl;
      
    // Add in the tremolo
    inl = trem.Process(inl);

    // Add in the Soap Bandpass
    outl = soap.Process(inl);
    
    // blending dry signal with tremolo+bandpass signal
    outl = (float)(outl + 0.1*orig_ileft) / 2;          

    outr = (float)outl;
}





