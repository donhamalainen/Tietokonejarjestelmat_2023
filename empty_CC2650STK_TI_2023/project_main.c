/* C Standard library */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* RANDOM GENERATOR */
#include <time.h>
#include <stdlib.h>

/* XDCtools files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>

/* BIOS Header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/power/PowerCC26XX.h>
#include <ti/drivers/UART.h>

/* Board Header files */
#include "Board.h"
#include "buzzer.h"
#include "sensors/opt3001.h"
#include "sensors/mpu9250.h"


/* Task */
#define STACKSIZE 2048
Char sensorTaskStack[STACKSIZE];
Char uartTaskStack[STACKSIZE];
Char speakerTaskStack[STACKSIZE];
uint8_t uartBuffer[80];

enum Actions {
    EAT,
    EXERCISE,
    PET,
    ACTIVATE,
    IDLE,
    SEND,
    SEND_TWO,
};
enum Actions action = IDLE;

enum musicState {
    DEAD,
    ALIVE
};
enum musicState isAlive = ALIVE;

enum state { WAITING=1, DATA_READY };
enum state programState = WAITING;
// GLOBALS VAR
double ambientLight = -1000.0;
char lux[100], eat[100], pet[100], exercise[100], activate[100], send[100], sendTwo[100], uartRead[100];
char* myID = "3232";
const char *beepMSG[1] = {
  "3232,BEEP:Too late, you need to take better care of me next time" // DEAD
};
// MPU
static PIN_Handle hMpuPin;
static PIN_State  MpuPinState;
// BUZZER
static PIN_Handle hBuzzer;
static PIN_State sBuzzer;
// BUTTONS
static PIN_Handle buttonHandle;
static PIN_State buttonState;
static PIN_Handle buttonHandle_two;
static PIN_State buttonState_two;
static PIN_Handle ledHandle;
static PIN_State ledState;
static PIN_Handle ledHandle_two;
static PIN_State ledState_two;

// CONFIG
PIN_Config cBuzzer[] = {
  Board_BUZZER | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
  PIN_TERMINATE
};
PIN_Config buttonConfig[] = {
   Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE
};
PIN_Config buttonConfig_two[] = {
   Board_BUTTON1  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE
};
PIN_Config ledConfig[] = {
   Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE
};
PIN_Config ledConfig_two[] = {
   Board_LED1 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE
};

static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};


///////////////////////////////////
// FUNCTIONS
///////////////////////////////////

// Buttons, where buttonTwo is RED LED and button is GREEN LED
void buttonFxnTwo(PIN_Handle handle, PIN_Id pinId) {
    uint_t pinValue_two = PIN_getOutputValue( Board_LED1 );
    pinValue_two = !pinValue_two;
    PIN_setOutputValue( ledHandle_two, Board_LED1, pinValue_two );
    // ACT
    action = PET;
}
void buttonFxn(PIN_Handle handle, PIN_Id pinId) {
    uint_t pinValue = PIN_getOutputValue( Board_LED0 );
    pinValue = !pinValue;
    PIN_setOutputValue( ledHandle, Board_LED0, pinValue );
    // ACT
    action = ACTIVATE;
}

void uartReadMessageCompare(){
    int i;
    for(i = 0; i <= 1; i++){
        if(strncmp(uartRead, beepMSG[i], 18) == 0){
            isAlive = DEAD;
            break;
        }
    }
}
static void uartFxn(UART_Handle handle, void *rxBuf, size_t len) {
   // Käsittelijän viimeisenä asiana siirrytään odottamaan uutta keskeytystä..
   if(strstr(rxBuf, myID) != 0){
       sprintf(uartRead, "%s", rxBuf);
       uartReadMessageCompare();
   }
   UART_read(handle, rxBuf, 80);
}


Void uartTaskFxn(UArg arg0, UArg arg1) {
    UART_Handle uart;
    UART_Params uartParams;

    // Alustetaan sarjaliikenne
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_TEXT;
    uartParams.readDataMode = UART_DATA_TEXT;
    uartParams.readEcho = UART_ECHO_OFF;
    //uartParams.readMode= UART_MODE_BLOCKING;
    uartParams.readMode      = UART_MODE_CALLBACK; // Keskeytyspohjainen vastaanotto
    uartParams.readCallback  = &uartFxn; // Käsittelijäfunktio
    uartParams.baudRate = 9600; // nopeus 9600baud
    uartParams.dataLength = UART_LEN_8; // 8
    uartParams.parityType = UART_PAR_NONE; // n
    uartParams.stopBits = UART_STOP_ONE; // 1s

    // Avataan yhteys laitteen sarjaporttiin vakiossa Board_UART0
    uart = UART_open(Board_UART0, &uartParams);
        if (uart == NULL) {
            System_abort("Error opening the UART");
    }

    UART_read(uart, uartBuffer, 80);

    // JTKJ: Exercise 4. Setup here UART connection as 9600,8n1
    sprintf(eat, "id:%d,EAT:%d", 3232, 4);
    sprintf(pet, "id:%d,PET:%d", 3232, 2);
    sprintf(exercise, "id:%d,EXERCISE:%d", 3232, 2);
    sprintf(activate, "id:%d,ACTIVATE:%d;%d;%d", 3232, 1,2,3);
    sprintf(send, "id:%d,MSG1:It's so bright here,PET:%d\0", 3232,2);
    sprintf(sendTwo, "id:%d,MSG2:Weeeeeeeee! :D\0", 3232,2);
    bool statement = true;
    while (1) {
        if(programState == DATA_READY){
            // OPT
            // UART_write(uart, ping, strlen(ping) + 1);
            // MPU
            switch(action){
                       case EAT:
                            System_printf("EAT\n");
                            UART_write(uart, eat, strlen(eat) + 1);
                            statement = true;
                            break;
                       case PET:
                            System_printf("PET\n");
                            UART_write(uart, pet, strlen(pet) + 1);
                            statement = true;
                            break;
                       case EXERCISE:
                            System_printf("EXE\n");
                            UART_write(uart, exercise, strlen(exercise) + 1);
                            statement = true;
                            break;
                       case ACTIVATE:
                            System_printf("ACT\n");
                            UART_write(uart, activate, strlen(activate) + 1);
                            statement = true;
                            break;
                       case IDLE:
                            if(statement) {
                               System_printf("IDLE\n");
                               statement = false;
                            }
                            break;
                       case SEND:
                            System_printf("MSG\n");
                            UART_write(uart, send, strlen(send) + 1);
                            statement = true;
                            break;
                       case SEND_TWO:
                           System_printf("MSG2\n");
                           UART_write(uart, sendTwo, strlen(sendTwo) + 1);
                           statement = true;
                           break;
                       default:
                           break;
                       }
                       action = IDLE;
                       programState = WAITING;
        }
       // LOPPU ODOTUS
       Task_sleep(1000000 / Clock_tickPeriod);
    }
}
Void sensorTaskFxn(UArg arg0, UArg arg1) {
    // MPU
    float ax, ay, az, gx, gy, gz;
    I2C_Handle i2cMPU; // Own i2c-interface for MPU9250 sensor
    I2C_Params i2cMPUParams;
    // OPT
    I2C_Handle      i2c;
    I2C_Params      i2cParams;

    // MPU Väylä
    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;
    // OPT Väylä
    I2C_Params_init(&i2cParams);
    i2cParams.bitRate = I2C_400kHz;
    // MPU power on
    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON);

    // OPT CONFIGURE
    i2c = I2C_open(Board_I2C, &i2cParams);
    if (i2c == NULL) {
        System_abort("Error Initializing I2C\n");
    }
    Task_sleep(100000 / Clock_tickPeriod);
    opt3001_setup(&i2c);
    I2C_close(i2c);


    // MPU CONFIGURE
    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    if (i2cMPU == NULL) {
        System_abort("Error Initializing I2CMPU\n");
    }
    // MPU setup and calibration
    System_printf("MPU9250: Setup and calibration...\n");
    System_flush();
    Task_sleep(100000 / Clock_tickPeriod);
    mpu9250_setup(&i2cMPU);
    I2C_close(i2cMPU);
    System_printf("MPU9250: Setup and calibration OK\n");
    System_flush();


    while (1) {
        if(programState == WAITING){

        // OPT
        i2c = I2C_open(Board_I2C, &i2cParams);
            if (i2c == NULL) {
                System_abort("Error Initializing I2C\n");
        }
        ambientLight = opt3001_get_data(&i2c);

        if (ambientLight >= 150) {
            action = SEND;
        }



        I2C_close(i2c);
        Task_sleep(100000 / Clock_tickPeriod);
        // MPU
        i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
            if (i2cMPU == NULL) {
                System_abort("Error Initializing I2CMPU\n");
        }
        // MPU:n getData kutsu
        mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
               if(az <= -1.2 || az >= 0.8){
                   action = SEND_TWO;
               }
               if(ax >= 0.4 || ax <= -0.4){
                   action = EXERCISE;
               }
               if(ay >= 0.4 || ay <= -0.4){
                   action = EAT;
               }



        I2C_close(i2cMPU);
        programState = DATA_READY;
        // LOPPU ODOTUS
        Task_sleep(1000000 / Clock_tickPeriod);
        }
    }
}
void deadMusic(){
    buzzerSetFrequency(784);
    Task_sleep(100000 / Clock_tickPeriod);

    buzzerSetFrequency(0);
    Task_sleep(50000 / Clock_tickPeriod);

    buzzerSetFrequency(392);
    Task_sleep(100000 / Clock_tickPeriod);

    buzzerSetFrequency(0);
    Task_sleep(50000 / Clock_tickPeriod);

    buzzerSetFrequency(784);
    Task_sleep(100000 / Clock_tickPeriod);

    buzzerSetFrequency(0);
    Task_sleep(50000 / Clock_tickPeriod);

    buzzerSetFrequency(1568);
    Task_sleep(100000 / Clock_tickPeriod);

    buzzerSetFrequency(0);
    Task_sleep(50000 / Clock_tickPeriod);

    buzzerSetFrequency(392);
    Task_sleep(100000 / Clock_tickPeriod);

    buzzerSetFrequency(0);
    Task_sleep(50000 / Clock_tickPeriod);

    buzzerSetFrequency(784);
    Task_sleep(100000 / Clock_tickPeriod);

    buzzerSetFrequency(0);
    Task_sleep(50000 / Clock_tickPeriod);

    buzzerSetFrequency(1568);
    Task_sleep(100000 / Clock_tickPeriod);

    buzzerSetFrequency(0);
    Task_sleep(50000 / Clock_tickPeriod);

    buzzerSetFrequency(2093);
    Task_sleep(100000 / Clock_tickPeriod);

    buzzerSetFrequency(0);
    Task_sleep(50000 / Clock_tickPeriod);

    buzzerSetFrequency(1568);
    Task_sleep(100000 / Clock_tickPeriod);

    buzzerSetFrequency(0);
    Task_sleep(50000 / Clock_tickPeriod);

    buzzerSetFrequency(784);
    Task_sleep(100000 / Clock_tickPeriod);

    buzzerSetFrequency(0);
    Task_sleep(50000 / Clock_tickPeriod);

    buzzerSetFrequency(392);
    Task_sleep(100000 / Clock_tickPeriod);

    buzzerSetFrequency(0);
    Task_sleep(50000 / Clock_tickPeriod);

    buzzerSetFrequency(196);
    Task_sleep(100000 / Clock_tickPeriod);

    buzzerSetFrequency(0);
    Task_sleep(50000 / Clock_tickPeriod);

    buzzerSetFrequency(784);
    Task_sleep(100000 / Clock_tickPeriod);

    buzzerSetFrequency(0);
    Task_sleep(50000 / Clock_tickPeriod);

    buzzerSetFrequency(392);
    Task_sleep(100000 / Clock_tickPeriod);

    buzzerSetFrequency(0);
    Task_sleep(50000 / Clock_tickPeriod);

    buzzerSetFrequency(784);
    Task_sleep(100000 / Clock_tickPeriod);

    buzzerSetFrequency(0);
    Task_sleep(50000 / Clock_tickPeriod);

      buzzerSetFrequency(1569);
      Task_sleep(100000 / Clock_tickPeriod);

      buzzerSetFrequency(0);
      Task_sleep(50000 / Clock_tickPeriod);

      buzzerSetFrequency(784);
      Task_sleep(100000 / Clock_tickPeriod);

      buzzerSetFrequency(0);
      Task_sleep(50000 / Clock_tickPeriod);

      buzzerSetFrequency(392);
      Task_sleep(100000 / Clock_tickPeriod);

      buzzerSetFrequency(0);
      Task_sleep(50000 / Clock_tickPeriod);

      buzzerSetFrequency(196);
        Task_sleep(100000 / Clock_tickPeriod);

          buzzerSetFrequency(0);
          Task_sleep(50000 / Clock_tickPeriod);

          buzzerSetFrequency(392);
          Task_sleep(100000 / Clock_tickPeriod);

          buzzerSetFrequency(0);
          Task_sleep(50000 / Clock_tickPeriod);

          buzzerSetFrequency(784);
          Task_sleep(100000 / Clock_tickPeriod);

          buzzerSetFrequency(0);
          Task_sleep(50000 / Clock_tickPeriod);

          buzzerSetFrequency(1569);
          Task_sleep(100000 / Clock_tickPeriod);

          buzzerSetFrequency(0);
          Task_sleep(50000 / Clock_tickPeriod);

          buzzerSetFrequency(2093);
          Task_sleep(100000 / Clock_tickPeriod);

          buzzerSetFrequency(0);
          Task_sleep(50000 / Clock_tickPeriod);

          buzzerSetFrequency(0);
}

Void speakerFxn(UArg arg0, UArg arg1) {
    while(1){
        switch(isAlive){
            case DEAD:
                buzzerOpen(hBuzzer);
                deadMusic();
                buzzerClose();
                break;
            default:
                break;
        }
        isAlive = ALIVE;
        Task_sleep(200000L / Clock_tickPeriod);
    }
}

Int main(void) {
    ///////////////////////////////////
    // VARIABLES
    ///////////////////////////////////

    Task_Handle sensorTaskHandle;
    Task_Params sensorTaskParams;
    Task_Handle uartTaskHandle;
    Task_Params uartTaskParams;
    Task_Handle speakerTaskHandle;
    Task_Params speakerTaskParams;

    ///////////////////////////////////
    // INIT
    ///////////////////////////////////

    Board_initGeneral();
    Board_initI2C();
    Board_initUART();

    ///////////////////////////////////
    // BUTTONS
    ///////////////////////////////////

    ledHandle = PIN_open( &ledState, ledConfig );
    ledHandle_two = PIN_open( &ledState_two, ledConfig_two );
    if(!ledHandle || !ledHandle_two) {
       System_abort("Error initializing LED pin\n");
    }
    buttonHandle = PIN_open(&buttonState, buttonConfig);
    buttonHandle_two = PIN_open(&buttonState_two, buttonConfig_two);
    if(!buttonHandle || !buttonHandle_two) {
       System_abort("Error initializing button pin\n");
    }
    if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0) {
       System_abort("Error registering button callback function");
    }
    if (PIN_registerIntCb(buttonHandle_two, &buttonFxnTwo) != 0) {
             System_abort("Error registering button callback function");
    }

    ///////////////////////////////////
    // MPU
    ///////////////////////////////////

    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
    if (hMpuPin == NULL) {
        System_abort("Pin open failed!");
    }
    ///////////////////////////////////
    // BUZZER
    ///////////////////////////////////
    hBuzzer = PIN_open(&sBuzzer, cBuzzer);
    if (hBuzzer == NULL) {
        System_abort("Pin open failed!");
    }

    ///////////////////////////////////
    // TASKS
    ///////////////////////////////////
    Task_Params_init(&speakerTaskParams);
    speakerTaskParams.stackSize = STACKSIZE;
    speakerTaskParams.stack = &speakerTaskStack;
    speakerTaskHandle = Task_create((Task_FuncPtr)speakerFxn, &speakerTaskParams, NULL);
    if (speakerTaskHandle == NULL) {
        System_abort("Task create failed!");
    }

    Task_Params_init(&sensorTaskParams);
    sensorTaskParams.stackSize = STACKSIZE;
    sensorTaskParams.stack = &sensorTaskStack;
    sensorTaskParams.priority=2;
    sensorTaskHandle = Task_create(sensorTaskFxn, &sensorTaskParams, NULL);
    if (sensorTaskHandle == NULL) {
        System_abort("Task create failed!");
    }

    Task_Params_init(&uartTaskParams);
    uartTaskParams.stackSize = STACKSIZE;
    uartTaskParams.stack = &uartTaskStack;
    uartTaskParams.priority=2;
    uartTaskHandle = Task_create(uartTaskFxn, &uartTaskParams, NULL);
    if (uartTaskHandle == NULL) {
        System_abort("Task create failed!");
    }

    System_printf("Nice hair!\n");
    System_flush();

    BIOS_start();

    return (0);
}
