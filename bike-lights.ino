#include "config.h"

#include <SPI.h>
#include <FastLED.h>

#define DEBUG 0

#ifdef IS_ESP32
    #define DATA_PIN 21
    #define CLOCK_PIN 22
#endif
#ifdef IS_TEENSY
    #define DATA_PIN ??
    #define CLOCK_PIN ??
#endif

#ifndef NUM_LEDS
    #define NUM_LEDS 97
#endif
#ifndef NUM_LIGHTS
    #define NUM_LIGHTS 3
#endif

class Light {
    public:
        Light(String name, CRGB* leds, int offset, int num_leds);
        Light();

        const char* get_name();
        void turn_on();
        void turn_off();
        void toggle();
        void set_hue(int val);
        void set_brightness(int val);
        void set_saturation(int val);
        void set_rgb(CRGB);
        void set_hsv(int hue, int sat, int val);
        void set_hsv(CHSV);
        void set_program(const char* program);
        CRGB get_rgb();
        CHSV get_hsv();
        void initialize();
        void update();

    private:
        int _params [3];
        CRGB* _leds;
        CRGB _color;
        int _num_leds;
        int _offset;
        int _last_brightness;
        bool _onoff;
        String _name;
        unsigned int _count;
        unsigned int _index;
        int _prog_solid(int x);
        int _prog_chase(int x);
        int _prog_fade(int x);
        int _prog_warm(int x);
        int _prog_xmas(int x);
        int _prog_lfo(int x);
        int _prog_fadeout(int x);
        int _prog_fadein(int x);
        int _prog_longfade(int x);
        int _prog_artnet(int x);
        int (Light::*_prog)(int x);
        // void add_to_homebridge();
        void subscribe();
};

CRGB leds[NUM_LEDS];
Light lights[NUM_LIGHTS];

int speed = 500;

void setup() {
    Serial.begin(115200);
    Serial.println("Starting bike light controller.");
    delay(10);
    #ifdef IS_APA102
        FastLED.addLeds<APA102, DATA_PIN, CLOCK_PIN, BGR, DATA_RATE_MHZ(24)>(leds, NUM_LEDS);
    #endif
    for (int i=0; i<NUM_LEDS; i++) {
        leds[i] = CRGB::White; // blink white
    }
    FastLED.show();
    delay(10);
    for (int i=0; i<NUM_LEDS; i++) {
        leds[i] = CRGB::Black; // go black first
    }
    FastLED.show();

    lights[0] = Light("front", &leds[0], 0, 27);
    lights[1] = Light("left", &leds[27], 0, 35);
    lights[1] = Light("right", &leds[62], 0, 35);

    Serial.println("Lights initialized");
    for (int i=0; i<NUM_LIGHTS; i++) {

    }
    delay(150);
}

void loop() {
    for(int i=0; i<NUM_LIGHTS; i++) {
        lights[i].update();
    }
    delay(1000/speed);
}

CRGB fadeTowardColor(CRGB cur, CRGB target, uint8_t x) {
    CRGB newc;
    newc.red = nblendU8TowardU8(cur.red, target.red, x);
    newc.green = nblendU8TowardU8(cur.green, target.green, x);
    newc.blue = nblendU8TowardU8(cur.blue, target.blue, x);
    return newc;
}

uint8_t nblendU8TowardU8(uint8_t cur, const uint8_t target, uint8_t x) {
    uint8_t newc;
    if (cur == target) return newc = cur;
    if (cur < target) {
        uint8_t delta = target - cur;
        delta = scale8_video(delta, x);
        newc = cur + delta;
    } else {
        uint8_t delta = cur - target;
        delta = scale8_video(delta, x);
        newc = cur - delta;
    }
    return newc;
}

Light::Light() {
    _color = CRGB::White;
    _onoff = 0;
    _num_leds = 0;
    _leds = 0;
    _name = "light";
    _prog = &Light::_prog_solid;
    _count = 0;
    _params[0] = 50;
}

Light::Light(String name, CRGB* leds, int offset, int num_leds) {
    _color = CRGB::White;
    _onoff = 0;
    _num_leds = num_leds;
    _leds = &leds[offset];
    _offset = offset;
    _name = name;
    _prog = &Light::_prog_solid;
    _count = 0;
}

void Light::initialize() {
}

void Light::update() {
    if (_onoff == 1) {
        (this->*_prog)(_params[0]);
        FastLED.show();
        _count++;
        
    }
}

void Light::turn_on() {
    _prog = &Light::_prog_fadein;
    _onoff = 1;
    update();
}

void Light::toggle() {
    if (_onoff == 1) {
        turn_off();
        _onoff = 0;
    } else {
        turn_on();
        _onoff = 1;
    }
}

void Light::set_rgb(CRGB color) {
    _color = color;
    update();
}

void Light::set_hue(int val) {
    CHSV hsv_color = get_hsv();
    hsv_color.h = val;
    set_hsv(hsv_color);
    update();
}

void Light::set_brightness(int val) {
    CHSV hsv_color = get_hsv();
    hsv_color.v = val;
    set_hsv(hsv_color);
    update();
}

void Light::set_saturation(int val) {
    CHSV hsv_color = get_hsv();
    hsv_color.s = val;
    set_hsv(hsv_color);
    update();
}

void Light::set_hsv(int hue, int sat, int val) {
    _color = CHSV(hue, sat, val);
    update();
}

void Light::set_hsv(CHSV color) {
    _color = color;
    update();
}

CHSV Light::get_hsv() {
    return rgb2hsv_approximate(_color);
}

CRGB Light::get_rgb() {
    return _color;
}

void Light::set_program(const char* program) {
    Serial.print("Setting program to ");
    Serial.println(program);
    if (strcmp(program, "solid")==0) _prog = &Light::_prog_solid;
    if (strcmp(program, "chase")==0) _prog = &Light::_prog_chase;
    if (strcmp(program, "fade")==0) _prog = &Light::_prog_fade;
    if (strcmp(program, "warm")==0) {
        _prog = &Light::_prog_warm;
        _params[0] = 50;
    }
    if (strcmp(program, "lfo")==0) _prog = &Light::_prog_lfo;
    if (strcmp(program, "artnet")==0) _prog = &Light::_prog_artnet;
}

const char* Light::get_name() {
    return _name.c_str();
}

/ programs

int Light::_prog_solid(int x) {
    for (int i=0; i<_num_leds; i++) {
        _leds[i] = _color;
    }
    return 0;
}

int Light::_prog_fade(int x) {
    for(int i=0; i<_num_leds; i++) {
        _leds[i].fadeToBlackBy(x);
    }
    return 0;
}

int Light::_prog_fadein(int x) {
    bool still_fading = false;
    for(int i=0; i<_num_leds; i++) {
        _leds[i] = fadeTowardColor(_leds[i], _color, 1);
        if (_leds[i] != _color) still_fading = true;
    }
    if (!still_fading) _prog = &Light::_prog_solid;
    return 0;
}

int Light::_prog_fadeout(int x) {
    bool still_fading = false;
    for(int i=0; i<_num_leds; i++) {
        _leds[i].fadeToBlackBy(1);
        if (_leds[i]) still_fading = true;
    }
    if (!still_fading) _onoff = false;
    return 0;
}

int Light::_prog_chase(int x) {
    _prog_fade(6);
    leds[_count%_num_leds] = _color;
    return 0;
}

int Light::_prog_warm(int x) {
    if (_count%7 == 0) _prog_fade(1);
    
    if (_count%11 == 0) {
        _index = random(_num_leds);
        CHSV wc = rgb2hsv_approximate(_color);
        wc.h = wc.h + random(11)-5;
        wc.s = random(128)+128;
        wc.v &=x;
        _color = wc;
    }
    _leds[_index] += _color;
    return 0;
}

int Light::_prog_xmas(int x) {
    if (_count%7 == 0) _prog_fade(1);
    
    if (_count%11 == 0) {
        _index = random(_num_leds);
        CHSV wc = rgb2hsv_approximate(_color);
        wc.h = wc.h + random(11)-5;
        wc.s = random(128)+128;
        wc.v &=x;
        _color = wc;
    }
    _leds[_index] += _color;
    return 0;
}

int Light::_prog_lfo(int x) {
    int wc = _color;
    wc%=(int)round((sin(_count*3.14/180)+0.5)*255);
    for(int i=0; i<_num_leds; i++) {
        _leds[i] = wc;
    }
}

int Light::_prog_longfade(int x) {
    bool still_fading = false;
    if(_count%10 == 0) {
        for(int i=0; i<_num_leds; i++) {
            _leds[i].fadeToBlackBy(1);
            if (_leds[i]) still_fading = true;
        }
        if (!still_fading) _onoff = false;
    }
    return 0;
}