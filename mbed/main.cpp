#include "mbed.h"
#include <string>
#include <vector>
#include "Speaker.h"

// These are used for setting the serial buffer size and how long the mbed should wait between updates
static const float SLEEP_TIME = 0.5;    // x86 background process will only have updates every 1 second in ideal situations
static const float PIN_DELAY = 0.05;   // the time to wait for allowing the relays to fully cycle between transitions
static const float POWER_UP_TIME = 0; // seconds for the device clicker to go from an OFF to ON state
static const int NUM_CMDS = 8;

// Used for signaling to the main loop if new serial data has arrived by the serial rx interrupt
bool new_data;

// Enumerations for the device's power state
enum device_state_t {
    ON = 1,
    OFF = 2
};

// Object for linking to a computer via serial communication
Serial pc(USBTX, USBRX);

// Global string for storing the received serial data on the serial rx interrupt
string gStr;

Speaker mySpeaker(p18);
//AnalogOut DACout(p18);
//wave_player waver(&DACout);
//LocalFileSystem local("local");
//FILE *wave_file;


void rx_interrupt(void)
{
    int i = 0;

    // update global character array
    while (pc.readable() | (i<6) ) {
        gStr += pc.getc();
        i++;
        wait(0.02);
    }

    new_data = 1;
}


void make_edge(DigitalOut& pin)
{
    pin = !pin;
    wait(PIN_DELAY);
    pin = !pin;
}


void confirm_on(AnalogIn& ain)
{
    uint16_t readings[3];

    // get 5 initial readings
    for (int i=0; i<4; i++) {
        readings[i] = ain.read_u16();
        wait(0.01);
    }

    for (int i=0; i<4; i++) {
        if (readings[i] < 60000) {
            confirm_on(ain);  // break into recursion if past readings are not settled values
        }
    }
}


int main()
{
    pc.baud(9600);

    // Blinking LED to show that mbed is running
    DigitalOut led1(LED1);

    // On/Off, Send
    DigitalOut controls[] = { p5, p25 };

    // A, B, C, D, E
    DigitalOut selections[] = { p26, p27, p28, p29, p30 };

    // Used for reading the clicker's voltage regulator output for determining if clicker is on/off
    AnalogIn pwr_status(p17);

    // Define the commands variable that holds a list of all valid serial commands
    std::vector<string> commands(NUM_CMDS);

    // Define the valid set of serial commands
    commands[0] = "clickerPower\n";
    commands[1] = "clickerSend\n";
    commands[2] = "sayHere\n";
    commands[3] = "clickerA\n";
    commands[4] = "clickerB\n";
    commands[5] = "clickerC\n";
    commands[6] = "clickerD\n";
    commands[7] = "clickerE\n";

    //wave_file = fopen("/local/p.wav","r");

    // Initialize all relays to off state
    for (int i=0; i<6; i++)
        selections[i] = 0;

    // get initial power readings
    uint16_t pwr_level = pwr_status.read_u16();
    uint16_t pwr_level_new = pwr_status.read_u16();

    device_state_t dev_power = ON;    // initialize to an off state

    // variables for the main scope
    char current_selection = NULL;
    bool update_selection = false;
    int usr_cmd = 0;

    // Holding down active low Power button will put clicker in configuration mode...we don't want that
    controls[0] = 1;

    // Function to run at receiving serial data
    pc.attach(&rx_interrupt, Serial::RxIrq);

    // infinite processing loop
    while(1) {

        // pull in from globally allocated memory when interurpts are disabled
        __disable_irq();

        pwr_level_new = pwr_status.read_u16();

        if (new_data) {
            new_data = 0;

            // assign a valid command to variable for the switch case statement. If command is invalid, ignore
            for (int i=0; i<NUM_CMDS+1; i++) {
                if ( !(strcmp(gStr.c_str(), commands[i].c_str())) ) {
                    usr_cmd = i+1;
                    break;
                }
            }

            switch( usr_cmd ) {
                case 1: // clickerPower
                    make_edge(controls[0]);
                    break;

                case 2: // clickerSend
                    if(current_selection) {
                       // make_edge(controls[1]);
                        pc.printf("    Answer submitted: %c\r\n", current_selection);
                        current_selection = NULL;
                    } else {
                        pc.printf("    Must make a selection before attempting a submission\r\n");
                    }
                    make_edge(controls[1]); // comment line out for final version. Used for testing purposes here
                    break;

                case 3:  // sayHere
                    pc.printf("    Audio playing...\r\n");
                    mySpeaker.PlayNote(969.0,1.5,1.0);
                    //wave_file=fopen("/local/P.WAV","r");
                    //waver.play(wave_file);
                    //fclose(wave_file);
                    break;

                case 4: // clickerA
                    current_selection = 'A';
                    update_selection = true;
                    break;

                case 5: // clickerB
                    current_selection = 'B';
                    update_selection = true;
                    break;

                case 6: // clickerC
                    current_selection = 'C';
                    update_selection = true;
                    break;

                case 7: // clickerD
                    current_selection = 'D';
                    update_selection = true;
                    break;

                case 8: // clickerE
                    current_selection = 'E';
                    update_selection = true;
                    break;

                default:
                    break;
            }
        }


        if (update_selection) {

            update_selection = false;

            if (dev_power == ON) {
                make_edge(selections[usr_cmd - 4]);
                pc.printf("    You have selected option %c\r\n", current_selection);

            } else {
                pc.printf("    Device is not on\r\n");
                current_selection = NULL;
            }
        }

        // if clicker's voltage level has changed significantly...
        if ( abs(pwr_level_new - pwr_level) > 20000) {

            // settling time
            while ( abs(pwr_level_new - pwr_status.read_u16()) > 32 ) {
                // wait until the analog input settles to a steady state reading
                pwr_level_new = pwr_status.read_u16();
                wait(0.1);
            }

            if (pwr_level_new < pwr_level) { // device is shutting down
                dev_power = OFF;

            } else if (pwr_level_new > pwr_level) { // device is turning on
                confirm_on(pwr_status);     // wait for voltage levels to settle to steady value when powering up
                wait(POWER_UP_TIME);
                dev_power = ON;
            }

            // update the power level reading
            pwr_level = pwr_level_new;
            pc.printf("    Device power status updated. New status: %s\n\r", dev_power == ON ? "ON": "OFF");
        }


        // clear all command related variables
        gStr.clear();
        usr_cmd = 0;

        // turn interrupts back on before going into a sleeping state
        __enable_irq();

        // blink that shit then be lazy
        led1 = !led1;
        wait(SLEEP_TIME);
    }
}
