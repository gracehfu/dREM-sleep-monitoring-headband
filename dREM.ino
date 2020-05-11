1.  #include <arduinoFFT.h>  
2.  #include <ArduinoBLE.h>  
3.     
4.  #define SAMPLES 1024             //Must be a power of 2  
5.  #define SAMPLING_FREQUENCY 68    //Hz, must be less than 10000 due to ADC  
6.    
7.  arduinoFFT FFT = arduinoFFT();  
8.  BLEService EEGService("1101");  
9.  BLEUnsignedIntCharacteristic EEGChar("2101", BLENotify);  
10. // create switch characteristic and allow remote device to read and write  
11. BLECharCharacteristic switchChar("2A19", BLERead | BLEWrite);  
12.   
13. /*Sampling & HPF Variables*/  
14. unsigned int sampling_period_us;  
15. unsigned long microseconds;  
16.    
17. double vReal[SAMPLES];  
18. double vImag[SAMPLES];  
19.   
20. float sum;  
21. float average;  
22.   
23. /*Classification Variables*/  
24. float maxAmp;  
25. float noiseThreshold;  
26. float deltaLim = 8;  
27. float alphaLim = 12;  
28. float betaLim = 32;  
29. int sStage[1920];//estimated length for 8 hours with 15 second samples  
30. int counter = 1;  
31.   
32. /*Alarm Variables*/  
33. int alarm = 0;  
34. int speakerPin = 8;  
35. float alarmTime;  
36.   
37.   
38. void setup() {  
39.     /*EEG INPUT SETUP*/  
40.     Serial.begin(9600);  
41.     sampling_period_us = round(1000000*(1.0/SAMPLING_FREQUENCY));   
42.      
43.     /*BLUETOOTH SETUP*/  
44.     while (!Serial);  
45.   
46.     pinMode(LED_BUILTIN, OUTPUT);  
47.     if (!BLE.begin())  
48.     {  
49.       Serial.println("starting BLE failed!");  
50.       while (1);  
51.     }  
52.     /*BLUETOOTH TRANSMIT SETUP*/  
53.     BLE.setLocalName("EEGMonitor");  
54.     BLE.setAdvertisedService(EEGService);  
55.     EEGService.addCharacteristic(EEGChar);  
56.     BLE.addService(EEGService);  
57.     EEGChar.writeValue(0);  
58.       
59.     // add service and characteristic  
60.     ledService.addCharacteristic(switchChar);  
61.     BLE.addService(ledService);  
62.       
63.     // assign event handlers for characteristic - receives information from the computer - interrupt   
64.     switchChar.setEventHandler(BLEWritten, switchCharacteristicWritten);  
65.     // set an initial value for the characteristic  
66.     switchChar.setValue(0);  
67.       
68.     BLE.advertise();  
69.     Serial.println("Bluetooth device active, waiting for connections...");    
70. }  
71.   
72.    
73. void loop() {  
74. /*DATA ANALYSIS*/  
75.   
76.   while(millis()%15000 < 10){ //replaces "delay" &prevents halting rest of code. Repeats when time past is divisible by 1000ms (time%1000=0) but compare to 10 for any error    
77.     /*SAMPLING*/  
78.       for(int i=0; i<SAMPLES; i++)  
79.       {  
80.         microseconds = micros();    //Overflows after around 70 minutes!  
81.        
82.         vReal[i] = analogRead(0);  
83.         vImag[i] = 0;  
84.        
85.         while(micros() < (microseconds + sampling_period_us)){  
86.         }  
87.       }  
88.   
89.     /*HPF: REMOVE DC BIAS*/  
90.     sum = 0; //reset sum and average to 0  
91.     average = 0;  
92.     for(int j=0; j<SAMPLES; j++){ //Find offset  
93.         sum += vReal[j];  
94.     }  
95.     average = sum/SAMPLES;  
96.     maxAmp = 0; //Reset max Amplitude  
97.     for(int j=0; j<SAMPLES; j++){ //Remove offset  
98.         vReal[j] = vReal[j] - average;  
99.         if(vReal[j] > maxAmp){ //Find maxAmp for later analysis  
100.          maxAmp = vReal[j];  
101.          }  
102.      }  
103.        
104.      /*FFT*/  
105.      FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);  
106.      FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);  
107.      FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);  
108.      double peak = FFT.MajorPeak(vReal, SAMPLES, SAMPLING_FREQUENCY);  
109.     
110.     /*CLASSIFY SLEEP STAGE*/   
111.      noiseThreshold = 1024*(1-0.05); //5% less than max value of 10-bit ADC   
112.    
113.     if (maxAmp >= noiseThreshold){ //if there's noise marked by high amplitude  
114.         sStage[counter] = sStage[counter-1]; //ignore data and set this stage to be the same as the previous  
115.         counter += 1;  
116.      }  
117.      else{ //otherwise, analyze the data  
118.         if (peak <= deltaLim){  
119.             sStage[counter] = 3;} //deep sleep  
120.         else if ((peak > deltaLim) && (peak <= alphaLim)){  
121.             sStage[counter] = 2;} //light sleep  
122.         else if ((peak > alphaLim) && (peak < betaLim)){  
123.             sStage[counter] = 1;} //REM  
124.         else{  
125.             sStage[counter] = 4;} //awake  
126.            
127.         counter += 1;  
128.      }  
129.    }  
130.    
131.   /*DATA TRANSMISSION ONCE ALARM RINGS*/  
132.      if (alarm == 1){  
133.        if ((sStage[counter-1] != 3) || (millis() >= alarmTime+900000)){ //alarm goes off in REM/light sleep or at desired alarm time (note: notification received 15mins before desired alarm time)  
134.          tone(speakerPin, 1000, 500); //tone(pin,Hz,ms duration);  
135.          //connect to bluetooth device to transmit array  
136.          BLEDevice central = BLE.central();  
137.    
138.          if (central)  
139.          {  
140.            digitalWrite(LED_BUILTIN, HIGH);  
141.    
142.            if (central.connected()) {  
143.    
144.              for(int i=1; i<(sizeof(sStage)/sizeof(int)); i++){//length of array = sizeof(sStafe)/sizeof(int)  
145.                EEGChar.writeValue(sStage[i]);  
146.                Serial.println(sStage[i]);  
147.                delay(50); //delay transmission so matlab can read fast enough  
148.              }     
149.            }  
150.          }  
151.          digitalWrite(LED_BUILTIN, LOW);  
152.          Serial.println("Disconnected from central");  
153.        }  
154.      }  
155.    
156.  /*ALARM NOTIFICATION*/  
157.    // poll peripheral  
158.    BLE.poll();         
159.  }  
160.    
161.  /*ALARM NOTIFICATION FUNCTIONS*/  
162.    void blePeripheralConnectHandler(BLEDevice central) {  
163.      // central connected event handler  
164.      Serial.print("Connected event, central: ");  
165.      Serial.println(central.address());  
166.    }  
167.      
168.    void blePeripheralDisconnectHandler(BLEDevice central) {  
169.      // central disconnected event handler  
170.      Serial.print("Disconnected event, central: ");  
171.      Serial.println(central.address());  
172.    }  
173.      
174.    // function to handle writing to alram pin   
175.    void switchCharacteristicWritten(BLEDevice central, BLECharacteristic characteristic) {  
176.      // central wrote new value to characteristic, update alarm pin  
177.      Serial.print("Characteristic event, written: ");  
178.      
179.      if (switchChar.value()) {  
180.        alarm = 1;  
181.        alarmTime = millis();  
182.      }   
183.      else {  
184.        alarm = 0;  
185.      }  
186.    }  
