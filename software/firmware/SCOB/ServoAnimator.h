#ifndef ServoAnimator_h
#define ServoAnimator_h

#include <stdlib.h>
#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <WProgram.h>
#include <wiring.h>
#endif

#include <Servo.h>
#include "SimpleBezier.h"

struct ANIMATION {
    uint8_t numFrames;
    byte *frames;
    unsigned long *durations;
};


class ServoAnimator {
private:
    struct SERVO {
        uint8_t pin;
        uint8_t center;
        uint8_t startPos;
        uint8_t targetPos;
        uint8_t pos;
        Servo *servo;
    };

    uint8_t _numServos;
    SERVO *_servos; // array of servo structures

    // how long to take in ms to get to targetPos from startPos
    unsigned long _moveDuration = 250;

    // millis() value at the start of the move
    unsigned long _moveStartedAt = 0;

    // move status flag - true when moving, cleared by update()
    boolean _busy = false;

    ANIMATION * _animation;

    uint8_t _animFrame = 0;
    uint8_t _repeatCount = 0;
    boolean _reverse = false; // set to true to reverse animation
    float _speed = 1.0;  // multiplier applied to animation timings

public:

    ServoAnimator (uint8_t numServos);
    void initServo(uint8_t num, uint8_t pin, uint8_t center);
    void setServoCenter(uint8_t num, uint8_t center);

    void moveServosTo(const byte keyframe[], unsigned long dur);
    void setAnimation(ANIMATION& animation, boolean reverse = false);
    void setRepeatCount(uint8_t repeatCount);
    void setSpeed(float speed);

    boolean isBusy();

    void update();
    boolean moveToFrame(uint8_t frame); // returns false when done
    void nextFrame();

    void stop();

};

#endif
