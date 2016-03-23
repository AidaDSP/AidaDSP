% ********************
% *  TS9 Simulation  *  
% ********************
% Max Pennazio 2016

clear all;
close all;
clc;

Fs = 48000; % [Hz]
Ts = 1/Fs; % [s]
ws = 2*pi*Fs;

P = bodeoptions;
P.FreqUnits = 'Hz';

R1 = 4700;
Cz = 0.047e-6;
Cc = 51e-12;
N1 = 5; % Number of steps for drive command
N2 = 64; % Number of steps for function evaluation
drive = linspace(0, 1, N1);
res = zeros(N1, N2);

for i = 1 : N1

    R2 = 51000 + 500000*drive(i);
   
    a1 = (R1 + R2) * Cz * 2 * Fs;
    a2 = R1 * Cz * 2 * Fs;
    B0 = (1 + a1) / (1 + a2)
    B1 = (1 - a1) / (1 + a2)
    A1 = (1 - a2) / (1 + a2)

    z=tf('z',Ts);

    tf1z = (B0+B1*(z^-1))/(1+(A1*(z^-1)));
    zpk(tf1z)

    figure(1);
    hold on;
    bode(tf1z,P);
    if(i==1)
        msg = sprintf('Bode TS9 main filter with GAIN varying from 0 to 1');
        title(msg);
    end
    grid on;
    
    Is = 10 * 1e-12;
    mUt = 30 * 1e-3;
    D=sym('D');
    X=sym('X');
    
    %f = (X/(R1*Cc)) - (D/(R2*Cc)) - (Is/Cc) * (exp(D/mUt) - exp(-D/mUt)); %
    f = (X/(R2*Cc)) - (D/(R2*Cc)) - (Is/Cc) * (exp(D/mUt) - exp(-D/mUt)); % Why is lecit to substitute R1 for R2??? O_o
    fname1 = inline(char(f));
    
    vect1 = linspace(-1.0, 1.0, N2); % Evaluating non linear function for X -1:1 
    for j = 1:N2
       %func = feval(fname1, D, vect1(j)*2.83); % Obtain an expression with only Y choosing X and X2
       func = feval(fname1, D, vect1(j)); % Obtain an expression with only Y choosing X and X2
       fname2 = inline(char(func));
       res(i,j) = fzero(fname2, 0); % Find roots near current input value and divide it by signal input
    end

    figure(2);
    hold on;
    plot(vect1, res(i,:), '-b');
    grid on;
    if(i==1)
        msg = sprintf('Non linear function for TS9 with GAIN varying from 0 to 1');
        title(msg);
        xlabel('In');
        ylabel('Out');  
    end
    
    % Write data to file for LUT import
    % in Sigma Studio
    name = sprintf('LutDiodesGain%d.txt',i);
    fileID = fopen(name,'w');
    formatSpec = '%1.6f\n';
    fprintf(fileID, formatSpec, res(i,:));
    fclose(fileID);
end

% Output tone analysis

s=tf('s');

tone = linspace(0, 1, N1);

for i = 1 : N1
   
    T = tone(i);
    Rf = 1000;
    Rr = (1-T)*20e3;
    Rl = T*20e3;
    Rz = 220;
    Cz = 220e-9;
    Ri = 10e3;
    Rs = 1e3;
    Cs = 220e-9;
    
    wz = 1/(Cz*(Rz + ((Rl*Rr)/(Rl+Rr))));
    wp = 1/(Cs*((Rs*Ri)/(Rs+Ri)));

    Y = (Rl + Rr)*(Rz + ((Rl*Rr)/(Rl+Rr)));
    W = Y/(Rl*Rf+Y);
    X = (Rr/(Rl + Rr))*(1/(Rz + ((Rl*Rr)/(Rl+Rr))*Cz));

    tf2 = ((Rl*Rf + Y)/(Y*Rs*Cs))*((s+W*wz)/((s+wp)*(s+wz)+X*s));
    
    zpk(tf2)
    
    figure(3);
    hold on;
    bode(tf2,P);
    if(i==1)
        msg = sprintf('Bode TS9 output tone circuit with tone varying from 0 to 1');
        title(msg);
    end
    grid on;
    
end

for i = 1 : 5
    R2 = 51000 + 500000*drive(i);
    fdtc = ((s / (R1*(s+(1/(R1*Cz)))))*R2)+1;
    zpk(fdtc)
end

% clear s
% clear T
% 
% syms s T
% 
% Rf = 1000;
% Rr = (1-T)*20e3;
% Rl = T*20e3;
% Rz = 220;
% Cz = 220e-9;
% Ri = 10e3;
% Rs = 1e3;
% Cs = 220e-9;
% 
% wz = 1/(Cz*(Rz + ((Rl*Rr)/(Rl+Rr))));
% wp = 1/(Cs*((Rs*Ri)/(Rs+Ri)));
% 
% Y = (Rl + Rr)*(Rz + ((Rl*Rr)/(Rl+Rr)));
% W = Y/(Rl*Rf+Y);
% X = (Rr/(Rl + Rr))*(1/(Rz + ((Rl*Rr)/(Rl+Rr))*Cz));
% 
% g = ((Rl*Rf + Y)/(Y*Rs*Cs))*((s+W*wz)/((s+wp)*(s+wz)+X*s));
% 
% F = factor(g, [s T]); % Funziona solo con versioni recenti


