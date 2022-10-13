/**
 * This program reads a WAV audio file and decript the message contained in the audio file. The command line
 * specifies the number of least significant bits used for decription, file name, and output file name. The decripted message will
 * be written in the output file name. If the file name is not provided or the file is not readable, the program will 
 * exit and provide an error message. If the least significant bits number is not valid, the output file will be full of unreadable characters.
 * A summary message will also be printed in terminal about the number of characters and samples recovered.
 *
 * @author Runhao Huang huanr20@wfu.edu
 * @date Oct. 8, 2022
 * @assignment Lab 5  
 * @course CSC 250
 **/

#include <stdio.h>
#include <stdlib.h>  
#include <string.h>  
#include <math.h>  
#include "getWavArgs.h"

#define MAX_STRING 26

int readWavReader(FILE* inFile, short *sampleSizePtr, int *numSamplesPtr, int *sampleRatePtr, short *numOfChannel);
int readWavData(FILE* inFile, int bit, short sampleSize, int numSamples, int sampleRate, short numOfChannel, int lSB, char textFileName[]);

/*
* The main function contains the validation of the wav file through calling the checking function in getWavArgs.h and opening the wav file. After validation,
* it calls the function that handles the chunks in the file before data chunk and get general information of the file. If the format is 
* correct, it will then call the function that reads through data in the file data chunk and decript the message in samples.
*/

int main(int argc, char *argv[]) {
    FILE *inFile;      /* WAV file */
    short sampleSize;  /* size of an audio sample (bits) */
    int sampleRate;    /* sample rate (samples/second) */
    int numSamples;    /* number of audio samples */ 
    int wavOK = 0;     /* 1 if the WAV file si ok, 0 otherwise */
    short numOfChannel; /* Number of channels in the file. */
    int bit;           /* Assigned least significant bits in the file. */
    int argsOK = 0;    /* 1 if the command line argument is ok, 0 otherwise */
    char waveFileName [MAX_STRING];  /* The wave file name provided in the command line argument */
    char textFileName [MAX_STRING];  /* The output file name provided in the command line argument */

    /* Call funtion from getWavArgs.h to check the validity of argument and store wave file name and output file name. */
    argsOK = getWavArgs(argc,argv, &bit, waveFileName, textFileName);

    /* If the command line argument is not valid. */
    if(argsOK ==0) {
        return 1;
    }

    inFile = fopen(waveFileName, "rbe");

    /* If the wave file can't be open. */
    if(!inFile) {
        printf("could not open wav file %s \n", argv[1]);
        return 2;
    }

    /* Call the function to read the chuncks before data chunk. */
    wavOK = readWavReader(inFile, &sampleSize, &numSamples, &sampleRate, &numOfChannel);

    /* If wave file format is wrong. */
    if(!wavOK) {
       printf("wav file %s has incompatible format \n", argv[1]);   
       return 3;
    }
    else {
        /* Call the funtion to read the data chunck and decript the message. */
        readWavData(inFile,bit, sampleSize, numSamples, sampleRate, numOfChannel, bit, textFileName);
        
    }
    if(inFile) fclose(inFile);
    return 0;
}


/**
 *  function reads the RIFF, fmt, and start of the data chunk. 
 */
int readWavReader(FILE* inFile, short *sampleSizePtr, int *numSamplesPtr, int *sampleRatePtr, short *numOfChannel) {
    char chunkId[] = "    ";  /* chunk id, note initialize as a C-string */
    char data[] = "    ";      /* chunk data */
    int chunkSize = 0;        /* number of bytes remaining in chunk */
    short audioFormat = 0;    /* audio format type, PCM = 1 */
    short numChannels = 0;    /* number of audio channels */ 
    int sampleRate = 0;       /* Audio samples per second */ 
    int byteRate = 0;         /* SampleRate × NumChannels × BitsPerSample/8 */ 
    short blockAlign = 0;     /* The number of bytes for one sample including all channels. */
    short bitsPerSample = 0;  /* Number of bits used for an audio sample. */
    int numSampleByte = 0;    /* This is the number of bytes of data (audio samples). */

    /* first chunk is the RIFF chunk, let's read that info */  
    fread(chunkId, 1, 4, inFile);
    fread(&chunkSize, 1, 4, inFile);
    fread(data, 1, 4, inFile);

    /* let's try to read the next chunk, it always starts with an id */
    fread(chunkId, 1, 4, inFile);
    /* if the next chunk is not "fmt " then let's skip over it */  
    while(strcmp(chunkId, "fmt ") != 0) {
        fread(&chunkSize, 1, 4, inFile);
        /* skip to the end of this chunk */  
        fseek(inFile, chunkSize, SEEK_CUR);
        /* read the id of the next chuck */  
        fread(chunkId, 1, 4, inFile);
    }  

    /* if we are here, then we must have the fmt chunk, now read that data */  
    fread(&chunkSize, 1, 4, inFile);
    fread(&audioFormat, 1,  sizeof(audioFormat), inFile);
    fread(&numChannels, 1,  sizeof(numChannels), inFile);
    fread(&sampleRate, 1,  sizeof(sampleRate), inFile);
    fread(&byteRate, 1,  sizeof(byteRate), inFile);
    fread(&blockAlign, 1,  sizeof(blockAlign), inFile);
    fread(&bitsPerSample, 1,  sizeof(bitsPerSample), inFile);

    /* let's try to read the next chunk, it always starts with an id */
    fread(chunkId, 1, 4, inFile);
    /* if the next chunk is not "data" then let's skip over it */  
    while(strcmp(chunkId, "data") != 0) {
        fread(&chunkSize, 1, 4, inFile);
        /* skip to the end of this chunk */  
        fseek(inFile, chunkSize, SEEK_CUR);
        /* read the id of the next chuck */  
        fread(chunkId, 1, 4, inFile);
    } 

    /* Read the size of the data chunk here */
    fread(&numSampleByte, 1, 4, inFile);
    *numSamplesPtr = (numSampleByte*8/numChannels)/bitsPerSample;
    *sampleSizePtr = (numSampleByte/(*numSamplesPtr))*8;
    *sampleRatePtr = sampleRate;
    *numOfChannel = numChannels;

    return (audioFormat == 1);
}


/**
 *  Function reads the WAV audio data (last part of the data chunk) and decript the message by using the number of least significant bits. 
 *  Write the message to the output file. Print a brief summary of the message recovered.
 */
int readWavData(FILE* inFile, int bit, short sampleSize, int numSamples, int sampleRate, short numOfChannel, int lSB, char textFileName[]) {
    char ch = 0;     /* Current character in the loop */ 
    char prevCh = 0; /* Previous character in the loop */ 
    int sample = 0;  /* Current value in the loop */ 
    int bytes_per_sample = sampleSize / (8 * numOfChannel);  /* Number of bytes per sample */ 
    int i;
    int j;
    int k = 0;      /* Number of characters in the message. */  
    int numChar = 0;  /* Number of maximum characters in the samples */ 
    int loopNum = 0;  /* Number of sub samples constructing a character */ 
    short mask = 0;   /* Mask of bit operation */ 
    char temp = 0;    /* Temperary character in the loop */ 
    FILE *fp = fopen(textFileName, "we");
    
    if (numOfChannel==2) {
        numSamples *= 2;
    }

    /* Number of maximum characters we could read from the samples. */  
    numChar = numSamples / (8 / lSB);
    loopNum = 8 / lSB;

    /* Hard coded mask */  
    if (lSB == 1) {
        mask = 1;
    }
    else if (lSB == 2) {
        mask = 3;
    }
    else {
        mask = 15;
    }
    mask = (char)mask;

    for (i = 0; i < numChar; i++) {
        for (j = 0; j < loopNum; j++) {
            fread(&sample, 1, bytes_per_sample, inFile);
            if(bytes_per_sample == 2) {
                sample = (short) sample;
            }
            else if(bytes_per_sample == 4) {
                sample = (int) sample;
            }
            if (j == 0) {
                ch = (sample & mask) << (8-lSB);
            }
            else {
                temp = (sample & mask) << ((loopNum-j-1)*lSB);
                ch = ch | temp;
            }
        }
        /* Write the characters into the output file. */  
        fprintf(fp, "%c", ch);
        k += 1;
        /* The loop is ended if the previous character is : and current character is ). */ 
        if (ch ==')' && i!=0) {
            if (prevCh ==':'){
                break;
            }
        }
        prevCh = ch;
        ch = 0;
    }

    /* Print the brief summary to the terminal. */ 
    printf("%d characters recovered from %d samples\n", k, (k*(8/lSB)));
    fclose(fp);
    return 1;
}


