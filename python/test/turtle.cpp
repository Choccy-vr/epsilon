#include <quiz.h>

#include "execution_environment.h"

// TODO: to be completed

QUIZ_CASE(python_turtle) {
#ifndef PLATFORM_WINDOWS
  TestExecutionEnvironment env = init_environement();
  assert_command_execution_fails(env, "forward(3)");
  assert_command_execution_succeeds(env, "from turtle import *");
  assert_command_execution_succeeds(env, "reset()");
  assert_command_execution_succeeds(env, "forward(10)");
  assert_command_execution_succeeds(env, "backward(5)");
  assert_command_execution_succeeds(env, "right(10)");
  assert_command_execution_succeeds(env, "left(5)");
  assert_command_execution_succeeds(env, "circle(28)");
  assert_command_execution_succeeds(env, "speed(28)");
  assert_command_execution_succeeds(env, "goto(28,28)");
  assert_command_execution_succeeds(env, "position()", "(28.0, 28.0)\n");
  assert_command_execution_succeeds(env, "setheading(28)");
  assert_command_execution_succeeds(env, "heading()", "28.0\n");
  assert_command_execution_succeeds(env, "pendown()");
  assert_command_execution_succeeds(env, "penup()");
  assert_command_execution_succeeds(env, "pensize(2)");
  assert_command_execution_succeeds(env, "isdown()", "False\n");
  assert_command_execution_succeeds(env, "showturtle()");
  deinit_environment();
#endif
}

QUIZ_CASE(python_turtle_circle) {
#ifndef PLATFORM_WINDOWS
  // Turtle position should be unchanged after a complete circle
  TestExecutionEnvironment env = init_environement();
  assert_command_execution_succeeds(env, "from turtle import *");
  assert_command_execution_succeeds(env, "goto(0,0)");
  assert_command_execution_succeeds(env, "circle(28)");
  // FIXME: this is actually not true
  // assert_command_execution_succeeds(env, "position()", "(0.0, 0.0)\n");
  deinit_environment();
#endif
}

QUIZ_CASE(python_turtle_distance) {
#ifndef PLATFORM_WINDOWS
  TestExecutionEnvironment env = init_environement();
  assert_command_execution_succeeds(env, "from turtle import *");
  assert_command_execution_succeeds(env, "goto(10,10)");
  assert_command_execution_succeeds(env, "distance(0,0)",
                                    "14.14213562011719\n");
  assert_command_execution_succeeds(env, "distance((0,0))",
                                    "14.14213562011719\n");
  assert_command_execution_succeeds(env, "distance(10,0)", "10.0\n");
  assert_command_execution_fails(env, "distance()");
  assert_command_execution_fails(env, "distance(,)");
  assert_command_execution_fails(env, "distance(10)");
  assert_command_execution_fails(env, "distance(,10)");
  assert_command_execution_fails(env, "distance(10,)");
  deinit_environment();
#endif
}
