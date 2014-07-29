//============================================================
//	Touch Sensing Demo on Spark Core
//
//============================================================
//	Copyright (c) 2014 Tangibit Studios LLC.  All rights reserved.
//
//	This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//	License as published by the Free Software Foundation, either
//	version 3 of the License, or (at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//	Lesser General Public License for more details.
//
//	You should have received a copy of the GNU Lesser General Public
//	License along with this program; if not, see <http://www.gnu.org/licenses/>.
//
//============================================================

// LED port
#define LED D7

// Polling times in [ms]
#define POLL_TIME 20

// Touch events
#define tEVENT_NONE 0
#define tEVENT_TOUCH 1
#define tEVENT_RELEASE 2

// touch sensor pins
    int sPin = D3;
    int rPin = D2;
    
// timestamps
    unsigned long tS;
    unsigned long tR;
    
// reading and baseline
    long tReading;
    long tBaseline;


//------------------------------------------------------------
//	Setup
//------------------------------------------------------------
void setup()
{
	// Lamp pins
	pinMode(LED, OUTPUT);
    
    // intialize conditions for touch sensor
    pinMode(sPin,OUTPUT);
    attachInterrupt(rPin,touchSense,RISING);
    
    // calibrate touch sensor- Keep hands off!!!
    tBaseline = touchSampling();    // initialize to first reading

	/*
	// Make sure your Serial Terminal app is closed before powering your Core
    Serial.begin(9600);
    // Now open your Serial Terminal, and hit any key to continue!
    while(!Serial.available()) SPARK_WLAN_Loop();
    */
    
}

//------------------------------------------------------------
//	Loop
//------------------------------------------------------------
void loop()
{
	// time stamps
	static unsigned long lastUpdate = 0;
	
	//Update every POLL_TIME [ms]
	if (millis() > lastUpdate + POLL_TIME)
	{
		// check Touch UI
		int touchEvent = touchEventCheck();
		
		if (touchEvent == tEVENT_TOUCH)
		{
			digitalWrite(LED, HIGH);
		}
		
		if (touchEvent == tEVENT_RELEASE)
		{
			digitalWrite(LED, LOW);
		}
		
		// time stamp updated
		lastUpdate = millis();
	}
}


//============================================================
//	Touch UI
//============================================================
//------------------------------------------------------------
// ISR for touch sensing
//------------------------------------------------------------
void touchSense()
{
    tR = micros();
}

//------------------------------------------------------------
// touch sampling
//
// sample touch sensor 32 times and get average RC delay [usec]
//------------------------------------------------------------
long touchSampling()
{
    long tDelay = 0;
    int mSample = 0;
    
    for (int i=0; i<32; i++)
    {
        // discharge capacitance at rPin
        pinMode(rPin, OUTPUT);
        digitalWrite(sPin,LOW);
        digitalWrite(rPin,LOW);
        
        // revert to high impedance input
        pinMode(rPin,INPUT);
        
        // timestamp & transition sPin to HIGH and wait for interrupt in a read loop
        tS = micros();
        tR = tS;
        digitalWrite(sPin,HIGH);
        do
        {
            // wait for transition
        } while (digitalRead(rPin)==LOW);
        
        // accumulate the RC delay samples
        // ignore readings when micros() overflows
        if (tR>tS)
        {
            tDelay = tDelay + (tR - tS);
            mSample++;
        }
        
    }
    
    // calculate average RC delay [usec]
    if (mSample>0)
    {
        tDelay = tDelay/mSample;
    }
    else
    {
        tDelay = 0;     // this is an error condition!
    }

    //autocalibration using exponential moving average on data below trigger point
    if (tDelay<(tBaseline + tBaseline/4))
    {
        tBaseline = tBaseline + (tDelay - tBaseline)/8;
    }
	
	/*
	Serial.println(tDelay, tBaseline);
	*/
    
    return tDelay;
    
}

//------------------------------------------------------------
// touch event check
//
// check touch sensor for events:
//      tEVENT_NONE     no change
//      tEVENT_TOUCH    sensor is touched (Low to High)
//      tEVENT_RELEASE  sensor is released (High to Low)
//
//------------------------------------------------------------
int touchEventCheck()
{
    int touchSense;                     // current reading
    static int touchSenseLast = LOW;    // last reading
    
    static unsigned long touchDebounceTimeLast = 0; // debounce timer
    int touchDebounceTime = 50;                     // debounce time
    
    static int touchNow = LOW;  // current debounced state
    static int touchLast = LOW; // last debounced state
    
    int tEvent = tEVENT_NONE;   // default event
    
    
    // read touch sensor
    tReading = touchSampling();
    
    // touch sensor is HIGH if trigger point 1.25*Baseline
    if (tReading>(tBaseline + tBaseline/4)) 
    {
        touchSense = HIGH; 
    }
    else
    {
        touchSense = LOW; 
    }
    
    // debounce touch sensor
    // if state changed then reset debounce timer
    if (touchSense != touchSenseLast)
    {
        touchDebounceTimeLast = millis();
    }
    
    touchSenseLast = touchSense;
    
    
    // accept as a stable sensor reading if the debounce time is exceeded without reset
    if (millis() > touchDebounceTimeLast + touchDebounceTime)
    {
        touchNow = touchSense;
    }
    
    
    // set events based on transitions between readings
    if (!touchLast && touchNow)
    {
        tEvent = tEVENT_TOUCH;
    }
    
    if (touchLast && !touchNow)
    {
        tEvent = tEVENT_RELEASE;
    }
    
    
    // update last reading
    touchLast = touchNow;
    
    return tEvent;
}