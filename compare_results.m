clf; clear;
alex;
Z_alex = DatasetOut;
m_seq = readtable(".\bin\out_s_1000s_p1e-3.csv");
m_par = readtable(".\bin\out_12t_1000s_p1e-3.csv");
Z_seq = m_seq{:,:};
Z_par = m_par{:,:};

[X_s,Y_s] = size(Z_par);
[X,Y] = meshgrid(0:X_s-1,0:Y_s-1);
figure(1);
subplot(1,2,1);

seq = surf(X,Y,Z_alex);
%label(seq, "alex")
set(seq, 'edgecolor', 'none');
subplot(1,2,2);
par = surf(X,Y,Z_seq);
%title(par, "harry")
set(par, 'edgecolor', 'none');

tol = 1e-5;
LIA = ismembertol(Z_alex,Z_seq,tol);
figure(2);
lia_surf = surf(double(LIA));
set(lia_surf, 'edgecolor', 'none');
if LIA == 1
    disp("All within tol")
else
    disp("Not all within tol")
end

figure(3);
comp = surf(Z_alex-Z_par);
set(comp, 'edgecolor', 'none');