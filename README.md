# buttermill
A windmill standpoint feasibility project using an Atmel Butterfly

### The basic idea behind the project

This is a project to create a windmill standpoint feasibility analyser using commodity hardware and relatively inexpensive Atmel hardware. What's meant by a windmill standpoint feasibility analysis is that it is advisable to test the proposed location of (e.g. electricity generating) windmill before actually going ahead with acquisition and building. Using commodity, low-priced wind-speed measurement devices (anemometers), it is possible to gain an idea of the average windspeed which a bigger windmill would be likely to be exposed to. The people at http://www.speedofthewind.com put it like this:

With increasing interest in generating electricity using wind turbines
both on a commercial and domestic scale , our anemometers offer a
practical solution to assess viability before significant investment is made.

Fool proof logging and update system

As this project is meant to make it easy for non-technical users to log the speed of the wind over prolonged periods of time, it should be possible for non-technical users to access and parse the log files and to update the firmware easily. As we already decided to use an SD card to do the logging, programming the bootloader to be able to detect new firmware on the SD card seemed like a good idea...
This is a photo of the Buttermill as of 2010-03-26

A rough sketch of the mast and the proposed box

In the background is a roughly 12m mast with an anemometer at the top

