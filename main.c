
#include "tinywav.h"
#include <math.h>
#include <assert.h>
#include <stdlib.h>

#define _USE_MATH_DEFINES
#define NUM_CHANNELS 1
#define SAMPLE_RATE 44100
#define BLOCK_SIZE 256
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

int main(int argc, char *argv[]) {
  char* outputPath = "output.wav";
  
  if (argc < 3) return -1;
  const char *inputPath = argv[1];

  TinyWav twReader;
  tinywav_open_read(&twReader, inputPath, TW_INLINE);

  if (twReader.numChannels != NUM_CHANNELS || twReader.h.SampleRate != SAMPLE_RATE) {
      printf("Supplied test wav has wrong format - should be [%d]channels, fs=[%d]\n", NUM_CHANNELS, SAMPLE_RATE);
      return -1;
  }

  TinyWav twWriter;
  tinywav_open_write(&twWriter, NUM_CHANNELS, SAMPLE_RATE, TW_FLOAT32, TW_INLINE, outputPath);

  int totalNumSamples = twReader.numFramesInHeader;
  int samplesProcessed = 0;
  

  // temp float pointers for reallocation
  float *temp1,*temp2;

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  //WSOLA__block(float *buffer, 44100, )

  // main buffer, init as block size times 8 to prevent overflow
  // full of 0's
  float *main_buffer;
  main_buffer = malloc(sizeof(float) * BLOCK_SIZE * 2);
  int i;
  for (i = 0; i < BLOCK_SIZE * 2; i++) {
    main_buffer[i] = 0;
  }

  // buffer counter 
  int block_count = 0;

  // wsola parameters
  int tolerance = 128;
  double shift_percent = atof(argv[2]);
  double length_factor = 100 / shift_percent;

  // end buffer for post wsola proccess, init 8*BLOCK_Size 0's
  float *output_buffer;
  
  
      
  //generate hanning window
  float hann[BLOCK_SIZE] = {0};  
  for (i = 0; i < BLOCK_SIZE; i++) {
    hann[i] = pow(sin((i * M_PI) / BLOCK_SIZE), 2);
  }
 
  // wsola paramater variables
  int window_idx = 1;
  int delta = 0;
  int win_size = floor(BLOCK_SIZE * length_factor);
  int s = 0;

  while (samplesProcessed < totalNumSamples) {
    // reallocations
    
    int temp1_size;
    temp1_size = BLOCK_SIZE * (4+block_count);
    
    temp1 = realloc(main_buffer,sizeof(float) * temp1_size);
    
    
    if (temp1 == NULL) {
      // problem reallocating
      free(main_buffer);
      exit(1);

    }
    else {
      main_buffer = temp1;
    }
    float buffer[NUM_CHANNELS * BLOCK_SIZE];
    
    int samplesRead = tinywav_read_f(&twReader, buffer, BLOCK_SIZE);
    assert(samplesRead > 0 && " Could not read from file!");

    // copy input block to main buffer based on block count
    for (i = 0; i < BLOCK_SIZE; i++) {
        main_buffer[BLOCK_SIZE * block_count + i + BLOCK_SIZE ] = buffer[i];        
    }
    block_count++;
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    samplesProcessed += samplesRead * NUM_CHANNELS;
    
  }
  // write to file AFTER read (not real time)

    //input 1 block ahead of wsola start
    

    // start wsola on main_buffer
    // START WSOLA LOOP
    int out_size = sizeof(float) * (floor(totalNumSamples * (1 / length_factor)) + BLOCK_SIZE);
    output_buffer = malloc(out_size);
    int output_win_pos = (window_idx - 1) * (BLOCK_SIZE / 2);
    
    while (s<totalNumSamples) {
      for (i = 0; i < BLOCK_SIZE; i++) {
          output_buffer[output_win_pos + i] += hann[i] * main_buffer[s + delta + i];
      }
    
      //calculate cross correlation of the windows
      float frame_adj[BLOCK_SIZE], frame_next[BLOCK_SIZE * 2];
      
      for (i = 0; i < BLOCK_SIZE; i++) {
        frame_adj[i] = main_buffer[s + delta + i + BLOCK_SIZE/2];
      }
      
      s = s + floor(BLOCK_SIZE / 2 * length_factor);
      for (i = 0; i < BLOCK_SIZE * 2; i++) {
        if (s - tolerance + i < 0) {
          frame_next[i] = 0;
        }
        else {
          frame_next[i] = main_buffer[s - tolerance + i];
        }
      }
      
      // cc frame_adj and frame_next
      float CC_result[BLOCK_SIZE * 3 - 1] = {0};
      float next_rev[BLOCK_SIZE * 2] = {0};
      for (i = 0; i < BLOCK_SIZE * 2;i++) {
        next_rev[i] = frame_next[BLOCK_SIZE * 2 - (i+1)];
      }
      int j = 0;

      for (i = 0; i < BLOCK_SIZE * 3 - 1; i++) {
        int adj_start = MAX(0,i - BLOCK_SIZE * 2 + 1);
        int adj_end = MIN(i + 1, BLOCK_SIZE);
        int next_start = MIN(i, BLOCK_SIZE * 2 - 1);
        for (j = adj_start; j < adj_end; j++) {
          CC_result[i] += next_rev[next_start--] * frame_adj[j];
        }
      }
     
      //find max value of CC_result
      float max = 0;
      int max_index = 0;
      for (i = BLOCK_SIZE; i < BLOCK_SIZE * 2; i++) {
        if (CC_result[i] > max) {
          max = CC_result[i];
          max_index = i-BLOCK_SIZE;
        }
      }
      
      delta = tolerance - max_index;  
      window_idx++;
      output_win_pos = (window_idx - 1) * (BLOCK_SIZE / 2);
    }
    // END WSOLA LOOP

    // resample to original window size (TO DO)

    // write end buffer to file
    
  samplesProcessed = 0;
  int pos = 0;
  while (samplesProcessed < floor(totalNumSamples*(1/length_factor))) {
    
    float buffer[NUM_CHANNELS * BLOCK_SIZE];
    
    for (i = 0; i < BLOCK_SIZE; i++) {
      buffer[i] = output_buffer[i+pos];  
    }  
    
    int samplesWritten = tinywav_write_f(&twWriter, buffer, BLOCK_SIZE);
    assert(samplesWritten > 0 && "Could not write to file!");    
    samplesProcessed += samplesWritten * NUM_CHANNELS;
    pos += BLOCK_SIZE;
    
  }
  
  
  free(main_buffer);
  free(output_buffer);
   
  tinywav_close_read(&twReader);
  tinywav_close_write(&twWriter);

  return 0;
}



 
