# dREM sleep monitoring headband
A wearable device to monitor sleep activity based on EEG signals. 
The device is designed using an Arduino Nano BLE 33 for bluetooth communication between the headband and PC. 
Sleep data for calibrating the system was provided by Dr. deBruin at McMaster University. 

More information on the hardware design and code can found in this repository. 

## Motivation
Many traditional sleep tracking techniques are either inconvenient, requiring overnight monitoring at a sleep laboratory (polysomnography) or rely primarily on motion and heart rate data, which cannot accurately measure sleep quality. Sleep trackers that track user movement are also known to become less accurate with disrupted and poor quality sleep. Therefore, an EEG recording headband that can measure the brain wave characteristics of each stage in sleep can provide a convenient to use and accurate sleep tracker.

## System Overview
In order to improve the quality of sleep, dREM offers users the ability to monitor their sleep, tracking the different sleep stages they go through and what it means. One of the features dREM includes, allows users to track their diet and exercise to monitor how these factors affect the quality of their sleep as well as input their mood for the day to display the relationship between the quality of their sleep to their mood. dREM also provides its users with suggestions of the optimal time to wake up feeling rested. 

At a high level, the development of the dREM device includes four major modules: signal acquisition and processing to properly filter the noise and amplify EEG signals, wireless communication between the headband and PC, data analysis to be able to identify the sleep stage of the user, and the user interface to present the information to the user and allow for suggestions and user input. 

## Future Improvements
With further testing and lab equipment, the design can be developed as a cheaper alternative to other methods of EEG-based sleep.  Accelerometers, pulse oximeters, and EOG measurements can also be added to improve sleep staging accuracy. Further features of dREM could include the ability to detect sleep disorders such as sleep apnea and various parasomnias, allowing sleep studies to be performed in the comfort of the patient’s home.
