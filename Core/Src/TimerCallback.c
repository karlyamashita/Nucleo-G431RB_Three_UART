/*

Creates a timer and will callback a function when timer reaches threshold value.


Good for
- Debouncing a push button switch
- Blinking an LED or sending telemetry data at an interval
- Debouncing a push button switch and counting the number of times the PB is pressed. This requires more than one callback function.
- And anything else you can imagine.




- Define timer instance some where in main or polling file. // TimerCallbackStruct timerInstanceMilliSeconds[MAX_TIMER_CALLBACK] = {0};
- Create a callback passing timer instance and parameters. // TimerCallbackRegister(timerInstanceMilliSeconds, <function to callback>, 100, TIMER_REPEAT);
					Note: The very first callback that is registered holds the timerLastIndex value. This callback should never be deleted or the index value will be lost.
- Call this function "TimerCallbackRegister(&timerCallback, <the function name>) to register a function.
- Call this function "TimerCallbackTick(&timerCallback)" from SysTick_Handler() in stm32f1xx_it.c which will increment a timer for each function that is registered.
- Call this function "TimerCallbackCheck(&timerCallback)" from polling routine. This will go through each function and call the function if the timer is >= desired time.



- Make sure to adjust define values in header file accordingly to your needs.


Revision 2 - 02/17/2022, Added of passing TimerCallbackStruct Instance so that you can have more than one timer instance. One typically for Systick at 1ms and maybe another for micro seconds.
Rev 2.1 - 10/21/2022, Added callback repetition.
Rev 2.2 - 11/22/2022, Added TimerCallbackRegisterStruct function to pass structure data instead of individual arguments.
Rev 2.3 - 12/24/2022, Finished the Callback Repetition. Added secondary callback, not tested yet.
Rev 2.4 - 12/24/2022, Finished Secondary callback and working.
Rev 2.5 - 12/25/2022, Renamed some variables and function names.
Rev 2.6 - 12/28/2022, Updated TimerCallbackRegisterStruct to pass the instance you want to add the structure to
Rev 2.7 - 12/28/2022, Modified TimerCallbackRegister to only register a function.
						Some renaming. Added disable and reset for Repetition. Updated descriptions of some functions.
Rev 2.8 - 12/29/2022, Renamed ShutDown to Timeout. Added functions for 2nd callback.
Rev 2.9 - 01/09/2023, Renamed some missed variables from ShutDown to Timeout.


*/


#include "main.h"
#include "TimerCallback.h"

TimerCallbackStruct timerCallback[MAX_TIMER_CALLBACK]; // default instance. Use extern where needed

static void TimerCallbackSort(TimerCallbackStruct *timerCallback);


/*
 * Description: Register a callback. There are no parameters.
 * 				To start repetition, use the TimerCallbackRepetitionStart.
 * 				To start a timer to shut down after a time use TimerCallbackTimeoutStart
 * 				To start a regular timer use TimerCallbackTimerStart. You can have it repeat or no repeat.
 *
 */
int TimerCallbackRegisterOnly(TimerCallbackStruct *timerInstance, TimerCallback callback)
{
    uint8_t i = 0;

    while(timerInstance[i].callback != 0) {
		if(timerInstance[i].callback == callback) {
			return -1;// Callback already defined
		}

		if(i == MAX_TIMER_CALLBACK) {
			return 0;// Maximum timers reached
		}
		i++;// next
	};

    timerInstance[i].callback = callback;

    timerInstance[i].timerRepetitionEnable = false;
    timerInstance[i].timerEnabled = false;

    timerInstance[0].timerLastIndex = i + 1; // only stored in first callback, index 0.

    return 0;
}

/*
 * Description: Register a 2nd callback that will be called after the timer is disabled usually from a timetout or repetition.
 * 				The 2nd callback is automatically enabled when this function is called.
 * 					However the 2nd callback will not be called until a timer (one shot, timeout or repetition) is started and has been disabled at the end of it's cycle.
 * 				Use TimerCallbackRegister2ndDisable to disable the 2nd callback at any time.
 * 				Note - The primary callback has to be registered first, otherwise the 2nd callback will not be registered.
 *
 */
int TimerCallbackRegister2nd(TimerCallbackStruct *timerInstance, TimerCallback callback, TimerCallback callback2)
{
	int i = 0;

	while(timerInstance[i].callback != callback) {
		if( i == timerInstance[0].timerLastIndex) {
			return 1;// callback not found
		}
		i++;
	};

	timerInstance[i].callback2 = callback2;
	timerInstance[i].timerCallback2Enabled = true;

	return i;
}


/*
 * Description: Disable the 2nd callback.
 *
 */
int TimerCallbackRegister2ndDisable(TimerCallbackStruct *timerInstance, TimerCallback callback)
{
	int i = 0;

	while(timerInstance[i].callback != callback) {
		if( i == timerInstance[0].timerLastIndex) {
			return 1;// callback not found
		}
		i++;
	};

	timerInstance[i].timerCallback2Enabled = false;

	return i;
}

/*
 * Description: Uses a single structure to save all the parameters instead of passing a bunch of arguments. Be sure to initialize new instance to zero.
 * Input: timerInstance to add the new callback to, timerInstanceAdd is the new callback you want to add
 */
int TimerCallbackRegisterStruct(TimerCallbackStruct * timerInstance, TimerCallbackStruct * timerInstanceAdd)
{
    int i = 0;
    
    while(timerInstance[i].callback != 0) {
		if(i == MAX_TIMER_CALLBACK) {
			return 0;// Maximum timers reached
		}	
		i++;// next
	};

    timerInstance[i].callback = timerInstanceAdd->callback;
    timerInstance[i].callback2 = timerInstanceAdd->callback2;
    
    timerInstance[i].timerTimeoutEnable = timerInstanceAdd->timerTimeoutEnable;
    timerInstance[i].timerTimeoutValue = timerInstanceAdd->timerTimeoutValue;
    timerInstance[i].timerTimeoutTick = 0; // clear shutdown timer

    timerInstance[i].timerRepetitionEnable = timerInstanceAdd->timerRepetitionEnable;
    timerInstance[i].timerRepetitionValue = timerInstanceAdd->timerRepetitionValue;
    timerInstance[i].timerRepetitionTick = 0; // clear

    timerInstance[i].timerEnabled = timerInstanceAdd->timerEnabled;
    timerInstance[i].timerValue = timerInstanceAdd->timerValue;
    timerInstance[i].timerTick = 0;// clear the timer

    timerInstance[i].timerRepeat = timerInstanceAdd->timerRepeat;
    timerInstance[i].timerCallback2Enabled = timerInstanceAdd->timerCallback2Enabled;

    timerInstance[0].timerLastIndex = i + 1; // only stored in first callback, index 0.
    
    return i;
}

/*
* Description: Starts the Timeout timer. User must register a function using TimerCallbackRegisterOnly
* input: timer instance, the function to callback, the timer value between each callback. The timerTimeoutValue for disabling the callback after amount of time.
* output: the timer array pointer. 0 if no array available, -1 if defined already, -2 if null callback
*/
int TimerCallbackTimeoutStart(TimerCallbackStruct *timerInstance, TimerCallback callback, uint32_t timerValue, uint32_t timerTimeoutValue)
{
	int i = 0;

	while(timerInstance[i].callback != callback) {
		if( i == timerInstance[0].timerLastIndex) {
			return 1;// callback not found
		}
		i++;
	};

	if(timerTimeoutValue < timerValue){ // timerShutDownValue should not be less than timerValue
	    timerTimeoutValue = timerValue + 1;
	}

	timerInstance[i].timerValue = timerValue;
    timerInstance[i].timerTick = 0;// clear the timer count
    timerInstance[i].timerRepeat = true;
    timerInstance[i].timerEnabled = 1;

	timerInstance[i].timerTimeoutValue = timerTimeoutValue;
	timerInstance[i].timerTimeoutTick = 0; // clear shutdown timer count
	timerInstance[i].timerTimeoutEnable = true;

	return i;
}

/*
 * Description: Disable the Timeout timer
 *
 *
 */
int TimerCallbackTimeoutDisable(TimerCallbackStruct *timerInstance, TimerCallback callback)
{
    uint8_t i = 0;

    while (timerInstance[i].callback != callback)
    {
        if (i == timerInstance[0].timerLastIndex)
        {
            return 1; // callback not found
        }
        i++;
    };
    timerInstance[i].timerTimeoutEnable = 0; // not enabled
    return 0;
}

/*
 * Description: Resets the Timeout tick count
 *
 */
int TimerCallbackTimeoutReset(TimerCallbackStruct *timerInstance, TimerCallback callback)
{
    uint8_t i = 0;

    while(timerInstance[i].callback != callback) {
        if(i == timerInstance[0].timerLastIndex) {
            return 1;// callback not found
        }
        i++;
    };
    timerInstance[i].timerTimeoutTick = 0; // clear timerShutDownCount
    return 0;
}



/*
 * Description: You can set how many callback repetitions to do before ending callbacks. The time is the delay between each repetition.
 * 				User needs to register a callback first using TimerCallbackRegisterOnly
 *
 *				Example use would be to blink an LED 3 times On/Off using the HAL_GPIO_TogglePin.
 * 				The time could be like 500ms between On and Off. If the LED is starting in the Off state,
 * 				use a even number to end with the LED Off, else use an odd number to end with the LED on.
 *				So to blink the LED 3 times, use 6 for the repetition.
 *
 */
int TimerCallbackRepetitionStart(TimerCallbackStruct *timerInstance, TimerCallback callback, uint32_t time, uint32_t repetition)
{
    uint8_t i = 0;

    while(timerInstance[i].callback != callback) {
        if( i == timerInstance[0].timerLastIndex) {
            return 1;// callback not found
        }
        i++;
    };

    timerInstance[i].timerRepetitionValue = repetition;
    timerInstance[i].timerRepetitionTick = 0;
    timerInstance[i].timerRepetitionEnable = true;

    timerInstance[i].timerValue = time;
    timerInstance[i].timerRepeat = true;
    timerInstance[i].timerEnabled = true;

    return 0;
}

/*
 * Description: Disables the Repetition
 *
 */
int TimerCallbackRepetitionDisable(TimerCallbackStruct *timerInstance, TimerCallback callback)
{
    uint8_t i = 0;

    while(timerInstance[i].callback != callback) {
        if( i == timerInstance[0].timerLastIndex) {
            return 1;// callback not found
        }
        i++;
    };

    timerInstance[i].timerEnabled = true;

    return 0;
}

/*
 * Description: Resets the Repetition tick counter.
 *
 */
int TimerCallbackRepetitionResetTimer(TimerCallbackStruct *timerInstance, TimerCallback callback)
{
    uint8_t i = 0;

    while(timerInstance[i].callback != callback) {
        if( i == timerInstance[0].timerLastIndex) {
            return 1;// callback not found
        }
        i++;
    };

    timerInstance[i].timerRepetitionTick = 0;

    return 0;
}


/*
function:	Starts the callback timer
input: timer instance, the callback, the timerThreshold value and if it repeats.
output: return 0 if successful
*/
int TimerCallbackTimerStart(TimerCallbackStruct *timerInstance, TimerCallback callback, uint32_t timerValue, uint8_t repeat)
{
	uint8_t i = 0;

	while(timerInstance[i].callback != callback) {
		if( i == timerInstance[0].timerLastIndex) {
			return 1;// callback not found
		}
		i++;
	};
	timerInstance[i].timerValue = timerValue; // new timer value
	timerInstance[i].timerRepeat = repeat;
	timerInstance[i].timerTick = 0;// clear the timer count
	timerInstance[i].timerEnabled = true;
	return 0;
}

/*
function:	Disable the callback.
input: timer instance, the callback
output: return 0 if successful
*/
int TimerCallbackDisable(TimerCallbackStruct *timerInstance, TimerCallback callback)
{
	uint8_t i = 0;

	while(timerInstance[i].callback != callback) {
		if( i == timerInstance[0].timerLastIndex) {
			return 1;// callback not found
		}
		i++;
	};
    timerInstance[i].timerEnabled = 0; // disable
	return 0;
}

/*
function: Resets the timerCount value to zero and enables it. Good for de-bouncing switch
input: timer instance, the callback
output: return 0 if successful
 */
int TimerCallbackResetTimer(TimerCallbackStruct *timerInstance, TimerCallback callback)
{
	uint8_t i = 0;

	while(timerInstance[i].callback != callback) {
		if( i == timerInstance[0].timerLastIndex) {
			return 1;// callback not found
		}
		i++;
	};
	timerInstance[i].timerTick = 0;
	return 0;
}


/*
function:	Get the callback's timer value
input: timer instance, the callback, the variable address to pass the current timer value to
output: return 0 if successful
*/
int TimerCallbackGetCurrentTimerValue(TimerCallbackStruct *timerInstance, TimerCallback callback, uint32_t *timerValue)
{
	uint8_t i = 0;

	while(timerInstance[i].callback != callback) {
		if( i == timerInstance[0].timerLastIndex) {
			return 1;// callback not found
		}
		i++;
	};
	*timerValue = timerInstance[i].timerTick;
	return 0;
}

/*
function:	See if timer callback exists
input: timer instance, the callback
output: return 1 if callback exists
*/
int TimerCallbackExists(TimerCallbackStruct *timerInstance, TimerCallback callback)
{
	uint8_t i = 0;

	while(timerInstance[i].callback != callback) {
		if( i == timerInstance[0].timerLastIndex) {
			return 0;// callback not found
		}
		i++;
	};
	return 1;
}

/*
function: frees up the array callback position
input: timer instance, the callback
output: return 0 if successful
*/
int TimerCallbackDelete(TimerCallbackStruct *timerInstance, TimerCallback callback)
{
	uint8_t i = 0;

    while(timerInstance[i].callback != callback) {
		if( i == timerInstance[0].timerLastIndex) {
			return 1; // callback not found
		}
		i++;
	};
	timerInstance[i].callback = 0; // deinit callback routine
	TimerCallbackSort(timerInstance);// sort the array
    timerInstance[0].timerLastIndex -= 1; // one less callback
	return 0;
}


/*
function: Call this function from SysTick_Handler() in stm32f1xx_it.c
input: timer instance
output: none
*/
void TimerCallbackTick(TimerCallbackStruct *timerInstance)
{
	int i = 0;

	while(i != timerInstance[0].timerLastIndex) { // iterate through all arrays
        if (timerInstance[i].callback != 0)
        {
            if (timerInstance[i].timerTimeoutEnable) // check if shutdown is enabled
            {
                timerInstance[i].timerTimeoutTick += 1; // increment the timerShutDownCount
            }

            if (timerInstance[i].timerEnabled) // check if callback is enabled.
            {
            	timerInstance[i].timerTick += 1; // increment the timerCount
            }
        }
		i++;
	}
}

/*
function: Checks if timer is reached and jumps to callback function. 
				Will enter and exit on each array pointer increment. This is to reduce doing multiple callback functions in one call.
				Call this function from polling routine.
input: timer instance
output: none
*/
void TimerCallbackCheck(TimerCallbackStruct *timerInstance)
{
	int i = 0; // the array pointer

	while(i != timerInstance[0].timerLastIndex) {
	    if(timerInstance[i].timerTimeoutEnable == 1) { // check for shutdown first
	        if(timerInstance[i].timerTimeoutTick >= timerInstance[i].timerTimeoutValue) {
	        	timerInstance[i].timerTimeoutTick = 0;
	        	timerInstance[i].timerEnabled = 0; // disable timer

	        	if(timerInstance[i].timerCallback2Enabled)// new 12-25-2022
	        	{
	        		timerInstance[i].callback2();// jump to secondary callback function
	        	}
	        }
	    }

		if(timerInstance[i].timerEnabled) {// timer or repetition is enabled
			if(timerInstance[i].timerTick >= timerInstance[i].timerValue) {
				timerInstance[i].timerTick = 0;// clear timer
				timerInstance[i].callback();// jump to callback function
				if(timerInstance[i].timerRepetitionEnable) // new 4-27-2022
				{
				    if(++timerInstance[i].timerRepetitionTick >= timerInstance[i].timerRepetitionValue)
				    {
				        timerInstance[i].timerEnabled = 0; // disable timer

				        if(timerInstance[i].timerCallback2Enabled) // new 12-25-2022
						{
							timerInstance[i].callback2();// jump to secondary callback function
						}
				    }
				}
				if(timerInstance[i].timerRepeat == TIMER_NO_REPEAT) {// if no repeat then disable timer for this function
					timerInstance[i].timerEnabled = 0; // disable timer

					if(timerInstance[i].timerCallback2Enabled)// new 12-25-2022
					{
						timerInstance[i].callback2();// jump to secondary callback function
					}
				}
				i++;
				return;// A callback function has been called so exit and re-enter at next array pointer. This avoids blocking.
			}
		}
		i++; // iterate until no more array.
	}
	i = 0;// resets to beginning of array
}

/*
function: When a callback is deleted, this will sort and remove any blank callback in between other callbacks in timerCallback array.
input: timer instance
output: none
*/
static void TimerCallbackSort(TimerCallbackStruct *timerInstance)
{
	int i = 0;

	while(i != timerInstance[0].timerLastIndex - 1) { // go through all callbacks
		if(timerInstance[i].callback == 0) { // found zero at this index
			if(timerInstance[i+1].callback != 0) { // make sure next index is not zero.
				timerInstance[i].callback = timerInstance[i+1].callback;// shift following callback down one index
				timerInstance[i+1].callback = 0; // now delete callback from this index
			}
		}
		i++;
	}
}


