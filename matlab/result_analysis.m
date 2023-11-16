clf; clear;
fpath = "../res/";
% m = readtable(".\bin\out_12t_1000s_p0.5_gay.csv");
% Z = m{:,:};
% [X_s,Y_s] = size(Z);
% [X,Y] = meshgrid(0:X_s-1,0:Y_s-1);
% plt = surf(X,Y,Z);
% set(plt, 'edgecolor', 'none')

m = readtable(fpath + "ultra_test_10_2000_44t_r.csv");
M = m{:,:};
x = M(:,1);
y = M(:,2);
z = M(:,3);

xi = unique(x); yi = unique(y);
[X,Y] = meshgrid(xi,yi);
Y = sortrows(Y);
Z = reshape(z,size(X));
Z = sortrows(Z);
Z_log=log10(Z);
plt = mesh(X,Y,Z);
set(gca,'xlim',[1 44],'ylim',[10 2000]);
set(plt, 'facecolor', 'flat')
xlabel("No. Threads");
ylabel("Array Size");
zlabel("Completion Time (Seconds)");
title("Completion time for 1e-3 precision of 'r' dataset");

% [X,Y] = meshgrid(1:0.5:10,1:20);
% Z = sin(X) + cos(Y);
% surf(X,Y,Z)
