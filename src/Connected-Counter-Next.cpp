/*
 * Project - A rewrite of the counter code to accomodate new sensors and a new structure
 * Description: Sends data via cellular on an hourly basis - Oritented to Counting
 * Author: Chip McClelland
 * Date: 1-31-23
 */

// Version list 
// v0.1 - First Iteration - basic functionality
// v0.2 - Fixed issue with daily count reset
// v0.3 - Added a new Class for error handling
// v0.4 - Looking at improving the accuracy of the battery state of charge measurement, reporting period to last second of previous hour
// v0.5 - Changed startup squencing to prevent resetting counts on startup, added function to support setting daily count, changed startup flow
// v0.6 - Implemented a new selection for the sensor type - 0 - pressure, 1 - PIR, and now 2 - magnetometer, also adding a stub for accelerometer (3) - in their own class for easy editing
// v0.7 - Implemented local time to determine if the park is open
// v0.8 - Need to figure out why we are getting stuck after ~24 hours.  Need to report firmware release with "long".  Added Restart functions and Implemented delay setting and delay configuration particle function
// v0.9 - Merged Code from Alex and Chip - made some tweaks but not sure I have fixed the issue.
// v1.0 - Fixed the infinite flashing green error - issue was in the Alert Handline Switch / Case
// v1.1 - Updated the pinMode and code for handling the user switch
// v1.2 - Delay now in milliseconds. Added "retrieveAssetFirmwareVersion" to setup(), now able to configure printing various assets' firmware versions using the "status"="long" Particle function.
// v1.2.1 - Moved all interrupt functionality to Magnetometer instead, removed delay
// v1.3 - Fixed issue with AlertHandline for AlertCode 31
// v1.4 - Added publishing to the Update-Device, Send-Configuration, and Device-Command Particle Integration Hooks
// v1.4.1 - Added Serial SCPI Query, automatically updates a device type to 2 (Magnetometer) if a response is received.
// v1.4.2 - Fixed bug where SCPI queries falsely triggered altomatic updates added in v1.4.1

// Particle Libraries
#include "Particle.h"                                 // Because it is a CPP file not INO
#include "PublishQueuePosixRK.h"                      // Allows for queuing of messages - https://github.com/rickkas7/PublishQueuePosixRK
#include "LocalTimeRK.h"
#include "AB1805_RK.h"                                // Real Time Clock -and watchdog
// Application Libraries / Class Files
#include "device_pinout.h"							  // Define pinouts and initialize them
#include "Take_Measurements.h"						  // Manages interactions with the sensors (default is temp for charging)
#include "MyPersistentData.h"						  // Persistent Storage
#include "Particle_Functions.h"						  // Where we put all the functions specific to Particle
#include "Alert_Handling.h"
#include "Record_Counts.h"

#define FIRMWARE_RELEASE "1.4"						  // Will update this and report with stats

PRODUCT_VERSION(1);									  // For now, we are putting nodes and gateways in the same product group - need to deconflict #

// Prototype functions
void publishStateTransition(void);                    // Keeps track of state machine changes - for debugging
void userSwitchISR();                                 // interrupt service routime for the user switch
void sensorISR(); 
void countSignalTimerISR();							  // Keeps the Blue LED on
void UbidotsHandler(const char *event, const char *data);
bool isParkOpen(bool verbose);						  // Simple function returns whether park is open or not
void dailyCleanup();								  // Reset each morning
void softDelay(uint32_t t);							  // function for a safe delay()
String retrieveAssetFirmwareVersion();                // Keeps track of state machine changes - for debugging
void recordCount();									  // Called from the main loop when a sensor is triggered

// System Health Variables
int outOfMemory = -1;                                 // From reference code provided in AN0023 (see above)

// State Machine Variables
enum State { INITIALIZATION_STATE, ERROR_STATE, IDLE_STATE, SLEEPING_STATE, CONNECTING_STATE, DISCONNECTING_STATE, REPORTING_STATE, RESP_WAIT_STATE};
char stateNames[8][16] = {"Initialize", "Error", "Idle", "Sleeping", "Connecting", "Disconnecting", "Reporting", "Response Wait"};
State state = INITIALIZATION_STATE;
State oldState = INITIALIZATION_STATE;

// Initialize Functions
SystemSleepConfiguration config;                      // Initialize new Sleep 2.0 Api
void outOfMemoryHandler(system_event_t event, int param);
extern LocalTimeConvert conv;						  // For determining if the park should be opened or closed - need local time
AB1805 ab1805(Wire);                                  // Rickkas' RTC / Watchdog library

// Program Variables
volatile bool userSwitchDectected = false;		
volatile bool sensorDetect = false;					  // Flag for sensor interrupt
bool dataInFlight = false;                            // Flag for whether we are waiting for a response from the webhook

Timer countSignalTimer(1000, countSignalTimerISR, true);      // This is how we will ensure the BlueLED stays on long enough for folks to see it.

// Timing variables
const int wakeBoundary = 1*3600 + 0*60 + 0;           // Sets a reporting frequency of 1 hour 0 minutes 0 seconds
const unsigned long stayAwakeLong = 90000UL;          // In lowPowerMode, how long to stay awake every hour
const unsigned long stayAwakeShort = 1000UL;		  // In lowPowerMode, how long to stay awake when not reporting
const unsigned long webhookWait = 45000UL;            // How long will we wait for a WebHook response
const unsigned long resetWait = 30000UL;              // How long will we wait in ERROR_STATE until reset
unsigned long stayAwakeTimeStamp = 0UL;               // Timestamps for our timing variables..
unsigned long stayAwake = stayAwakeLong;              // Stores the time we need to wait before napping

// Testing variables
// bool dailyCleanupTestExecuted = false;

void setup() {

	char responseTopic[125];
	String deviceID = System.deviceID();              // Multiple devices share the same hook - keeps things straight
	deviceID.toCharArray(responseTopic,125);          // Puts the deviceID into the response topic array
	Particle.subscribe(responseTopic, UbidotsHandler, MY_DEVICES);      // Subscribe to the integration response event
	System.on(out_of_memory, outOfMemoryHandler);     // Enabling an out of memory handler is a good safety tip. If we run out of memory a System.reset() is done.

	waitFor(Serial.isConnected, 10000);               // Wait for serial to connect - for debugging
	softDelay(2000);								  // For serial monitoring - can delete

	Particle_Functions::instance().setup();			  // Initialize Particle Functions and Variables

    initializePinModes();                             // Sets the pinModes

	digitalWrite(BLUE_LED, HIGH);					  // Turn on the Blue LED for setup

    initializePowerCfg();                             // Sets the power configuration for solar

	sysStatus.setup();								  // Initialize persistent storage
	sysStatus.set_firmwareRelease(FIRMWARE_RELEASE);
	sysStatus.set_assetFirmwareRelease(retrieveAssetFirmwareVersion());

	current.setup();
	current.set_alertCode(0);						  // Clear any alert codes

  	PublishQueuePosix::instance().setup();            // Start the Publish Queue
	PublishQueuePosix::instance().withFileQueueSize(200);

	// Take note if we are restarting due to a pin reset - either by the user or the watchdog - could be sign of trouble
  	if (System.resetReason() == RESET_REASON_PIN_RESET || System.resetReason() == RESET_REASON_USER) { // Check to see if we are starting from a pin reset or a reset in the sketch
    	sysStatus.set_resetCount(sysStatus.get_resetCount() + 1);
    	if (sysStatus.get_resetCount() > 3) current.set_alertCode(13);    // Excessive resets 
  	}

    ab1805.withFOUT(D8).setup();                	  // Initialize AB1805 RTC   
	if (!ab1805.detectChip()) current.set_alertCode(12);
    ab1805.setWDT(AB1805::WATCHDOG_MAX_SECONDS);	  // Enable watchdog

	// Setup local time and set the publishing schedule
	LocalTime::instance().withConfig(LocalTimePosixTimezone("EST5EDT,M3.2.0/2:00:00,M11.1.0/2:00:00"));			// East coast of the US
	conv.withCurrentTime().convert();  	

	System.on(out_of_memory, outOfMemoryHandler);     // Enabling an out of memory handler is a good safety tip. If we run out of memory a System.reset() is done.

  	Take_Measurements::instance().takeMeasurements(); // Populates values so you can read them before the hour

	if (!digitalRead(BUTTON_PIN)) {				 	  // The user will press this button at startup to reset settings
		Log.info("User button at startup - setting defaults");
		state = CONNECTING_STATE;
		sysStatus.initialize();                  	  // Make sure the device wakes up and connects - reset to defaults and exit low power mode
	}

	if (!Time.isValid()) {
		Log.info("Time is invalid -  %s so connecting", Time.timeStr().c_str());
		state = CONNECTING_STATE;
	}
	else {
		Log.info("LocalTime initialized, time is %s and RTC %s set", conv.format("%I:%M:%S%p").c_str(), (ab1805.isRTCSet()) ? "is" : "is not");
		if (Time.day(sysStatus.get_lastConnection()) != Time.day()) {
			Log.info("New day, resetting counts");
			dailyCleanup();
		}
	} 

	attachInterrupt(BUTTON_PIN,userSwitchISR,FALLING);// We may need to monitor the user switch to change behaviours / modes
	attachInterrupt(INT_PIN,sensorISR,RISING);        // We need to monitor the sensor for activity


	if (state == INITIALIZATION_STATE) {
		if(sysStatus.get_lowPowerMode()) {
			state = IDLE_STATE;               		  // Go to the IDLE state unless changed above
		}
		else {
			state = CONNECTING_STATE;          		  // Go to the CONNECTING state unless changed above
		}
	}

	conv.withTime(sysStatus.get_lastConnection()).convert();	// Want to know the last time we connected in local time
  	Log.info("Startup complete with last connect %s in %s", conv.format("%I:%M:%S%p").c_str(), (sysStatus.get_lowPowerMode()) ? "low power mode" : "normal mode");
	conv.withCurrentTime().convert();
  	digitalWrite(BLUE_LED,LOW);                       // Signal the end of startup

	isParkOpen(true);

	Alert_Handling::instance().setup();
	Record_Counts::instance().setup();
}

void loop() {
	switch (state) {
		case IDLE_STATE: {						      // Unlike most sketches - nodes spend most time in sleep and only transit IDLE once or twice each period
			// if(!dailyCleanupTestExecuted){
			// 	dailyCleanup();
			// 	dailyCleanupTestExecuted = true;
			// }
			if (state != oldState) publishStateTransition();
			if (sysStatus.get_lowPowerMode() && (millis() - stayAwakeTimeStamp) > stayAwake) state = SLEEPING_STATE;         // When in low power mode, we can nap between taps
			if (isParkOpen(false) && Time.hour() != Time.hour(sysStatus.get_lastReport())) state = REPORTING_STATE;          // We want to report on the hour but not after bedtime
		} break;

		case SLEEPING_STATE: {
			if (state != oldState) publishStateTransition();              	// We will apply the back-offs before sending to ERROR state - so if we are here we will take action
	    	if (sensorDetect || countSignalTimer.isActive())  break;        // Don't nap until we are done with event - exits back to main loop but stays in napping state
			if (Particle.connected() || !Cellular.isOff()) {
				if (!Particle_Functions::instance().disconnectFromParticle()) {         // Disconnect cleanly from Particle and power down the modem
					current.set_alertCode(15);
					break;
				}
			}
			if (!isParkOpen(true)) digitalWrite(ENABLE_PIN,HIGH);
			else digitalWrite(ENABLE_PIN,LOW);
			stayAwake = stayAwakeShort;                                     // Keeps device awake for just a second - when we are not reporting
			int wakeInSeconds = constrain(wakeBoundary - Time.now() % wakeBoundary, 1, wakeBoundary) + 1;;	// Figure out how long to sleep 		
			config.mode(SystemSleepMode::ULTRA_LOW_POWER)
				.gpio(BUTTON_PIN,CHANGE)
				.gpio(INT_PIN,RISING)
				.duration(wakeInSeconds * 1000L);
			ab1805.stopWDT();  												 // No watchdogs interrupting our slumber
			SystemSleepResult result = System.sleep(config);              	 // Put the device to sleep device continues operations from here
			ab1805.resumeWDT();                                              // Wakey Wakey - WDT can resume
			if (result.wakeupPin() == BUTTON_PIN) {                          // If the user woke the device we need to get up - device was sleeping so we need to reset opening hours
				Log.info("Woke with user button - Resetting hours and going to connect");
				sysStatus.set_lowPowerMode(false);
				sysStatus.set_closeTime(24);
				sysStatus.set_openTime(0);
				stayAwake = stayAwakeLong;
				stayAwakeTimeStamp = millis();
				state = CONNECTING_STATE;
			}
			else if (result.wakeupPin() == INT_PIN) {
				Log.info("Woke with sensor - counting");
				state = IDLE_STATE;
			}
			else {															  // In this state the device was awoken for hourly reporting
				softDelay(2000);											  // Gives the device a couple seconds to get the battery reading
				Log.info("Time to wake up at %s with %li free memory", Time.format((Time.now()+wakeInSeconds), "%T").c_str(), System.freeMemory());
				if (isParkOpen(true)) stayAwake = stayAwakeLong;              // Keeps device awake after reboot - helps with recovery
				state = IDLE_STATE;
			}
		} break;

		case REPORTING_STATE: {
			if (state != oldState) publishStateTransition();
			sysStatus.set_lastReport(Time.now());                             // We are only going to report once each hour from the IDLE state.  We may or may not connect to Particle
			Take_Measurements::instance().takeMeasurements();                 // Take Measurements here for reporting
			if (Time.day(sysStatus.get_lastConnection()) != conv.getLocalTimeYMD().getDay()) {
				dailyCleanup();
				Log.info("New Day - Resetting everything");
			}
			Particle_Functions::instance().sendEvent();                       // Publish hourly but not at opening time as there is nothing to publish
			state = CONNECTING_STATE;                                         // Default behaviour would be to connect and send report to Ubidots

			// Let's see if we need to connect 
			if (Particle.connected()) {                                       // We are already connected go to response wait
				stayAwakeTimeStamp = millis();
				state = RESP_WAIT_STATE;
			}
			// If we are in a low battery state - we are not going to connect unless we are over-riding with user switch (active low)
			else if (sysStatus.get_lowBatteryMode() && digitalRead(BUTTON_PIN)) {
				Log.info("Not connecting - low battery mode");
				state = IDLE_STATE;
			}
			// If we are in low power mode, we may bail if battery is too low and we need to reduce reporting frequency
			else if (sysStatus.get_lowPowerMode() && digitalRead(BUTTON_PIN)) {     // Low power mode and user switch not pressed
				if (current.get_stateOfCharge() > 65) {
					Log.info("Sufficient battery power connecting");
				}
				else if (current.get_stateOfCharge() <= 50 && (Time.hour() % 4)) {  // If the battery level is <50%, only connect every fourth hour
					Log.info("Not connecting - <50%% charge - four hour schedule");
					state = IDLE_STATE;                                             // Will send us to connecting state - and it will send us back here
				}                                                                   // Leave this state and go connect - will return only if we are successful in connecting
				else if (current.get_stateOfCharge() <= 65 && (Time.hour() % 2)) {  // If the battery level is 50% -  65%, only connect every other hour
					Log.info("Not connecting - 50-65%% charge - two hour schedule");
					state = IDLE_STATE;                                             // Will send us to connecting state - and it will send us back here
					break;                                                          // Leave this state and go connect - will return only if we are successful in connecting
				}
			}
		} break;

  		case RESP_WAIT_STATE: {
    		static unsigned long webhookTimeStamp = 0;                         // Webhook time stamp
    		if (state != oldState) {
      			webhookTimeStamp = millis();                                   // We are connected and we have published, head to the response wait state
      			dataInFlight = true;                                           // set the data inflight flag
      			publishStateTransition();
    		}

    		if (!dataInFlight)  {                                              // Response received --> back to IDLE state
				stayAwakeTimeStamp = millis();
      			state = IDLE_STATE;
    		}
    		else if (millis() - webhookTimeStamp > webhookWait) {              // If it takes too long - will need to reset
				current.set_alertCode(40);
			}
  		} break;

  		case CONNECTING_STATE:{                                                // Will connect - or not and head back to the Idle state - We are using a 3,5, 7 minute back-off approach as recommended by Particle
			// dailyCleanupTestExecuted = false;								   // For testing Daily Cleanup in the Idle State
			static State retainedOldState;                                     // Keep track for where to go next (depends on whether we were called from Reporting)
			static unsigned long connectionStartTimeStamp;                     // Time in Millis that helps us know how long it took to connect
			char data[64];                                                     // Holder for message strings

			if (state != oldState) {                                           // Non-blocking function - these are first time items
				retainedOldState = oldState;                                   // Keep track for where to go next
				sysStatus.set_lastConnectionDuration(0);                       // Will exit with 0 if we do not connect or are already connected.  If we need to connect, this will record connection time.
				publishStateTransition();
				connectionStartTimeStamp = millis();                           // Have to use millis as the clock may get reset on connect
				Particle.connect();                                            // Tells Particle to connect, now we need to wait
			}

			sysStatus.set_lastConnectionDuration(int((millis() - connectionStartTimeStamp)/1000));

			if (Particle.connected()) {
				sysStatus.set_lastConnection(Time.now());                      // This is the last time we last connected
				stayAwakeTimeStamp = millis();                                 // Start the stay awake timer now
				Take_Measurements::instance().getSignalStrength();             // Test signal strength since the cellular modem is on and ready
				snprintf(data, sizeof(data),"Connected in %i secs",sysStatus.get_lastConnectionDuration());  // Make up connection string and publish
				Log.info(data);
				if (sysStatus.get_verboseMode()) Particle.publish("Cellular",data,PRIVATE);
				(retainedOldState == REPORTING_STATE) ? state = RESP_WAIT_STATE : state = IDLE_STATE; // so, if we are connecting to report - next step is response wait - otherwise IDLE
			}
			else if (sysStatus.get_lastConnectionDuration() > 600) { 		   // What happens if we do not connect - non-zero alert code will send us to the Error state
				Log.info("Failed to connect in 10 minutes");
				if (Cellular.ready()) current.set_alertCode(30);
				else current.set_alertCode(31);
				sysStatus.set_lowPowerMode(true);						       // If we are not connected after 10 minutes, we are going to go to low power mode
			}
		} break;

		case ERROR_STATE: {													   // Where we go if things are not quite right
			static int alertResponse = 0;
			static unsigned long resetTimer = millis();

			if (state != oldState) {
				publishStateTransition();                					   // We will apply the back-offs before sending to ERROR state - so if we are here we will take action
				alertResponse = Alert_Handling::instance().alertResolution();
				Log.info("Alert Response: %i so %s",alertResponse, (alertResponse == 0) ? "No action" : (alertResponse == 1) ? "Connecting" : (alertResponse == 2) ? "Reset" : (alertResponse == 3) ? "Power Down" : "Unknown");
				resetTimer = millis();
			}

			if (alertResponse >= 2 && millis()-resetTimer < 30000L) break;
			else Log.info("Delay is up - executing");

			switch (alertResponse) {
				case 0:
					Log.info("No Action - Going to Idle");
					state = IDLE_STATE;					// Least severity - no additional action required
					break;
				case 1:
					Log.info("Need to report - connecting");
					state = CONNECTING_STATE;			// Issue that needs to be reported - could be in a reset loop
					break;
				case 2:
					Log.info("Resetting");
					delay(1000);						// Give the system a second to get the message out
					System.reset();						// device needs to be reset
					break;
				case 3: 
					Log.info("Powering down");
					delay(1000);						// Give the system a second to get the message out
					ab1805.deepPowerDown();				// Power off the device for 30 seconds
					break;
				default:								// Ensure we do not get trapped in the ERROR State
					System.reset();
					break;
			}
		} break;
	}

	ab1805.loop();                                  	// Keeps the RTC synchronized with the Boron's clock

	// Housekeeping for each transit of the main loop
	current.loop();
	sysStatus.loop();

	PublishQueuePosix::instance().loop();               // Check to see if we need to tend to the message queue
	Alert_Handling::instance().loop();	
	Record_Counts::instance().loop();

	if (outOfMemory >= 0) {                         	// In this function we are going to reset the system if there is an out of memory error
	  current.set_alertCode(14);
  	}

	if (current.get_alertCode() > 0) state = ERROR_STATE;

	if (sensorDetect) {									// If the sensor has been triggered, we need to record the count
		sensorDetect = false;
		if (Record_Counts::instance().recordCounts()) {
			Log.info("Count recorded");
			pinSetFast(BLUE_LED);                       // Turn on the blue LED
			countSignalTimer.reset();					// Keeps the LED light on so we can see it
		}
		else Log.info("Count not recorded");
	}		

	if (userSwitchDectected) {							// If the user switch has been pressed, we need to reset the device
		userSwitchDectected = false;
		// digitalWrite(ENABLE_PIN, !digitalRead(ENABLE_PIN));						// Toggle the enable pin
		// Log.info("User switch pressed and Enable pin is now %s", (digitalRead(ENABLE_PIN)) ? "HIGH" : "LOW");
		// delay(1000);	// Give the system a second to get the message out
	}
}

/**
 * @brief Publishes a state transition to the Log Handler and to the Particle monitoring system.
 *
 * @details A good debugging tool.
 */
void publishStateTransition(void)
{
	char stateTransitionString[256];
	if (state == IDLE_STATE) {
		if (!Time.isValid()) snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s with invalid time", stateNames[oldState],stateNames[state]);
		else snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s", stateNames[oldState],stateNames[state]);
	}
	else snprintf(stateTransitionString, sizeof(stateTransitionString), "From %s to %s", stateNames[oldState],stateNames[state]);
	oldState = state;
	Log.info(stateTransitionString);
}

// Here are the various hardware and timer interrupt service routines
void outOfMemoryHandler(system_event_t event, int param) {
    outOfMemory = param;
}

void userSwitchISR() {
  	userSwitchDectected = true;                                     // The the flag for the user switch interrupt
}

void sensorISR() {
	sensorDetect = true;											// Set the flag for the sensor interrupt
}

void countSignalTimerISR() {
  digitalWrite(BLUE_LED,LOW);
}

bool isParkOpen(bool verbose) {
	conv.withCurrentTime().convert();
	if (verbose) {
	  Log.info("Local hour is %i and the park is %s", conv.getLocalTimeHMS().hour, (conv.getLocalTimeHMS().hour < sysStatus.get_openTime() || conv.getLocalTimeHMS().hour > sysStatus.get_closeTime()) ? "closed" : "open");
	}
	if (conv.getLocalTimeHMS().hour < sysStatus.get_openTime() || conv.getLocalTimeHMS().hour > sysStatus.get_closeTime()) return false;
	else return true;
}


void UbidotsHandler(const char *event, const char *data) {          // Looks at the response from Ubidots - Will reset Photon if no successful response
  char responseString[64];
    // Response is only a single number thanks to Template
  if (!strlen(data)) {                                              // No data in response - Error
    snprintf(responseString, sizeof(responseString),"No Data");
  }
  else if (atoi(data) == 200 || atoi(data) == 201) {
    snprintf(responseString, sizeof(responseString),"Response Received");
	dataInFlight = false;										    // We have received a response - so we can send another
    sysStatus.set_lastHookResponse(Time.now());                     // Record the last successful Webhook Response
  }
  else {
    snprintf(responseString, sizeof(responseString), "Unknown response recevied %i",atoi(data));
  }
  if (sysStatus.get_verboseMode() && Particle.connected()) {
    Particle.publish("Ubidots Hook", responseString, PRIVATE);
  }
  Log.info(responseString);
}

/**
 * @brief Cleanup function that is run at the beginning of the day.
 *
 * @details May or may not be in connected state.  Syncs time with remote service and sets low power mode.
 * Called from Reporting State ONLY. Cleans house at the beginning of a new day.
 */
void dailyCleanup() {
  if (Particle.connected()) Particle.publish("Daily Cleanup","Running", PRIVATE);   // Make sure this is being run
  Log.info("Running Daily Cleanup");
  sysStatus.set_verboseMode(false);                                       			// Saves bandwidth - keep extra chatter off
  if (sysStatus.get_solarPowerMode() || current.get_stateOfCharge() <= 65) {     	// If Solar or if the battery is being discharged
    sysStatus.set_lowPowerMode(true);
  }
  if(sysStatus.get_sensorType() != 2){			// Execute a response check on the serial line ONLY if the device is not already a Magnetometer
	String version = "";  						    	// Set version to empty string, we will check if this has changed later
  	Serial1.begin(115200);						    	// Open serial connection
  	Serial1.print("*VER?");						    	// Query device for its Version
	Serial1.setTimeout(200);							// Set a maximum timeout
	version = Serial1.readString();
	Log.info("Response from Serial? %s", strcmp(version.c_str(), "") != 0 ? "Yes" : "No");
	if(strcmp(version.c_str(), "") != 0){				// Close serial connection 
		Serial1.end();								 	// If we returned something ...
		sysStatus.set_sensorType(2);						 	// ... take note that we are a magnetometer now.		
		Log.info("Response from Serial. Setting device type to \"Magnetometer\"");
		Particle.publish("Magnetometer Sensor Detected. Setting Sensor Type.", "2 (Magnetometer)", PRIVATE);
	} else {
		Log.info("No Response from Serial. Not changing device sensor type.");
	}
  }
  char configData[256]; 							 	 // Store the configuration data in this character array - not global
  snprintf(configData, sizeof(configData), "{\"timestamp\":%lu000, \"power\":\"%s\", \"lowPowerMode\":\"%s\", \"timeZone\":\"" + sysStatus.get_timeZoneStr() + "\", \"open\":%i, \"close\":%i, \"sensorType\":%i, \"verbose\":\"%s\", \"connecttime\":%i, \"battery\":%4.2f}", Time.now(), sysStatus.get_solarPowerMode() ? "Solar" : "Utility", sysStatus.get_lowPowerMode() ? "Low Power" : "Not Low Power", sysStatus.get_openTime(), sysStatus.get_closeTime(), sysStatus.get_sensorType(), sysStatus.get_verboseMode() ? "Verbose" : "Not Verbose", sysStatus.get_lastConnectionDuration(), current.get_stateOfCharge());
  PublishQueuePosix::instance().publish("Send-Configuration", configData, PRIVATE | WITH_ACK);    // Send new configuration to FleetManager backend. (v1.4)
  current.resetEverything();                             // If so, we need to Zero the counts for the new day
}

/**
 * @brief soft delay let's us process Particle functions and service the sensor interrupts while pausing
 * 
 * @details takes a single unsigned long input in millis
 * 
 */
inline void softDelay(uint32_t t) {
  for (uint32_t ms = millis(); millis() - ms < t; Particle.process());  			//  safer than a delay()
}

/**
 * @brief retrieveAssetFirmwareVersion queries a sensor connected to this particle device,
 * asking for its version. Sets the the returned version in MyPersistentData.cpp
 * 
 */
inline String retrieveAssetFirmwareVersion() {
  switch(sysStatus.get_sensorType()) {
    case 0: {                                                /*** Pressure Sensor ***/
		  // set or retrieve pressure sensor's firmware version here
		  return "0.0";
    } break;
    case 1:	{												 /*** PIR Sensor ***/
		  // set or retrieve PIR sensor's firmware version here
		  return "0.0";
	  } break;
    case 2: {												 /*** Magnetometer Sensor ***/
		  Serial1.begin(115200);								 // Open serial connection
		  waitFor(Serial1.available(), 10000); 				 // Make sure the serial monitor can connect
		  Serial1.print("*VER?");								 // Query magnetometer for its version
		  delay(1000); 							     		 // Make sure it has time to output
		  String version = Serial1.readString();		 		 // Read the output		 
		  Serial1.end();	
		  return version;							             // Close serial connection
    } break;
    case 3: {												 /*** Accelerometer Sensor ***/
		  // set or retrieve accelerometer sensor's firmware version here 
		  return "0.0";
    } break;
    default:
     	return "0.0";                                  		 // Default to 0.0
  }
}
