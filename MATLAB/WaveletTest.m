clc;
close all;

% fileid=fopen('"D:\glucoseresults.txt"');
% format='%f';
% a[3,2] = fscanf(fileid,format);
total = 1000;

fid = fopen('D:\glucoseresults.txt', 'r');
    if fid == -1
        disp('Error, check file name')
    else
        S = textscan(fid, '%f %f', 'Delimiter', ',');
    end
    fclose(fid);

AC = (3.3/4096)*(S{1}/16);
test = AC(1:1000);

%Wavelet
[c, l] = wavedec(test, 8, 'sym4');
nc = wthcoef('a',c,l);
x = waverec(nc,l,'sym4');
l = 1:size(test);
%Moving Average
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
% A1 = appcoef(C1, L1, 'sym4',1);
% A2 = appcoef(C1, L1, 'sym4',2);
% A3 = appcoef(C1, L1, 'sym4',3);
% A4 = appcoef(C1, L1, 'sym4',4);
% A5 = appcoef(C1, L1, 'sym4',5);
% A6 = appcoef(C1, L1, 'sym4',6);
% A7 = appcoef(C1, L1, 'sym4',7);
% A8 = appcoef(C1, L1, 'sym4',8);
% AD = size(A8);
% AD = AD(1);
% AD = zeros(AD,1);
% appcoef(C1, L1, 'sym4',8) = AD
% [D1, D2, D3, D4, D5, D6, D7, D8] = detcoef(C1, L1, [1 2 3 4 5 6 7 8]);

% figure
% %subplot(9,1,1)
% plot(AC)
% title('Signal');
% figure
% plot (x)
% 
% subplot(9,1,2)
% plot(D8)
% title('Detail at Level 8');
% 
% subplot(9,1,3)
% plot(D7)
% title('Detail at Level 7');
% 
% subplot(9,1,4)
% plot(D6)
% title('Detail at Level 6');
% 
% subplot(9,1,5)
% plot(D5)
% title('Detail at Level 5');
% 
% subplot(9,1,6)
% plot(D4)
% title('Detail at Level 4');
% 
% subplot(9,1,7)
% plot(D3)
% title('Detail at Level 3');
% 
% subplot(9,1,8)
% plot(D2)
% title('Detail at Level 2');
% 
% subplot(9,1,9)
% plot(D1)
% title('Detail at Level 1');
% 
% figure
% subplot(9,1,1)
% plot(AC)
% title('Signal');
% 
% subplot(9,1,2)
% plot(A8)
% title('A at Level 8');
% 
% subplot(9,1,3)
% plot(A7)
% title('A at Level 7');
% 
% subplot(9,1,4)
% plot(A6)
% title('A at Level 6');
% 
% subplot(9,1,5)
% plot(A5)
% title('A at Level 5');
% 
% subplot(9,1,6)
% plot(A4)
% title('A at Level 4');
% 
% subplot(9,1,7)
% plot(A3)
% title('A at Level 3');
% 
% subplot(9,1,8)
% plot(A2)
% title('A at Level 2');
% 
% subplot(9,1,9)
% plot(A1)
% title('A at Level 1');
