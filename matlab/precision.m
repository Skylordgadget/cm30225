clf; clear;
fpath = "../res/";

m = readtable(fpath + "precision.csv");
M = m{:,:};
x = M(:,4);
y = M(:,3);

x=x';
y=y';
loglog(x,y,'LineWidth',3);
set(gca, 'XDir','reverse')
grid ON;
xlabel("Precision");
ylabel("Completion Time (s)");
title("Completion time for 1000x1000 matrix, dataset '1'");
xticks(flip(x,2));
yticks(y);
