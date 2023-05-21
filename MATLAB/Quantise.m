%
%Author: Cameron Stephens
%Description:
%This script is a very basic algorithm to quantise the data read from the
%micro-SD Card. The user must set the correct variables in order for this
%script to work correctly. 
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%Clears command window and closes all figures
clc;
close all;

%Finds the file contained on the micro-SD card and reads the values from it
%Storing them in S
%This was built with support from this YoutTube video from Painless
%Programming - https://www.youtube.com/watch?v=0MtpTWKIKrU
fid = fopen('D:\glucoseresults.txt', 'r');
    if fid == -1
        disp('Error, check file name')
    else
        S = textscan(fid, '%f %f', 'Delimiter', ',');
    end
fclose(fid); %Closes the text file again to prevent corruption

%User set variables
sampleRatems = 5; %Sample rate in milliseconds
periodCount = 2; %How many samples periods ran
periodLength = 10; %Period length in seconds
ADCBitResolution = 12; %Resolution of the ADC used
componentSize = 16; %Data size used to store components
maxADCVoltage = 3.3; %Maximum voltage value for the ADC

bufferSize = ((periodLength*1000)/sampleRatems); %Buffer size variable calculated from sample rate
time = (sampleRatems/1000):(sampleRatems/1000):periodLength; %Calculates the time interval for graph purposes

%Empty arrays with the size set by the sample rate - used to temporarily
%store the AC and DC components
acComponent = zeros(bufferSize,1);
dcComponent = zeros(bufferSize,1);

%Quantiser for loop - length is set by how many periods the user chooses
for cycleCounter = 0:periodCount-1
    
    %Runs for a period size worth of data and stores the AC and DC
    %components in a temprarily array to be graphed
    for bufferPositionCounter = 1:bufferSize 
        %Quantises the AC and DC components based on ADC specs
        acComponent(bufferPositionCounter) = (maxADCVoltage/(2^ADCBitResolution))*(S{1}(bufferPositionCounter+(bufferSize*cycleCounter))/componentSize); 
        dcComponent(bufferPositionCounter) = (maxADCVoltage/(2^ADCBitResolution))*(S{2}(bufferPositionCounter+(bufferSize*cycleCounter))/componentSize);
    end
    
    dataSetCount = cycleCounter+1; %Counts the current plot cycle
    
    %Plots graphs of the AC and DC components with titles
    figure (1)
    subplot(periodCount,1,cycleCounter+1);
    plot(time,acComponent);
    title("AC Component Data sample period: " + dataSetCount);
    xlabel('Time(s)')
    ylabel('Voltage(V)')
        
    figure (2)
    subplot(periodCount,1,cycleCounter+1);
    plot(time,dcComponent)
    title("DC Component Data sample period: " + dataSetCount);
    xlabel('Time(s)')
    ylabel('Voltage(V)')
end



