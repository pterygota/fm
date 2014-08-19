//define composition by rewriting score class as desired,
//compile and then run binary to output wav file set in main
#include <stdio.h>
#include <cstdlib>
#include <string.h>
#include <math.h>
#include <sndfile.h>
#include <time.h>
#include <ctime>
#include <cmath>

using namespace std;

int SR = 44100; //sample rate
int maxA = 30000; //max amplitude
int glbTm = 0; //current global frame

//noise generator
class noise {
private:
  float random;
  int now;
public:
  void Start() {
	srand(time(0));
  }

  double Out() {
	random = (((double)rand() / RAND_MAX));
	return random;
  }
};

//sin oscillator with frequency modulator input
class osc {
private:
  double cf;

public:
  void Start() {
	cf = 0;
  }

  void SetCF(double freq) {
	cf = freq;
  }

  double Sine(double freq, double mods) {
	if(freq != 0) {
	  cf = freq;
	}

	return sin((2 * M_PI * (cf/SR) * glbTm) + mods);
  }
};

//stereo mixer with amplitude and pan modulation
class mixer {
private:
  double In1; //input 1
  double A1; //amplitude 1
  double P1; //pan 1

  double In2;
  double A2;
  double P2;

  double In3;
  double A3;
  double P3;

  double L1; //left channel level 1
  double L2;
  double L3;

  double R1; //right channel level 1
  double R2;
  double R3;

public:

  //inputs are patched in here
  void In(double input1, double input2, double input3) {
	In1 = input1;
	In2 = input2;
	In3 = input3;
  }

  //amplitude can be set or modulated here
  void Sliders(double sl1, double sl2, double sl3) {
	A1 = sl1;
	A2 = sl2;
	A3 = sl3;
  }
  //set output pan for inputs
  void Pan(double knob1, double knob2, double knob3) {
	P1 = knob1;
	P2 = knob2;
	P3 = knob3;
  }

  //stereo output, left channel
  double OutL() {
	if(P1 == 0) {
	  L1 = 1;
	}

	if(P1 < 0) {
	  L1 = 1;
	}

	if(P1 > 0) {
	  L1 = 1 - P1;
	}

	if(P2 == 0) {
	  L2 = 1;
	}

	if(P2 < 0) {
	  L2 = 1;
	}

	if(P2 > 0) {
	  L2 = 1 - P2;
	}

	if(P3 == 0) {
	  L3 = 1;
	}
	if(P3 < 0) {
	  L3 = 1;
	}

	if(P3 > 0) {
	  L3 = 1 - P3;
	}
	return ((((In1 * A1 * L1)) + (In2 * A2 * L2) + (In3 * A3 * L3))/3);
  }

  //stereo output, right channel
  double OutR() {
	if(P1 == 0) {
	  R1 = 1;
	}

	if(P1 < 0) {
	  R1 = 1 + P1;
	}

	if(P1 > 0) {
	  R1 = 1;
	}

	if(P2 == 0) {
	  R2 = 1;
	}

	if(P2 < 0) {
	  R2 = 1 + P2;
	}

	if(P2 > 0) {
	  R2 = 1;
	}

	if(P3 == 0) {
	  R3 = 1;
	}

	if(P3 < 0) {
	  R3 = 1 + P3;
	}

	if(P3 > 0) {
	  R3 = 1;
	}
	return (((In1 * A1 * R1) + (In2 * A2 * R2) + (In3 * A3 * R3))/3);
  }

  short StereoBufferOut(int which) {
	if(which == 0) {
	  return (short)(OutL() * maxA);
	}
	if(which == 1) {
	  return (short)(OutR() * maxA);
	}
  }

  short BuffOut() {
	return (short)(maxA * ((In1 * A1)/3 + (In2 * A2)/3 + (In3 * A3)/3));
  }

  double Out() {
	return In1 * A1 + In2 * A2 + In3 * A3;
  }
};

//sample and hold
class SnH {
private:
  double hv;
public:
  void Sample(double input) {
	hv = input;
  }
  double Out() {
	return hv;
  }
};

//adsr envelope generator
class adsr {
private:
  int A; //time at which attack phase is to begin
  int R; //time at which Release phase is to begin
  int Ad; //duration of attack phase
  int Dd; //duration of decay phase
  double Sl; //level of sustain phase
  int Rd; //duration of release phase
  double Mv; //the output value
  int Switch; //current phase, A=0 - R=3
  int n; //current frame [0 - duration]
  int W; //are we
  int now;
public:
  //force off
  void Off() {
	W = 0;
	Mv = 0;
	n = 0;
  }
  //set values for durations and level of sustain
  void Set(double AttackD, double DecayD, double SustainL, double ReleaseD) {
	Ad = SR * AttackD;
	Dd = SR * DecayD;
	Sl = SustainL;
	Rd = SR * ReleaseD;
  }
  //start the envelope, or restart
  void Trigger() {
	if(W == 1) {
	  Switch = 0;
	  n = (int)(Mv * Ad);
	}
	if(W == 0) {
	  W = 1;
	  A = glbTm;
	  R = 0;
	  Mv = 0;
	  n = 0;
	  Switch = 0;
	}
  }
  //force into release phase
  void Release() {
	R = glbTm;
  }
  //increment the frame, set the output value
  double Gen() {
	now = glbTm;
	//attack
	if(n <= Ad && Switch == 0 && W == 1) {
	  Mv = n/(double)Ad;
	  n++;
	}
	//switch to decay
	if(n == Ad && Switch == 0 && W == 1) {
	  n = 0;
	  Switch = 1;
	}
	//decay
	if(n < Dd && Switch == 1 && W == 1) {
	  Mv = 1 - ((n/(double)Dd) * (1.0 - (double)Sl));
	  n++;
	}
	//switch to sustain
	if(n == Dd && Switch == 1) {
	  n = 0;
	  Switch = 2;
	}
	//sustain
	if(Switch == 2 && R == 0) {
	  Mv = Sl;
	}
	//switch to release
	if(Switch == 2 && R != 0) {
	  n = 0;
	  Switch = 3;
	}
	//release
	if(n < Rd && Switch == 3) {
	  Mv = Sl - (((n + 1.0)/Rd) * Sl);
	  n++;
	}
	if(n >= Rd && Switch == 3) {
	  Mv = 0;
	  W = 0;
	}
	if( W == 0) {
	  Mv = 0;
	  n = 0;
	}
  }
  //return the output value, generate a new one if necessary
  double Out() {
	if(now == glbTm) {
	  return Mv;
	}
	if(now != glbTm) {
	  Gen();
	  return Mv;
	}
  }
};

//the score class is where everything is wired together
class score {
public:
  double totalD;
  int channels, totalF, totalS;
  mixer mix1;
  osc osc1;
  adsr env1;
  void Start() {
	totalD = 15;
	channels = 2;
	totalF = SR * totalD;
	totalS = channels * totalF;
	osc1.Start();
	osc1.SetCF(400);
  }
  //what to do, when
  void Inc() {
	mix1.In(osc1.Sine(0, 0), 0, 0);
	mix1.Sliders(0.8, 0.8, 0.8);
	mix1.Pan(0, 0, 0);
  }
};

//set up the score, malloc buffer, increment, output to wav file
int main() {
	int nchan = 2;
	score scr1;
	scr1.Start();
	int dataSize = scr1.totalS * sizeof(short);
	short *buffer = (short *)malloc(dataSize);
	int i;
	for(i = 0; i + 1 < scr1.totalF ; i++ ) {
	  scr1.Inc();
	  buffer[2 * i] = scr1.mix1.StereoBufferOut(0);
	  buffer[(2 * i) + 1] = scr1.mix1.StereoBufferOut(1);
	  glbTm++;
	}
	SF_INFO info;
	info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
	info.channels = scr1.channels;
	info.samplerate = SR;
	SNDFILE *sndFile = sf_open("FM2014.wav", SFM_WRITE, &info);
	//long writtenFrames = sf_writef_short(sndFile, buffer, scr1.totalF);
	sf_write_sync(sndFile);
	sf_close(sndFile);
	free(buffer);
	exit(EXIT_SUCCESS);
}
