%% Initialisation

clear, clc, close all;

%% Paramètres

d_n     = 2e-3;                          % entrefer nominal
N       = 125;                          % nombre de spires bobine
A       = 20e-3 * 50e-3;                % surface de l'entrefer
R       = 0.8;                          % résistance de la bobine
mu0     = 4*pi*1e-7;                    % perméabilité du vide
L_n     = N^2 * mu0 * A / (2 * d_n)     % inductance nominale
g       = 9.81;                         % constante de gravité
m       = 3.4;                          % masse a soulever par 1 inducteur
d_0     = 3e-3;                         % entrefer minimum
d_min   = 0.1e-3;                       % entrefer minimum, structure collée
d_max   = 3e-3;                         % entrefer maximum, structure posée
d_c     = 2e-3;                         % consigne entrefer
Fel     = m*g;                           % Force nécessaire à maintenir d_n 
i_n     = sqrt(2*Fel*d_n/L_n);           % courant nominal

%% Simulink

Tend = 0.1;
sim('Bloc_2_inducteurs');

%% plot

figure;

subplot(2,2,1);
plot(i.Time,i.Data,i2.Time, i2.Data);
%vline(i.Time(find((i.Time) >= Fel.Time(find((Fel.Data) >= m*g , 1)),1)),'-.r',[num2str(i.Data(find((Fel.Data) >= m*g , 1)),'%0.2f'),' [A]',]);
title('Courant');
xlabel('t [s]'), ylabel('i [A]');
grid on;
legend('inducteur1', 'inducteur2');

subplot(2,2,2);
plot(Fel.Time,Fel.Data,Fel2.Time,Fel2.Data);
%vline(Fel.Time(find((Fel.Data) >= m*g , 1)),'-.r',[num2str(Fel.Data(find((Fel.Data) >= m*g , 1)),'%0.2f'),' [N]']);
title('Force électromagnétique');
xlabel('t [s]'), ylabel('F_{el} [N]');
grid on;

subplot(2,2,3);
plot(delta.Time,delta.Data,delta2.Time,delta2.Data);
title('Entrefer');
xlabel('t [s]'), ylabel('\delta [m]');
grid on;

subplot(2,2,4);
plot(deltaDot.Time,deltaDot.Data,deltaDot2.Time,deltaDot2.Data);
title('Vitesse de l''inducteur');
xlabel('t [s]'), ylabel('\deltaDot [m.s^{-1}]');
grid on;
