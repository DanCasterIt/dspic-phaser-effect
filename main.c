/*
 * File:   main.c
 * Author: daniele
 *
 * Created on 17 giugno 2017, 20.21
 */

/* DSPIC33FJ128GP802 Configuration Bit Settings */
// FOSCSEL
#pragma config FNOSC = FRC              // Oscillator Mode (Internal Fast RC (FRC))
// FOSC
#pragma config POSCMD = NONE            // Primary Oscillator Source (Primary Oscillator Disabled)
#pragma config OSCIOFNC = OFF           // OSC2 Pin Function (OSC2 pin has clock out function)
#pragma config IOL1WAY = ON             // Peripheral Pin Select Configuration (Allow Only One Re-configuration)
#pragma config FCKSM = CSECMD           // Clock Switching and Monitor (Clock switching is enabled, Fail-Safe Clock Monitor is disabled)
// FWDT
#pragma config FWDTEN = OFF             // Watchdog Timer Enabled/disabled by user software
// (LPRC can be disabled by clearing SWDTEN bit in RCON register

#include "main.h"
#include <dsp.h>
#include <xc.h>
#include <libpic30.h>
#include <math.h>
#include <stdio.h>
#include <p33FJ128GP802.h>

#define M_PI PI
#define M_PI_2 (PI/2)

/* Global Definitions */
#if TRANSFORM
#if HANNING_WINDOW
fractional window[DMA_BUFFER_LENGHT] __attribute__((space(ymemory), aligned(DMA_BUFFER_LENGHT * 2)));
#endif

fractcomplex sigCmpx[DMA_BUFFER_LENGHT] __attribute__((space(ymemory), far, aligned(DMA_BUFFER_LENGHT * 2 * 2)));
/* Typically, the input signal to an FFT  */
/* routine is a complex array containing samples */
/* of an input signal. For this example, */
/* we will provide the input signal in an */
/* array declared in Y-data space. */

#if FFTTWIDCOEFFS_IN_PROGMEM
fractcomplex twiddleFactors_FFT[DMA_BUFFER_LENGHT / 2] /* Declare Twiddle Factor array in X-space*/
__attribute__((section(".xbss, bss, xmemory"), aligned(DMA_BUFFER_LENGHT * 2)));
fractcomplex twiddleFactors_IFFT[DMA_BUFFER_LENGHT / 2] /* Declare Twiddle Factor array in X-space*/
__attribute__((section(".xbss, bss, xmemory"), aligned(DMA_BUFFER_LENGHT * 2)));
#else
extern const fractcomplex twiddleFactors_FFT[DMA_BUFFER_LENGHT / 2] /* Twiddle Factor array in Program memory */
__attribute__((space(auto_psv), aligned(DMA_BUFFER_LENGHT * 2)));
extern const fractcomplex twiddleFactors_IFFT[DMA_BUFFER_LENGHT / 2] /* Twiddle Factor array in Program memory */
__attribute__((space(auto_psv), aligned(DMA_BUFFER_LENGHT * 2)));
#endif
#endif

unsigned int RxDmaBuffer = 0, TxDmaBuffer = 0, count = 0;


fractional RxBufferA[DMA_BUFFER_LENGHT] __attribute__((space(dma)));
fractional RxBufferB[DMA_BUFFER_LENGHT] __attribute__((space(dma)));

fractional TxBufferA[DMA_BUFFER_LENGHT] __attribute__((space(dma)));
fractional TxBufferB[DMA_BUFFER_LENGHT] __attribute__((space(dma)));

void oscillator_setup(void);
void uart_setup(void);
void adc_setup(void);
void dac_setup(void);
void dma_setup(void);
#if !TRANSFORM
void test(fractional *in, fractional *out);
#else
void transform(fractional *in, fractional *realNumbers);
#endif
float atan2_approximation2(float y, float x);
void __attribute__((interrupt, no_auto_psv)) _DMA0Interrupt(void);

int main(void) {
    oscillator_setup();
    uart_setup();
    printf("RESET\n\r");
#if !TRANSFORM
    printf("PLAYBACK (TEST) MODE\n\r");
#else
    printf("TRANSFORM MODE\n\r");
    printf("Twiddle Factors ");
#if FFTTWIDCOEFFS_IN_PROGMEM
    printf("in RAM X-space\n\r");
#else
    printf("in program memory\n\r");
#endif
#if HANNING_WINDOW
    printf("Hanning windowing enabled\n\r");
#endif
#endif
    dma_setup();
    adc_setup();
    dac_setup();

#if TRANSFORM
#if FFTTWIDCOEFFS_IN_PROGMEM					/* Generate TwiddleFactor Coefficients */
    TwidFactorInit(DMA_BUFFER_LENGHT_LOG, &twiddleFactors_FFT[0], 0); /* We need to do this only once at start-up */
    TwidFactorInit(DMA_BUFFER_LENGHT_LOG, &twiddleFactors_IFFT[0], 1); /* We need to do this only once at start-up */
#endif
#if HANNING_WINDOW
    HanningInit(DMA_BUFFER_LENGHT, &window[0]);
#endif
#endif

    DMA0CONbits.CHEN = 1; // Enable DMA Channe 0
    DMA1CONbits.CHEN = 1; // Enable DMA Channe l

    AD1CON1bits.ADON = 1; // Turn on the A/D converter
    DAC1CONbits.DACEN = 1; // DAC1 Module Enabled

    while (1);
}

void oscillator_setup(void) {
    // Configure PLL prescaler, PLL postscaler, PLL divisor
    PLLFBD = 41; // M = 43
    CLKDIVbits.PLLPOST = 0; // N2 = 2
    CLKDIVbits.PLLPRE = 0; // N1 = 2
    OSCTUN = 0; // Tune FRC oscillator, if FRC is used
    // Disable Watch Dog Timer
    RCONbits.SWDTEN = 0;
    // Initiate Clock Switch to Internal FRC with PLL (NOSC = 0b001)
    __builtin_write_OSCCONH(0x01);
    __builtin_write_OSCCONL(0x01);
    // Wait for Clock switch to occur
    while (OSCCONbits.COSC != 0b001);
    // Wait for PLL to lock
    while (OSCCONbits.LOCK != 1) {
    };
}

void uart_setup(void) {
    AD1PCFGL = 0xFFFF; //all pins as digital
    TRISBbits.TRISB3 = 0; // TX as output
    TRISBbits.TRISB2 = 1; // RX as input

    RPINR18bits.U1RXR = 2; //U1RX on RP2 pin
    RPOR1bits.RP3R = 0b00011; //U1TX on RP3 pin

    U1MODEbits.STSEL = 0; // 1-stop bit
    U1MODEbits.PDSEL = 0; // No Parity, 8-data bits
    U1MODEbits.ABAUD = 0; // Auto-Baud disabled
    U1MODEbits.BRGH = 0; // Standard-Speed mode
    U1BRG = BRGVAL; // Baud Rate setting for 9600
    U1MODEbits.UARTEN = 1; // Enable 
    U1STAbits.UTXEN = 1; // Enable UART TX
    __C30_UART = 1;
}

void adc_setup(void) {
#if !TRANSFORM
    AD1CON1bits.FORM = 0b00; // Data Output Format: Unsigned Integer
#else
    AD1CON1bits.FORM = 0b11; // Data Output Format: Signed fractional
#endif
    AD1CON1bits.SSRC = 0b111; // (7) Internal Counter (SAMC) ends sampling and starts conversion
    AD1CON1bits.ASAM = 1; // ADC Sample Control: Sampling begins immediately after conversion
    AD1CON1bits.AD12B = 1; // 12-bit ADC operation
    AD1CON2bits.CHPS = 0b00; // Converts channel CH0
    AD1CON2bits.CSCNA = 0; // Do not scan input
    // Clock
    AD1CON3bits.ADRC = 0; // ADC Clock is derived from Systems Clock
    AD1CON3bits.SAMC = 3; // Auto Sample Time = 3 * TAD
    AD1CON3bits.ADCS = 63; // (64 - 1) ADC Conversion Clock
    // Initialize MUXA Input Selection
    AD1CHS0bits.CH0SA = 0b00000; // Select AN0 for CH0 +ve input
    AD1CHS0bits.CH0NA = 0; // Select VREF- for CH0 -ve input
    // Port Configuration
    AD1PCFGL = 0xFFFF; // all ANx pin as digital
    AD1PCFGLbits.PCFG0 = 0; // AN0 as Analog Input
    // Set up ADC1 for DMA operation :
    AD1CON2bits.BUFM = 0; // Start fillng the buffer from the start address
    AD1CON1bits.ADDMABM = 1; // DMA buffers are filled in conversion order
    AD1CON2bits.SMPI = 0; // 1 ADC buffer
    AD1CON4bits.DMABL = DMA_BUFFER_LENGHT_LOG;
    // Set up ADC1 interrupts
    IFS0bits.AD1IF = 0; // Clear the A/D interrupt flag bit
    IEC0bits.AD1IE = 0; // Do Not Enable A/D interrupt

    //AD1CON1bits.ADON = 1; // Turn on the A/D converter
}

void dac_setup(void) {
    DAC1STATbits.ROEN = 1; // Right Channel DAC Output Enabled
    DAC1CONbits.FORM = 1; // Data Format is signed integer
    DAC1CONbits.AMPON = 0; // Analog Output Amplifier is enabled during Sleep Mode/Stop-in Idle mode
    DAC1DFLT = 0x8000; // DAC Default value is the midpoint
    // Initiate DAC Clock
    ACLKCONbits.SELACLK = 0; // FRC w/ Pll as Clock Source
    ACLKCONbits.AOSCMD = 0; // Auxiliary Oscillator Disabled
    ACLKCONbits.ASRCSEL = 0; // Auxiliary Oscillator is the Clock Source
    ACLKCONbits.APSTSCLR = 0b111; // (7) Auxiliary Clock Output Divider (111 = divided by 1)
    DAC1CONbits.DACFDIV = 16; // (17 - 1) DAC Clock Divider bits
    // Set up DAC1 interrupts
    DAC1STATbits.REMPTY = 0;
    DAC1STATbits.RFULL = 0;

    //DAC1CONbits.DACEN = 1; // DAC1 Module Enabled
}

void dma_setup(void) {
    //Set up DMA Channel 0 for Receive in Continuous Ping-Pong mode:
    DMA0CONbits.AMODE = 0b00; // Configure DMA for Peripheral indirect mode
    DMA0CONbits.MODE = 0b10; // Configure DMA for Continuous Ping-Pong mode
    DMA0CONbits.DIR = 0; // Transfer Direction: peripheal to DPSRAM
    DMA0CONbits.SIZE = 0; // Data Transfer size: word (16-bit)
    DMA0PAD = (volatile unsigned int) &ADC1BUF0; // Point DMA to ADC1BUF0
    DMA0CNT = DMA_BUFFER_LENGHT - 1; // (2 buffers, each with DMA_BUFFER_LENGHT words)
    DMA0REQ = 0b0001101; // (13) Select ADC1 as DMA Request source
    DMA0STA = __builtin_dmaoffset(&RxBufferA);
    DMA0STB = __builtin_dmaoffset(&RxBufferB);
    IFS0bits.DMA0IF = 0; //Clear the DMA interrupt flag bit
    IEC0bits.DMA0IE = 1; //Set the DMA interrupt enable bit

    //DMA0CONbits.CHEN = 1; // Enable DMA

    //Set up DMA Channel 1 for Transmit in Continuous Ping-Pong mode:
    DMA1CONbits.AMODE = 0; // Configure DMA for register indirect mode
    DMA1CONbits.MODE = 0b10; // Configure DMA for Continuous Ping-Pong mode
    DMA1CONbits.DIR = 1; // Transfer Direction: DPSRAM to peripheal
    DMA1CONbits.SIZE = 0; // Data Transfer size: word (16-bit)
    DMA1PAD = (volatile unsigned int) &DAC1RDAT;
    DMA1CNT = DMA_BUFFER_LENGHT - 1;
    DMA1REQ = 0b1001110; // (78) Select DAC1 right as DMA Request source
    DMA1STA = __builtin_dmaoffset(TxBufferA);
    DMA1STB = __builtin_dmaoffset(TxBufferB);
    IFS0bits.DMA1IF = 0; // Clear DMA Interrupt Flag
    IEC0bits.DMA1IE = 0; // Disable DMA interrupt

    //DMA1CONbits.CHEN = 1; // Enable DMA Channel
}

void __attribute__((interrupt, no_auto_psv)) _DMA0Interrupt(void) {
    // Switch between Primary and Secondary Ping-Pong buffers
    IFS0bits.DMA0IF = 0; //Clear the DMA0 Interrupt Flag (ADC)
    IFS0bits.DMA1IF = 0; //Clear the DMA1 Interrupt Flag (DAC)
    if (RxDmaBuffer == 0) {
        // Notify application that TxBufferA has been received
#if !TRANSFORM
        test(RxBufferA, TxBufferA);
#else
        transform(RxBufferA, TxBufferA);
#endif
    } else {
        //Notify application that TxBufferB has been received
#if !TRANSFORM
        test(RxBufferB, TxBufferB);
#else
        transform(RxBufferB, TxBufferB);
#endif
    }
    RxDmaBuffer ^= 1;
}

#if !TRANSFORM

void test(fractional *in, fractional *out) {
    int i;
    for (i = 0; i < DMA_BUFFER_LENGHT; i++) {
        out[i] = (int) (8.00170940170940170 * in[i]);
    }
}
#else

void transform(fractional *in, fractional *realNumbers) {
    int i;
    fractcomplex module[DMA_BUFFER_LENGHT];
    fractional phase[DMA_BUFFER_LENGHT];
    fractional buff[DMA_BUFFER_LENGHT];
    VectorScale(DMA_BUFFER_LENGHT, buff, in, Float2Fract(0.2667198489));
    for (i = 0; i < DMA_BUFFER_LENGHT; i++) {
        sigCmpx[i].real = buff[i];
        sigCmpx[i].imag = 0;
    }
#if HANNING_WINDOW
    VectorWindow(DMA_BUFFER_LENGHT, &sigCmpx[DMA_BUFFER_LENGHT].real, &sigCmpx[DMA_BUFFER_LENGHT].real, (fractional*) & window[0]);
#endif
    /* Perform FFT operation */
#if FFTTWIDCOEFFS_IN_PROGMEM
    FFTComplexIP(DMA_BUFFER_LENGHT_LOG, &sigCmpx[0], &twiddleFactors_FFT[0], COEFFS_IN_DATA);
#else
    FFTComplexIP(DMA_BUFFER_LENGHT_LOG, &sigCmpx[0], (fractcomplex *) __builtin_psvoffset(&twiddleFactors_FFT[0]), (int) __builtin_psvpage(&twiddleFactors_FFT[0]));
#endif
    /* Store output samples in bit-reversed order of their addresses */
    BitReverseComplex(DMA_BUFFER_LENGHT_LOG, &sigCmpx[0]);
    
    //PHASER CODE
	SquareMagnitudeCplx(DMA_BUFFER_LENGHT, &sigCmpx[0], &module[0].real);
    for(i = 0; i < DMA_BUFFER_LENGHT; i++)  {
        //phase[i] = (fractional) atan2_approximation2((float) sigCmpx[i].imag, (float) sigCmpx[i].real);
        phase[i] = (fractional) atan2((float) sigCmpx[i].imag, (float) sigCmpx[i].real);
        
        //LFO TO PHASE
    
        sigCmpx[i].real = (fractional) ((float) module[i].real * cosf((float) phase[i]));
        sigCmpx[i].imag = (fractional) ((float) module[i].real * sinf((float) phase[i]));
    }

    /* Perform IFFT operation */
#if FFTTWIDCOEFFS_IN_PROGMEM
    IFFTComplexIP(DMA_BUFFER_LENGHT_LOG, &sigCmpx[0], &twiddleFactors_IFFT[0], COEFFS_IN_DATA);
#else
    IFFTComplexIP(DMA_BUFFER_LENGHT_LOG, &sigCmpx[0], (fractcomplex *) __builtin_psvoffset(&twiddleFactors_IFFT[0]), (int) __builtin_psvpage(&twiddleFactors_IFFT[0]));
#endif
    //FFTComplexIP() scales the signal by 1/N. This N has to be re-multiplied.
    for (i = 0; i < DMA_BUFFER_LENGHT; i++) sigCmpx[i].real = sigCmpx[i].real * DMA_BUFFER_LENGHT;

    for (i = 0; i < DMA_BUFFER_LENGHT; i++) realNumbers[i] = sigCmpx[i].real;
    VectorScale(DMA_BUFFER_LENGHT * 2, realNumbers, realNumbers, Float2Fract(0.9999694824));
    for (i = 0; i < DMA_BUFFER_LENGHT; i++) realNumbers[i] = realNumbers[i] << 1;
}
#endif

#define PI_FLOAT     3.14159265f
#define PIBY2_FLOAT  1.5707963f
// |error| < 0.005
float atan2_approximation2( float y, float x )
{
	if ( x == 0.0f )
	{
		if ( y > 0.0f ) return PIBY2_FLOAT;
		if ( y == 0.0f ) return 0.0f;
		return -PIBY2_FLOAT;
	}
	float atan;
	float z = y/x;
	if ( fabs( z ) < 1.0f )
	{
		atan = z/(1.0f + 0.28f*z*z);
		if ( x < 0.0f )
		{
			if ( y < 0.0f ) return atan - PI_FLOAT;
			return atan + PI_FLOAT;
		}
	}
	else
	{
		atan = PIBY2_FLOAT - z/(z*z + 0.28f);
		if ( y < 0.0f ) return atan - PI_FLOAT;
	}
	return atan;
}