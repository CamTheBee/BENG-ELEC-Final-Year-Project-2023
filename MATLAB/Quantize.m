clc;
close all;


fid = fopen('D:\glucoseresults.txt', 'r');
    if fid == -1
        disp('Error, check file name')
    else
        S = textscan(fid, '%f %f', 'Delimiter', ',');
    end
fclose(fid);
    
sampleRatems = 5;
periodCount = 3;
bufferSize = (10000/sampleRatems); 

acComponent = zeros(bufferSize,1);
dcComponent = zeros(bufferSize,1);

for cycleCounter = 0:periodCount-1

    for bufferPositionCounter = 1:bufferSize 
    acComponent(bufferPositionCounter) = (3.3/4096)*(S{1}(bufferPositionCounter+(bufferSize*cycleCounter))/16);
    dcComponent(bufferPositionCounter) = (3.3/4096)*(S{2}(bufferPositionCounter+(bufferSize*cycleCounter))/16);
    end
    figure
    plot(acComponent);
    figure
    plot(dcComponent)
end
