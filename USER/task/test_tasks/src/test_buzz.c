#include "test_buzz.h"
#include "key.h"
#include "buzz.h"

void TestBuzz_Init(void)
{
    /* Key_Init() and Buzz_Init() are called separately from main.c
       before test init — no double init here. */
}

void TestBuzz_Tick(void)
{
    if (Key_RisingEdge(1)) {
        Buzz_On();
    }
    if (Key_RisingEdge(2)) {
        Buzz_Off();
    }
}
