% % Clears command window and closes all figures
clc;
% close all;
% 
% %Finds the file contained on the micro-SD card and reads the values from it
% %Storing them in S
% fid = fopen('D:\glucoseresults.txt', 'r');
%     if fid == -1
%         disp('Error, check file name')
%     else
%         S = textscan(fid, '%f %f', 'Delimiter', ',');
%     end
% fclose(fid); %Closes the text file again to prevent corruption
% 
% %User set variables
% sampleRatems = 5; %Sample rate in milliseconds
% periodCount = 5; %How many samples periods ran
% periodLength = 10; %Period length in seconds
% ADCBitResolution = 12; %Resolution of the ADC used
% componentSize = 16; %Data size used to store components
% maxADCVoltage = 3.3; %Maximum voltage value for the ADC
% 
% bufferSize = (10000/sampleRatems); %Buffer size variable calculated from sample rate
% time = 0.005:(sampleRatems/1000):periodLength; %Calculates the time interval for graph purposes
% 
% %Empty arrays with the size set by the sample rate - used to temporarily
% %store the AC and DC components
% acComponent = zeros(bufferSize,1);
% dcComponent = zeros(bufferSize,1);
% 
% %Quantiser for loop - length is set by how many periods the user chooses
% for cycleCounter = 0:periodCount-1
%     
%     %Runs for a period size worth of data and stores the AC and DC
%     %components in a temprarily array to be graphed
%     for bufferPositionCounter = 1:bufferSize 
%         %Quantises the AC and DC components based on ADC specs
%         acComponent(bufferPositionCounter) = (maxADCVoltage/(2^ADCBitResolution))*(S{1}(bufferPositionCounter+(bufferSize*cycleCounter))/componentSize); 
%         dcComponent(bufferPositionCounter) = (maxADCVoltage/(2^ADCBitResolution))*(S{2}(bufferPositionCounter+(bufferSize*cycleCounter))/componentSize);
%     end
%     
%     dataSetCount = cycleCounter+1; %Counts the current plot cycle
%     
%     %Plots graphs of the AC and DC components with titles
%     figure (1)
%     subplot(10,1,cycleCounter+1);
%     plot(time,acComponent);
%     title("AC Component Data sample period: " + dataSetCount);
%     xlabel('Time(s)')
%     ylabel('Voltage(V)')
%         
%     figure (2)
%     subplot(10,1,cycleCounter+1);
%     plot(time,dcComponent)
%     title("DC Component Data sample period: " + dataSetCount);
%     xlabel('Time(s)')
%     ylabel('Voltage(V)')
% end


message = 0b1101100111011010u32;
messageLength = 16;
divisor = 0b1111u32;
divisorDegree = 3;

divisor = bitshift(divisor,messageLength-divisorDegree-1);
dec2bin(divisor)

divisor = bitshift(divisor,divisorDegree);
remainder = bitshift(message,divisorDegree);
dec2bin(divisor)

dec2bin(remainder)

for k = 1:messageLength
    if bitget(remainder,messageLength+divisorDegree)
        remainder = bitxor(remainder,divisor);
    end
    remainder = bitshift(remainder,1);
end

CRC_check_value = bitshift(remainder,-messageLength);
dec2bin(CRC_check_value)

remainder = bitshift(message,divisorDegree);
remainder = bitor(remainder,CRC_check_value);
remainder = bitset(remainder,6);
dec2bin(remainder)

for k = 1:messageLength
    if bitget(remainder,messageLength+divisorDegree)
        remainder = bitxor(remainder,divisor);
    end
    remainder = bitshift(remainder,1);
end
if remainder == 0
    disp('Message is error free.')
else
    disp('Message contains errors.')
end

