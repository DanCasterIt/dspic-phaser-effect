/* Constant Definitions */
#define FOSC    (80000000)
#define FP (FOSC/2)
#define BAUDRATE 19600
#define BRGVAL ((FP/BAUDRATE)/16)-1
#define DMA_BUFFER_LENGHT 128
#define DMA_BUFFER_LENGHT_LOG (log(DMA_BUFFER_LENGHT) / log(2))
#define FFTTWIDCOEFFS_IN_PROGMEM 0
#define HANNING_WINDOW 1
//------------DEBUG DEFINES------------
#define TRANSFORM 1 //0 FOR TEST MODE (playback device)