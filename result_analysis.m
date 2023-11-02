clf; clear;
m = readtable(".\bin\hkr33_result.csv");
Z = m{:,:};
[X_s,Y_s] = size(Z);
[X,Y] = meshgrid(0:X_s-1,0:Y_s-1);
surf(X,Y,Z)

% m = readtable(".\batch_run_result.csv");
% M = m{:,:};
% x = M(:,1);
% y = M(:,2);
% z = M(:,3);
% 
% xi = unique(x); yi = unique(y);
% [X,Y] = meshgrid(xi,yi);
% Z = reshape(z,size(X));
% surf(X,Y,Z)
% set(gca,'xlim',[1 44],'ylim',[64 4096]);
% xlabel("No. Threads");
% ylabel("Array Size");
% zlabel("Completion Time (Seconds)");

% [X,Y] = meshgrid(1:0.5:10,1:20);
% Z = sin(X) + cos(Y);
% surf(X,Y,Z)
