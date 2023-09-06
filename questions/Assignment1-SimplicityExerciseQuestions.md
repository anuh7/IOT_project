Please include your answers to the questions below with your submission, entering into the space below each question
See [Mastering Markdown](https://guides.github.com/features/mastering-markdown/) for github markdown formatting if desired.

**1. How much current does the system draw (instantaneous measurement) when a single LED is on with the GPIO pin set to StrongAlternateStrong?**
   Answer: 5.40 mA


**2. How much current does the system draw (instantaneous measurement) when a single LED is on with the GPIO pin set to WeakAlternateWeak?**
   Answer: 5.54 mA


**3. Is there a meaningful difference in current between the answers for question 1 and 2? Please explain your answer, referencing the main board schematic, WSTK-Main-BRD4001A-A01-schematic.pdf, and AEM Accuracy in the ug279-brd4104a-user-guide.pdf. Both of these PDF files are available in the ECEN 5823 Student Public Folder in Google drive at: https://drive.google.com/drive/folders/1ACI8sUKakgpOLzwsGZkns3CQtc7r35bB?usp=sharing . Extra credit is available for this question and depends on your answer.**
   Answer: The I/O pins have the same default drive strength unless configured otherwise in the GPIO initialisation. The function GPIO_DriveStrengthSet is used to set the drive strength for the entire port and the alternate function for the port. The 'Strong' mode provides a drive current of 10mA while the 'Weak' mode provides a drive current of 1mA. However, there is not any meaningful difference between the two. This is attributed to two reasons:
   - Accuracy of the AEM: For system currents of above 0.25mA, the AEM accuracy is within 0.1 mA. The difference in the instantaneous current measurements is around 0.15 mA. This could be attributed to inaccurate measurement by the AEM. 
   - Current limiting resistor: There is a current limiting resistor of 3k connected to the user LEDs which would limit the current flowing through the LED and thus the overall system.  

**4. With the WeakAlternateWeak drive strength setting, what is the average current for 1 complete on-off cycle for 1 LED with an on-off duty cycle of 50% (approximately 1 sec on, 1 sec off)?**
   Answer: 5.24 mA


**5. With the WeakAlternateWeak drive strength setting, what is the average current for 1 complete on-off cycle for 2 LEDs (both on at the time same and both off at the same time) with an on-off duty cycle of 50% (approximately 1 sec on, 1 sec off)?**
   Answer: 5.47 mA


