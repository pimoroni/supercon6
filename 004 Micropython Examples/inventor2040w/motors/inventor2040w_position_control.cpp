#include <cstdio>
#include "pico/stdlib.h"

#include "inventor.hpp"
#include "pid.hpp"

/*
An example of how to move a motor smoothly between random positions,
with the help of it's attached encoder and PID control.

Press "User" to exit the program.
*/

using namespace inventor;

// The gear ratio of the motor
constexpr float GEAR_RATIO = 50.0f;

// The direction to spin the motor in. NORMAL_DIR (0), REVERSED_DIR (1)
const Direction DIRECTION = NORMAL_DIR;

// The scaling to apply to the motor's speed to match its real-world speed
constexpr float SPEED_SCALE = 5.4f;

// How many times to update the motor per second
const uint UPDATES = 100;
constexpr float UPDATE_RATE = 1.0f / (float)UPDATES;

// The time to travel between each random value
constexpr float TIME_FOR_EACH_MOVE = 1.0f;
const uint UPDATES_PER_MOVE = TIME_FOR_EACH_MOVE * UPDATES;

// How many of the updates should be printed (i.e. 2 would be every other update)
const uint PRINT_DIVIDER = 4;

// Multipliers for the different printed values, so they appear nicely on the Thonny plotter
constexpr float SPD_PRINT_SCALE = 20.0f;    // Driving Speed multipler

// How far from zero to move the motor, in degrees
constexpr float POSITION_EXTENT = 180.0f;

// The interpolating mode between setpoints. STEP (0), LINEAR (1), COSINE (2)
const uint INTERP_MODE = 2;


// PID values
constexpr float POS_KP = 0.14f;   // Position proportional (P) gain
constexpr float POS_KI = 0.0f;    // Position integral (I) gain
constexpr float POS_KD = 0.002f;  // Position derivative (D) gain


// Create a new Inventor2040W
Inventor2040W board(GEAR_RATIO);

// Create PID object for position control
PID pos_pid = PID(POS_KP, POS_KI, POS_KD, UPDATE_RATE);


int main() {
  stdio_init_all();

  // Attempt to initialise the board
  if(board.init()) {

    // Access the motor and encoder from Inventor
    Motor& m = board.motors[MOTOR_A];
    Encoder& enc = board.encoders[MOTOR_A];

    // Set the motor's speed scale
    m.speed_scale(SPEED_SCALE);

    // Set the motor and encoder's direction
    m.direction(DIRECTION);
    enc.direction(DIRECTION);

    // Enable the motor
    m.enable();


    uint update = 0;
    uint print_count = 0;

    // Set the initial value and create a random end value between the extents
    float start_value = 0.0f;
    float end_value = (((float)rand() / (float)RAND_MAX) * (POSITION_EXTENT * 2.0f)) - POSITION_EXTENT;

    // Continually move the motor until the user button is pressed
    while(!board.switch_pressed()) {

      // Capture the state of the encoder
      Encoder::Capture capture = enc.capture();

      // Calculate how far along this movement to be
      float percent_along = (float)update / (float)UPDATES_PER_MOVE;

      switch(INTERP_MODE) {
      case 0:
        // Move the motor instantly to the end value
        pos_pid.setpoint = end_value;
        break;

      case 2:
        // Move the motor between values using cosine
        pos_pid.setpoint = (((-cosf(percent_along * (float)M_PI) + 1.0) / 2.0) * (end_value - start_value)) + start_value;
        break;

      case 1:
      default:
        // Move the motor linearly between values
        pos_pid.setpoint = (percent_along * (end_value - start_value)) + start_value;
      }

      // Calculate the velocity to move the motor closer to the position setpoint
      float vel = pos_pid.calculate(capture.degrees(), capture.degrees_per_second());

      // Set the new motor driving speed
      m.speed(vel);

      // Print out the current motor values and their setpoints, but only on every multiple
      if(print_count == 0) {
        printf("Pos = %f, ", capture.degrees());
        printf("Pos SP = %f, ", pos_pid.setpoint);
        printf("Speed = %f\n", m.speed() * SPD_PRINT_SCALE);
      }

      // Increment the print count, and wrap it
      print_count = (print_count + 1) % PRINT_DIVIDER;

      update++;   // Move along in time

      // Have we reached the end of this movement?
      if(update >= UPDATES_PER_MOVE) {
        update = 0;  // Reset the counter

        // Set the start as the last end and create a new random end value
        start_value = end_value;
        end_value = (((float)rand() / (float)RAND_MAX) * (POSITION_EXTENT * 2.0f)) - POSITION_EXTENT;
      }

      sleep_ms(UPDATE_RATE * 1000.0f);
    }

    // Disable the motor
    m.disable();
  }
}
