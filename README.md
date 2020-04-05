# arduino-nixie-clock
Custom IN-12A (ИН-12А) Nixie clock with RTC and arduino

This code is messy and specific to my board design. 
I made one change after recieving the boards. I needed a 5volt power pin for the rotary encoder. I scrapped up a trace to a digital pin on the button header and added a bodge wire for 5 volts.

#Main Modes
Adjust by turning rotary encoder
0: Time mode
1: Date mode
2: Rotate mode
Rotate mode rotates between date and time at a set interval

#Settings menu
Enter settings menu by pressing rotary encoder
Press it again to change the selected setting
0: Exit settings menu
1: Set Time
2: Set Date
3: Set rotation interval in seconds
4: Select 12 or 24 hour display
5: Set the time transition mode. currently there is only two. 0 for normal and 1 for slot machine effect.
6: Set time to turn display off then back on. First digit is the hour it turns off. second is the hour it turns back on
7: Set anti cathode poison time. Fist digit is the minute that the first run will happen on. Second is the interval in minutes that it runs
example: 15 - 10. the first run will happen at 0:15 and the next run will happen at 0:25. Add 10 minutes for every run after