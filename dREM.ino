#include <arduinoFFT.h>  
#include <ArduinoBLE.h>  
    
#define SAMPLES 1024             //Must be a power of 2  
#define SAMPLING_FREQUENCY 68    //Hz, must be less than 10000 due to ADC  
   
arduinoFFT FFT = arduinoFFT();  
BLEService EEGService("1101");  
BLEUnsignedIntCharacteristic EEGChar("2101", BLENotify);  
// create switch characteristic and allow remote device to read and write  
BLECharCharacteristic switchChar("2A19", BLERead | BLEWrite);  
   
 /*Sampling & HPF Variables*/  
unsigned int sampling_period_us;  
unsigned long microseconds;  
  
double vReal[SAMPLES];  
double vImag[SAMPLES];  
  
float sum;  
float average;  
  
/*Classification Variables*/  
float maxAmp;  
float noiseThreshold;  
float deltaLim = 8;  
float alphaLim = 12;  
float betaLim = 32;  
int sStage[1920];//estimated length for 8 hours with 15 second samples  
int counter = 1;  
   
/*Alarm Variables*/  
int alarm = 0;  
int speakerPin = 8;  
float alarmTime;  
  
  
void setup() {  
    /*EEG INPUT SETUP*/  
    Serial.begin(9600);  
    sampling_period_us = round(1000000*(1.0/SAMPLING_FREQUENCY));   
     
    /*BLUETOOTH SETUP*/  
    while (!Serial);  
   
     pinMode(LED_BUILTIN, OUTPUT);  
    if (!BLE.begin())  
    {  
      Serial.println("starting BLE failed!");  
      while (1);  
    }  
    /*BLUETOOTH TRANSMIT SETUP*/  
    BLE.setLocalName("EEGMonitor");  
    BLE.setAdvertisedService(EEGService);  
    EEGService.addCharacteristic(EEGChar);  
    BLE.addService(EEGService);  
    EEGChar.writeValue(0);  
      
    // add service and characteristic  
    ledService.addCharacteristic(switchChar);  
    BLE.addService(ledService);  
      
    // assign event handlers for characteristic - receives information from the computer - interrupt   
    switchChar.setEventHandler(BLEWritten, switchCharacteristicWritten);  
    // set an initial value for the characteristic  
    switchChar.setValue(0);  
      
    BLE.advertise();  
    Serial.println("Bluetooth device active, waiting for connections...");    
}  
  
   
void loop() {  
/*DATA ANALYSIS*/  
 
  while(millis()%15000 < 10){ //replaces "delay" &prevents halting rest of code. Repeats when time past is divisible by 1000ms (time%1000=0) but compare to 10 for any error    
    /*SAMPLING*/  
      for(int i=0; i<SAMPLES; i++)  
      {  
        microseconds = micros();    //Overflows after around 70 minutes!  
       
        vReal[i] = analogRead(0);  
        vImag[i] = 0;  
       
        while(micros() < (microseconds + sampling_period_us)){  
        }  
      }  
  
    /*HPF: REMOVE DC BIAS*/  
    sum = 0; //reset sum and average to 0  
    average = 0;  
    for(int j=0; j<SAMPLES; j++){ //Find offset  
        sum += vReal[j];  
    }  
    average = sum/SAMPLES;  
    maxAmp = 0; //Reset max Amplitude  
    for(int j=0; j<SAMPLES; j++){ //Remove offset  
        vReal[j] = vReal[j] - average;  
        if(vReal[j] > maxAmp){ //Find maxAmp for later analysis  
         maxAmp = vReal[j];  
         }  
     }  
       
     /*FFT*/  
     FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);  
     FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);  
     FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);  
     double peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);  
    
    /*CLASSIFY SLEEP STAGE*/   
     noiseThreshold = 1024*(1-0.05); //5% less than max value of 10-bit ADC   
   
    if (maxAmp >= noiseThreshold){ //if there's noise marked by high amplitude  
        sStage[counter] = sStage[counter-1]; //ignore data and set this stage to be the same as the previous  
        counter += 1;  
     }  
     else{ //otherwise, analyze the data  
        if (peak <= deltaLim){  
            sStage[counter] = 3;} //deep sleep  
        else if ((peak > deltaLim) && (peak <= alphaLim)){  
            sStage[counter] = 2;} //light sleep  
        else if ((peak > alphaLim) && (peak < betaLim)){  
            sStage[counter] = 1;} //REM  
        else{  
            sStage[counter] = 4;} //awake  
           
        counter += 1;  
     }  
   }  
   
  /*DATA TRANSMISSION ONCE ALARM RINGS*/  
     if (alarm == 1){  
       if ((sStage[counter-1] != 3) || (millis() >= alarmTime+900000)){ //alarm goes off in REM/light sleep or at desired alarm time (note: notification received 15mins before desired alarm time)  
         tone(speakerPin, 1000, 500); //tone(pin,Hz,ms duration);  
         //connect to bluetooth device to transmit array  
         BLEDevice central = BLE.central();  
   
         if (central)  
         {  
           digitalWrite(LED_BUILTIN, HIGH);  
   
           if (central.connected()) {  
   
             for(int i=1; i<(sizeof(sStage)/sizeof(int)); i++){//length of array = sizeof(sStafe)/sizeof(int)  
               EEGChar.writeValue(sStage[i]);  
               Serial.println(sStage[i]);  
               delay(50); //delay transmission so matlab can read fast enough  
             }     
           }  
         }  
         digitalWrite(LED_BUILTIN, LOW);  
         Serial.println("Disconnected from central");  
       }  
     }  
   
 /*ALARM NOTIFICATION*/  
   // poll peripheral  
   BLE.poll();         
 }  
   
 /*ALARM NOTIFICATION FUNCTIONS*/  
   void blePeripheralConnectHandler(BLEDevice central) {  
     // central connected event handler  
     Serial.print("Connected event, central: ");  
     Serial.println(central.address());  
   }  
      
   void blePeripheralDisconnectHandler(BLEDevice central) {  
     // central disconnected event handler  
     Serial.print("Disconnected event, central: ");  
     Serial.println(central.address());  
   }  
     
   // function to handle writing to alarm pin   
   void switchCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {  
     // central wrote new value to characteristic, update alarm pin  
     Serial.print("Characteristic event, written: ");  
     
     if (switchChar.value()) {  
       alarm = 1;  
       alarmTime = millis();  
     }   
     else {  
       alarm = 0;  
      }  
    }  
