/*
 * Copyright (C) 2016 The AllWinner Project
 */

#define LOG_TAG "powerkey"

#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "powerkey_input.h"

int main(int argc, char **argv)
{
	powerkey_input_init();
}
