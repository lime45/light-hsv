#include <Adafruit_NeoPixel.h>
#include <SoftwareSerial.h>

#define PIN 5
#define PIN2 6

uint8_t red =0;
uint8_t green=255;
uint8_t blue =0;

typedef enum {
    NORMAL_STATE,
    COMMAND_STATE
} state_t;
state_t state = NORMAL_STATE;

const uint8_t SET_LIGHT_MODE = 0x01;
const uint8_t RAINBOW_MODE   = 0x02;
const uint8_t TEAM_MODE      = 0x03;
const uint8_t GLITTER_MODE   = 0x04;

int jump=0;
uint8_t degreeold=0;
uint8_t oldlocation=0; 
#define SensorPin1      0
#define filterSamples   3              // filterSamples should  be an odd number, no smaller than 3
int sensSmoothArray1 [filterSamples];   // array for holding raw sensor values for sensor1 

int rawData1, smoothData1;  // variables for sensor1 data

Adafruit_NeoPixel strip = Adafruit_NeoPixel(120, PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel strip2 = Adafruit_NeoPixel(120, PIN2, NEO_GRB + NEO_KHZ800);

SoftwareSerial softserial(A4, A3);

void setup() {
    softserial.begin(9600); //setup Serial
    Serial.begin(9600);
    // Serial.setTimeout(10);

    //Setup Strip 1
    strip.begin();
    strip.show(); // Initialize all pixels to 'off'
    strip.setBrightness(255); //Setup global brightness

    //Setup Strip 2
    strip2.begin();
    strip2.show(); // Initialize all pixels to 'off'
    strip2.setBrightness(255); //Setup global brightness

    // initialize serial communication with computer:
    Serial.begin(9600);      
    Serial.println("Initialized.");    
}

void loop() {
    if(softserial.available() && state==NORMAL_STATE) {
        uint8_t degree  = softserial.read(); //incoming serial stream
        if (degree== 255) {
            state = COMMAND_STATE;
            Serial.println("Received COMMAND_STATE byte.");
        }
        if(state==NORMAL_STATE) {
            degree=(degree/2);
            rawData1 = degree;                       // read sensor 1
            degree = digitalSmooth(rawData1, sensSmoothArray1);  // every sensor you use with digitalSmooth needs its own array

            if(degree>degreeold) {
                jump= ((degree - degreeold)/10) +1;
                for(int i= degreeold; i <= degree; i+=jump) {
                    colorSeg(strip.Color(red,green,blue),  i, 5);
                }
            }
            
            if(degree<degreeold) {
                jump= ((degreeold -degree)/10) +1;
                for(int i= degreeold; i >= degree; i-=jump) {
                    colorSeg(strip.Color(red,green,blue),  i, 5);
                }
            }
            
            degreeold=degree;
        }
        
        if(state==COMMAND_STATE) {
            while(!softserial.available()) {
                // Spin here while we wait for more bytes
            }
            uint8_t infeed  = softserial.read(); //incoming serial stream
            if (infeed == SET_LIGHT_MODE) {
                Serial.println("Received SET_LIGHT_MODE byte.");
                while(!softserial.available()) {
                    // Spin here while we wait for more bytes
                }
                red  = softserial.read();
                Serial.print("r:");
                Serial.print(red);
                while(!softserial.available()) {
                    // Spin here while we wait for more bytes
                }
                green  = softserial.read();
                Serial.print(",g:");
                Serial.print(green);
                while(!softserial.available()) {
                    // Spin here while we wait for more bytes
                }
                blue  = softserial.read();
                Serial.print(",b:");
                Serial.println(blue);
                state=NORMAL_STATE;
            }
            else if(infeed == RAINBOW_MODE) {
                Serial.println("Received RAINBOW_MODE byte.");
                rainbow(10);
                turnoff();
                softserial.flush();
                strip.show();
                strip2.show();
                Serial.println("Returning to NORMAL_STATE.");
                state=NORMAL_STATE;
            }
            else if(infeed == TEAM_MODE) {
                // TODO: Add Team Mode
                Serial.println("Received TEAM_MODE byte.");
                Serial.println("Returning to NORMAL_STATE.");
                state=NORMAL_STATE;
            }
            else if(infeed == GLITTER_MODE) {
                // TODO: Add Glitter Mode
                Serial.println("Received GLITTER_MODE byte.");
                Serial.println("Returning to NORMAL_STATE.");
                state=NORMAL_STATE;
            }
            else {
                Serial.print("Returning to normal state, value:");
                Serial.println(infeed);
                state=NORMAL_STATE;
            }
        }
    }
}

// "int *sensSmoothArray" passes an array to the function - the asterisk indicates the array name is a pointer
int digitalSmooth(int rawIn, int *sensSmoothArray)
{     
    int j, k, temp, top, bottom;
    long total;
    static int i;
    // static int raw[filterSamples];
    static int sorted[filterSamples];
    boolean done;

    i = (i + 1) % filterSamples;    // increment counter and roll over if necc. -  % (modulo operator) rolls over variable
    sensSmoothArray[i] = rawIn;                 // input new data into the oldest slot

    // Serial.print("raw = ");

    for (j=0; j<filterSamples; j++){     // transfer data array into anther array for sorting and averaging
        sorted[j] = sensSmoothArray[j];
    }

    done = 0;                // flag to know when we're done sorting              
    while(done != 1){        // simple swap sort, sorts numbers from lowest to highest
        done = 1;
        for (j = 0; j < (filterSamples - 1); j++){
            if (sorted[j] > sorted[j + 1]){     // numbers are out of order - swap
                temp = sorted[j + 1];
                sorted [j+1] =  sorted[j] ;
                sorted [j] = temp;
                done = 0;
            }
        }
    }


    // throw out top and bottom 15% of samples - limit to throw out at least one from top and bottom
    bottom = max(((filterSamples * 15)  / 100), 1); 
    top = min((((filterSamples * 85) / 100) + 1  ), (filterSamples - 1));   // the + 1 is to make up for asymmetry caused by integer rounding
    k = 0;
    total = 0;
    for ( j = bottom; j< top; j++){
        total += sorted[j];  // total remaining indices
        k++;
    }

    return total / k;    // divide by number of samples
}

void turnoff() {
    for(uint16_t i=0; i<120; i++) {
        strip.setPixelColor(i, strip.Color(0,0,0));
        strip2.setPixelColor(i, strip.Color(0,0,0));
    }
}

//short segment of moving color
void colorSeg(uint32_t color, uint8_t location, uint32_t seg) {

    for(int n=oldlocation; n<oldlocation+seg; n++) {
        strip.setPixelColor(n, strip.Color(0,0,0));
        strip2.setPixelColor(n, strip.Color(0,0,0));
    }
    for(uint16_t i=location; i<location+seg; i++) {
        strip.setPixelColor(i, color);
        strip2.setPixelColor(i, color);
    }
    strip.show();
    //for(uint16_t i=location; i<location+seg; i++) {
    //  }
    strip2.show();

    oldlocation=location;
}

void rainbow(uint8_t wait) {
    uint16_t i, j;

    for(j=0; j<256; j++) {
        for(i=0; i<strip.numPixels(); i++) {
            strip.setPixelColor(i, Wheel((i+j) & 255));
            strip2.setPixelColor(i, Wheel((i+j) & 255));
        }
        strip.show();
        strip2.show();
        delay(wait);
    }
}

uint32_t Wheel(byte WheelPos) {
    WheelPos = 255 - WheelPos;
    if(WheelPos < 85) {
        return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
    } else if(WheelPos < 170) {
        WheelPos -= 85;
        return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
    } else {
        WheelPos -= 170;
        return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    }
}
