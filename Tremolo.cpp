// Brian Tice
//
// Tremolo + Bandpass Project
//

#include "daisysp.h"            // Open Source DSP Library
#include "daisy_pod.h"          // libDaisy, hardware abstraction library for
                                // the daisy platform

#define PI 3.141592653589793

// Set max delay time to 0.75 of samplerate.
#define MAX_DELAY static_cast<size_t>(48000 * 2.5f)
#define REV 5
#define DEL 4
#define TRM 2   // Tremolo Test State
#define CRU 3
#define BNP 1   // Band Pass SVF filter Test State
#define SAB 0   // Second Order All Pass Bandpass Filter

using namespace daisysp;
using namespace daisy;

static DaisyPod pod;

static Tremolo                                   trem;
static Svf                                       filt;

static ReverbSc                                  rev;
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS dell;
static DelayLine<float, MAX_DELAY> DSY_SDRAM_BSS delr;
static Tone                                      tone;
static Parameter deltime, cutoffParam, crushrate;
int              mode = REV;

float currentDelay, feedback, delayTarget, cutoff;

int   crushmod, crushcount;
float crushsl, crushsr, drywet;

// for Soap Filter
float soap_cutoff = 400.0;
float bw = 50.0;
float in_0 = 0.0;           // input            x0
float din_1 = 0.0;          // delayed input    x1
float din_2 = 0.0;          // delayed input    x2
float dout_1 = 0.0;         // delayed output   y1
float dout_2 = 0.0;         // delayed output   y2
float all_output = 0.0;     // all pass output  y0
float d;
float tf;           // tangent bandwidth   
float c;            // coefficient     
float sr;


//Helper functions
void Controls();

void GetReverbSample(float &outl, float &outr, float inl, float inr);

void GetDelaySample(float &outl, float &outr, float inl, float inr);

void GetCrushSample(float &outl, float &outr, float inl, float inr);

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
            case REV: GetReverbSample(outl, outr, inl, inr); break;
            case DEL: GetDelaySample(outl, outr, inl, inr); break;
            case CRU: GetCrushSample(outl, outr, inl, inr); break;
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

    //Inits and sample rate
    pod.Init();
    pod.SetAudioBlockSize(4);
    sample_rate = pod.AudioSampleRate();
    sr = sample_rate;
    rev.Init(sample_rate);
    trem.Init(sample_rate);
    filt.Init(sample_rate);
    dell.Init();
    delr.Init();
    tone.Init(sample_rate);

    //set parameters
    deltime.Init(pod.knob1, sample_rate * .05, MAX_DELAY, deltime.LOGARITHMIC);
    cutoffParam.Init(pod.knob1, 500, 20000, cutoffParam.LOGARITHMIC);
    crushrate.Init(pod.knob2, 1, 50, crushrate.LOGARITHMIC);

    //reverb parameters
    rev.SetLpFreq(18000.0f);
    rev.SetFeedback(0.85f);

    //tremolo parameters
    trem.SetFreq(2);
    trem.SetDepth(0.75);

    //bandpass filter settings
    filt.SetFreq(300.0);
    filt.SetRes(0.85);
    filt.SetDrive(0.8);

    //Soap filter settings
    //d = -cos(2.0 * PI * (soap_cutoff/sample_rate));
    //tf = tan(PI * (bw/sample_rate));      // tangent bandwidth   
    //c = (tf - 1.0)/(tf + 1.0);            // coefficient     


    //delay parameters
    currentDelay = delayTarget = sample_rate * 0.75f;
    dell.SetDelay(currentDelay);
    delr.SetDelay(currentDelay);

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
        case REV:
            drywet = k1;
            rev.SetFeedback(k2);
            break;
        case DEL:
            delayTarget = deltime.Process();
            feedback    = k2;
            break;
        case CRU:
            cutoff = cutoffParam.Process();
            tone.SetFreq(cutoff);
            crushmod = (int)crushrate.Process();
        case TRM:
            trem.SetFreq(k1*3);
            trem.SetDepth(k2);
        case BNP:
            filt.SetFreq(k1*3000);
        case SAB:
            soap_cutoff = (k1*3000);
            bw = (k2*100.0);
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
    delayTarget = feedback = drywet = 0;

    pod.ProcessAnalogControls();
    pod.ProcessDigitalControls();

    UpdateKnobs(k1, k2);

    UpdateEncoder();

    UpdateLeds(k1, k2);
}

void GetReverbSample(float &outl, float &outr, float inl, float inr)
{
    rev.Process(inl, inr, &outl, &outr);
    outl = drywet * outl + (1 - drywet) * inl;
    outr = drywet * outr + (1 - drywet) * inr;
}

void GetTremoloSample(float &outl, float &outr, float inl, float inr)
{
    outl = trem.Process(inl);
    //outr = trem.Process(inr);
    outr = outl;
}

void GetBandPassSample(float &outl, float &outr, float inl, float inr)
{

    filt.Process(inl);
    outl = filt.Band();
    //filt.Process(outl);
    //outr = filt.Band();
    outr = outl;

}

void GetSoapSample(float &outl, float &outr, float inl, float inr) 
{

    /*  float in_0 = 0.0;       // input            x0
        float din_1 = 0.0;      // delayed input    x1
        float din_2 = 0.0;      // delayed input    x2
        float dout_1 = 0.0;     // delayed output   y1
        float dout_2 = 0.0;     // delayed output   y2
        float all_output = 0.0; // all pass output  y0 */

    // recalculate the coefficients, later move this to a lookup table
    float d = -cos(2.0 * PI * (soap_cutoff/sr));
    float tf = tan(PI * (bw/sr));                     // tangent bandwidth   
    float c = (tf - 1.0)/(tf + 1.0);                  // coefficient     

    float orig_ileft = inl;
    float orig_iright = inr;

    // *******
    // Add in the tremolo
    inl = trem.Process(inl);
    //outr = trem.Process(inr);
    inr = inl;
    // *******

    in_0 = inl;   
    //outl = -c*in_0 + (d - d*c)*din_1 + din_2 - (d - d*c)*dout_1 + c*dout_2;
    all_output = -c*in_0 + (d - d*c)*din_1 + din_2 - (d - d*c)*dout_1 + c*dout_2;

    // move samples in delay for next sample
    din_2 = din_1;
    din_1 = in_0;
    dout_2 = dout_1;
    dout_1 = all_output;
    outl = (in_0 + all_output * -1.0) * 0.5;    // make factor -1.0 to create a bandpass

    outl = (outl + 0.1*orig_ileft) / 2;     // blending dry signal with tremolo+bandpass signal

    outr = outl;

}


void GetDelaySample(float &outl, float &outr, float inl, float inr)
{
    fonepole(currentDelay, delayTarget, .00007f);
    delr.SetDelay(currentDelay);
    dell.SetDelay(currentDelay);
    outl = dell.Read();
    outr = delr.Read();

    dell.Write((feedback * outl) + inl);
    outl = (feedback * outl) + ((1.0f - feedback) * inl);

    delr.Write((feedback * outr) + inr);
    outr = (feedback * outr) + ((1.0f - feedback) * inr);
}

void GetCrushSample(float &outl, float &outr, float inl, float inr)
{
    crushcount++;
    crushcount %= crushmod;
    if(crushcount == 0)
    {
        crushsr = inr;
        crushsl = inl;
    }
    outl = tone.Process(crushsl);
    outr = tone.Process(crushsr);
} 


