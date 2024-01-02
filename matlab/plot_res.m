clf; clear;
fpath = "../res/";
% m = readtable(".\bin\out_12t_1000s_p0.5_gay.csv");
% Z = m{:,:};
% [X_s,Y_s] = size(Z);
% [X,Y] = meshgrid(0:X_s-1,0:Y_s-1);
% plt = surf(X,Y,Z);
% set(plt, 'edgecolor', 'none')

m = readtable(fpath + "res_10t_1000s_1e-5p_1m.csv");
Z = m{:,:};
[X_s,Y_s] = size(Z);
[X,Y] = meshgrid(0:X_s-1,0:Y_s-1);
plt = surf(X,Y,Z);
set(plt, 'edgecolor', 'none');
