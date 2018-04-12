#include <TMRpcm.h>

#include <SPI.h>
#include <SD.h>

#define numReadings 10          // average number of readings to take for each input in order to smooth the results
#define jitterThreshold  5      // defines the range either side of the target value that will count as a valid reading (helps with old or noisy potentiometers)
#define numInputs 2             // number of potentiometers being read
#define maxSwitchReading 511    // max analog reading from the potentiometer
int readIndex = 0;              // the index of the current reading

bool audioAvailable = 1;        // defines if the SD card is available to play audio
TMRpcm audioPlayer;
#define SD_ChipSelectPin 53


struct physicalInputKnob
{
   int analogPin;
   int numPositions;
   int lastPositionReading;
   int averageReadingsSet[numReadings];
   int averageReadingsTotal;
   int averageReading;
};
typedef struct physicalInputKnob physical_input_knob_t;

physical_input_knob_t inputs[numInputs];

void setup() {
  // initialize serial communication with computer:
  Serial.begin(9600);
  audioPlayer.speakerPin = 11; //5,6,11 or 46 on Mega, 9 on Uno, Nano, etc

  Serial.begin(9600);
  if (!SD.begin(SD_ChipSelectPin)) {  // see if the card is present and can be initialized:
    Serial.println("SD fail");  
    audioAvailable = 0;
  }else{
    audioAvailable = 1;
    Serial.println("SD setup"); 
  }

  readIndex = 0;

  // setup each input pin
  inputs[0].analogPin = A0;
  inputs[0].numPositions = 5;
  
  inputs[1].analogPin = A1;
  inputs[1].numPositions = 4;

  // populate defaukts for each input
  for(int i = 0; i < numInputs; i++){
    inputs[i].lastPositionReading = 0;
    inputs[i].averageReadingsTotal = 0;
    inputs[i].averageReading = 0;
    // initialize all the readings to the current value
    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
      inputs[i].averageReadingsSet[thisReading] = analogRead(inputs[i].analogPin);;
    }
  }
  Serial.println("Init done"); 
}


void loop() {

  for(int i = 0; i < numInputs; i++){
    
    inputs[i].averageReadingsTotal = inputs[i].averageReadingsTotal - inputs[i].averageReadingsSet[readIndex];
    inputs[i].averageReadingsSet[readIndex] = analogRead(inputs[i].analogPin);
    inputs[i].averageReadingsTotal = inputs[i].averageReadingsTotal + inputs[i].averageReadingsSet[readIndex];
    // increment and reset to 0 if at the end of the list
    readIndex = (readIndex + 1) % numReadings;
    inputs[i].averageReading = inputs[i].averageReadingsTotal / numReadings;
  
    int switchPositionReading = getSwitchPosition(inputs[i].averageReading, inputs[i].numPositions);
    if(switchPositionReading != -1){
      if(switchPositionReading != inputs[i].lastPositionReading){
        inputs[i].lastPositionReading = switchPositionReading;
        notifySwitchChanged(i);
      }
    }
    
    //Serial.print("Switch ");Serial.print(i);Serial.print(": ");Serial.println(inputs[i].averageReading);
  }
}

int getSwitchPosition(int rawValue, int numPositions){
  numPositions++;
  for(int i = 0; i < numPositions; i++){
    double targetPosition = (maxSwitchReading/(numPositions - 1))* i;
    if(rawValue > (targetPosition - jitterThreshold) && rawValue < (targetPosition + jitterThreshold)){
      return i;
    }
  }
  return -1;
}

void notifySwitchChanged(int switchIndex){
  Serial.print("switch ");
  Serial.print(switchIndex);
  Serial.print(" changed to position ");
  Serial.print(inputs[switchIndex].lastPositionReading);
  Serial.println();

  char filenameBuffer[5] = "";
  //audio files should be saved to the sd card in the format {switchIndex}_{positionIndex}.wav (e.g. 1_4.wav) 
  sprintf(filenameBuffer, "%d_%d", switchIndex, inputs[switchIndex].lastPositionReading);
  
  Serial.print("play:");
  Serial.println(filenameBuffer);

  // play the audio for this switch position
  if(audioAvailable == 1){
    audioPlayer.play(filenameBuffer);
  }
}

