%
%Author: Cameron Stephens
%Description:
%This script was the start of the pre-processing algorithm for the
%Non-Invasive Blood Glucose Monitoring Project before it got halted due to
%time constaints and workloads. However, the basic alogirthms were
%following the same creation as is done in Rachim and Chung (2019). The
%majority of these alogirthms were from MathWorks by typing in the desired
%process - wavelet transform for excample.
%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

%Clears command window and closes all figures
clc;
close all;

total = 1000;

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
    fclose(fid);

%Quantise of data
AC = (3.3/4096)*(S{1}/16);
test = AC(1:1000);

%Wavelet
[c, l] = wavedec(test, 8, 'sym4');
nc = wthcoef('a',c,l);
x = waverec(nc,l,'sym4');
l = 1:size(test);
%Moving Average - Made in support from the YoutTube video created by
%Knowledge Amplifier - https://www.youtube.com/watch?v=yargH0L3B68&t=526s
averageAmount = 20;
oneMaker = ones(1,averageAmount);
num = (1/averageAmount)*oneMaker;
den = [1];
averageFilter = filter(num,den,x);

[pks,loc] = findpeaks(averageFilter);
%[max,indices] = localmax(averageFilter);
%max = islocalmax(averageFilter);
%figure
subplot(4,1,1)
plot(test)
subplot(4,1,2)
plot(x)
subplot(4,1,3)
plot(averageFilter)
subplot(4,1,4)
%plot(l,averageFilter,l(max),averageFilter(max),'r*')
plot(loc,pks)

