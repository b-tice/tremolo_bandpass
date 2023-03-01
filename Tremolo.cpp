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
//                         can include it in the open source libraries 
//                      4) Documentation of parameters 

#include "daisysp.h"            // Open Source DSP Library
#include "daisy_pod.h"          // libDaisy, hardware abstraction library for
                                // the daisy platform

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
static Parameter                                 deltime, cutoffParam, crushrate;
int                                              mode = SAB;

// for Bandpass derived from Second Order All Pass Filter
// note, x_n and y_n's are from difference eqn.
float soap_center_freq =        400.0;
float soap_bandwidth =          50.0;
float in_0 =                    0.0;          // input            x0
float din_1 =                   0.0;          // delayed input    x1
float din_2 =                   0.0;          // delayed input    x2
float dout_1 =                  0.0;          // delayed output   y1
float dout_2 =                  0.0;          // delayed output   y2
double all_output =              0.0;          // all pass output  y0
float sr;


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

    //Inits and sample rate
    pod.Init();
    pod.SetAudioBlockSize(4);
    sample_rate = pod.AudioSampleRate();
    sr = sample_rate;                       // used for soap coefficient calculations
    trem.Init(sample_rate);
    filt.Init(sample_rate);
    tone.Init(sample_rate);

    //set parameters
    cutoffParam.Init(pod.knob1, 500, 20000, cutoffParam.LOGARITHMIC);
    crushrate.Init(pod.knob2, 1, 50, crushrate.LOGARITHMIC);

    //tremolo parameters
    trem.SetFreq(2);
    trem.SetDepth(0.75);

    // SVF bandpass filter settings
    filt.SetFreq(300.0);
    filt.SetRes(0.85);
    filt.SetDrive(0.8);

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
            soap_center_freq = (k1*1000);
            soap_bandwidth = (k2*100.0);
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

    // This is the mapping of variables from the original example code to this code:
    /*  float in_0 = 0.0;       // input            x0
        float din_1 = 0.0;      // delayed input    x1
        float din_2 = 0.0;      // delayed input    x2
        float dout_1 = 0.0;     // delayed output   y1
        float dout_2 = 0.0;     // delayed output   y2
        float all_output = 0.0; // all pass output  y0 */

    // recalculate the coefficients, later move this to a lookup table
    double d = -cos(2.0 * PI * (soap_center_freq/sr));
    double tf = tan(PI * (soap_bandwidth/sr));                     // tangent bandwidth   
    double c = (tf - 1.0)/(tf + 1.0);                              // coefficient     

    float orig_ileft = inl;
    
    // *******
    // Add in the tremolo
    inl = trem.Process(inl);
    inr = inl;
    // *******

    in_0 = inl;   
    
    all_output = -c*in_0 + (d - d*c)*din_1 + din_2 - (d - d*c)*dout_1 + c*dout_2;

    // move samples in delay for next sample
    din_2 = din_1;
    din_1 = in_0;
    dout_2 = dout_1;
    dout_1 = all_output;
    outl = (in_0 + all_output * -1.0) * 0.5;            // make factor -1.0 to create a bandpass

    outl = (float)(outl + 0.1*orig_ileft) / 2;          // blending dry signal with tremolo+bandpass signal

    outr = (float)outl;

}





