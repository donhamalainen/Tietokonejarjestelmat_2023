/* C Standard library */
#include <stdio.h>

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
#include "sensors/opt3001.h"
#include "sensors/mpu9250.h"

/* Task */
enum Actions {
    EAT,
    EXERCISE,
    PET,
    ACTIVATE,
    IDLE
};
enum Actions action = IDLE;

#define STACKSIZE 2048
Char sensorTaskStack[STACKSIZE];
Char uartTaskStack[STACKSIZE];
// MPU power pin global variables
static PIN_Handle hMpuPin;
static PIN_State  MpuPinState;

// JTKJ: Tehtävä 3. Tilakoneen esittely
// JTKJ: Exercise 3. Definition of the state machine
enum state { WAITING=1, DATA_READY };
enum state programState = WAITING;

// JTKJ: Tehtävä 3. Valoisuuden globaali muuttuja
// JTKJ: Exercise 3. Global variable for ambient light
double ambientLight = -1000.0;
char lux[100], eat[100], pet[100], exercise[100], activate[100];

// JTKJ: Tehtävä 1. Lisää painonappien RTOS-muuttujat ja alustus

// RTOS:n muuttujat pinnien käyttöön
static PIN_Handle buttonHandle;
static PIN_State buttonState;
static PIN_Handle buttonHandle_two;
static PIN_State buttonState_two;
static PIN_Handle ledHandle;
static PIN_State ledState;
static PIN_Handle ledHandle_two;
static PIN_State ledState_two;

// Pinnien alustukset, molemmille pinneille oma konfiguraatio
// Vakio BOARD_BUTTON_0 vastaa toista painonappia
PIN_Config buttonConfig[] = {
   Board_BUTTON0  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE
};
PIN_Config buttonConfig_two[] = {
   Board_BUTTON1  | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_NEGEDGE,
   PIN_TERMINATE
};

// Vakio Board_LED0 vastaa toista lediä
PIN_Config ledConfig[] = {
   Board_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
   PIN_TERMINATE
};

// MPU power pin
static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

// MPU uses its own I2C interface
static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};


void buttonFxnTwo(PIN_Handle handle, PIN_Id pinId) {

    // JTKJ: Tehtävä 1. Vilkuta jompaa kumpaa lediä
    uint_t pinValue_two = PIN_getOutputValue( Board_LED1 );
    pinValue_two = !pinValue_two;
    PIN_setOutputValue( ledHandle_two, Board_LED1, pinValue_two );
    action = ACTIVATE;

    // JTKJ: Exercise 1. Blink either led of the device
}

void buttonFxn(PIN_Handle handle, PIN_Id pinId) {

    // JTKJ: Tehtävä 1. Vilkuta jompaa kumpaa lediä
    uint_t pinValue = PIN_getOutputValue( Board_LED0 );
    pinValue = !pinValue;
    PIN_setOutputValue( ledHandle, Board_LED0, pinValue );
    action = ACTIVATE;

    // JTKJ: Exercise 1. Blink either led of the device
}

/* Task Functions */
Void uartTaskFxn(UArg arg0, UArg arg1) {

    // JTKJ: Tehtävä 4. Lisää UARTin alustus: 9600,8n1
    UART_Handle uart;
    UART_Params uartParams;

    // Alustetaan sarjaliikenne
    UART_Params_init(&uartParams);
    uartParams.writeDataMode = UART_DATA_TEXT;
    uartParams.readDataMode = UART_DATA_TEXT;
    uartParams.readEcho = UART_ECHO_OFF;
    uartParams.readMode= UART_MODE_BLOCKING;
    uartParams.baudRate = 9600; // nopeus 9600baud
    uartParams.dataLength = UART_LEN_8; // 8
    uartParams.parityType = UART_PAR_NONE; // n
    uartParams.stopBits = UART_STOP_ONE; // 1s

    // Avataan yhteys laitteen sarjaporttiin vakiossa Board_UART0
    uart = UART_open(Board_UART0, &uartParams);
        if (uart == NULL) {
            System_abort("Error opening the UART");
    }

    // JTKJ: Exercise 4. Setup here UART connection as 9600,8n1
    // char lux[100], eat[100], pet[100], exercise[100], activate[100];
    sprintf(eat, "id:%d,EAT:%d", 3232, 4);
    sprintf(pet, "id:%d,PET:%d", 3232, 2);
    sprintf(exercise, "id:%d,EXERCISE:%d", 3232, 2);
    sprintf(activate, "id:%d,ACTIVATE:%d;%d;%d", 3232, 1,2,3);
    while (1) {
        if(programState == DATA_READY){
            // OPT
            // UART_write(uart, ping, strlen(ping) + 1);
            // MPU
            switch(action){
            case EAT:
                 System_printf("EAT");
                 UART_write(uart, eat, strlen(eat) + 1);
                 break;
            case PET:
                 System_printf("PET");
                 UART_write(uart, pet, strlen(pet) + 1);
                 break;
            case EXERCISE:
                 System_printf("EXE");
                 UART_write(uart, exercise, strlen(exercise) + 1);
                 break;
            case ACTIVATE:
                 System_printf("ACT");
                 UART_write(uart, activate, strlen(activate) + 1);
                 break;
            case IDLE:
                 System_printf("IDLE");
                 break;
            default:
                break;
            }
            action = IDLE;
            // UART_write(uart, lux, strlen(lux) + 1);
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

    // WHILE
    while (1) {
        if(programState == WAITING){
        // OPT
        i2c = I2C_open(Board_I2C, &i2cParams);
            if (i2c == NULL) {
                System_abort("Error Initializing I2C\n");
        }
        ambientLight = opt3001_get_data(&i2c);



        I2C_close(i2c);
        Task_sleep(800000 / Clock_tickPeriod);
        // MPU
        i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
            if (i2cMPU == NULL) {
                System_abort("Error Initializing I2CMPU\n");
        }
        // MPU:n getData kutsu
        mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);

        if(ax >= 0.2 || ax <= -0.2){
            action = EXERCISE;
        }
        if(ay >= 0.2 || ay <= -0.2){
            action = EAT;
        }
        if(az <= -1.2 || az >= 0.8){
            action = PET;
        }

        I2C_close(i2cMPU);
        programState = DATA_READY;
        // LOPPU ODOTUS
        Task_sleep(1000000 / Clock_tickPeriod);
    }
}
}


Int main(void) {

    // Task variables
    Task_Handle sensorTaskHandle;
    Task_Params sensorTaskParams;
    Task_Handle uartTaskHandle;
    Task_Params uartTaskParams;

    // Initialize board
    Board_initGeneral();


    // JTKJ: Tehtävä 2. Ota i2c-väylä käyttöön ohjelmassa
    Board_initI2C();
    // JTKJ: Exercise 2. Initialize i2c bus
    // JTKJ: Tehtävä 4. Ota UART käyttöön ohjelmassa
    // Otetaan sarjaportti käyttöön ohjelmassa
    Board_initUART();
    // JTKJ: Exercise 4. Initialize UART

    // JTKJ: Tehtävä 1. Ota painonappi ja ledi ohjelman käyttöön. Muista rekisteröidä keskeytyksen käsittelijä painonapille
    // Ledi käyttöön ohjelmassa

    ledHandle = PIN_open( &ledState, ledConfig );
    if(!ledHandle) {
       System_abort("Error initializing LED pin\n");
    }

    // Painonappi käyttöön ohjelmassa
    buttonHandle = PIN_open(&buttonState, buttonConfig);
    buttonHandle_two = PIN_open(&buttonState_two, buttonConfig_two);
    if(!buttonHandle || !buttonHandle_two) {
       System_abort("Error initializing button pin\n");
    }

    // Painonapille keskeytyksen käsittellijä
    if (PIN_registerIntCb(buttonHandle, &buttonFxn) != 0) {
       System_abort("Error registering button callback function");
    }
    if (PIN_registerIntCb(buttonHandle_two, &buttonFxnTwo) != 0) {
         System_abort("Error registering button callback function");
      }

    // Open MPU power pin
    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
    if (hMpuPin == NULL) {
        System_abort("Pin open failed!");
    }

    /* Task */
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

    /* Sanity check */
    System_printf("Hello world!\n");
    System_flush();

    /* Start BIOS */
    BIOS_start();

    return (0);
}
