clf; clear
fpath = "../res/";
seq_time = 12.109963;

m = readtable(fpath + "test_4000s_1e-3p_1s_1-44t.csv");
M = m{:,:};
x = M(:,1);
y = M(:,2);
z = M(:,3);

speedup = seq_time ./ z;
eff = speedup ./ x;
yyaxis('left');
plot(x,speedup,'LineWidth',3);
set(gca,'xlim',[1 44]);
xlabel("Threads");
ylabel("Speedup");
yyaxis('right');
plot(x,eff.*100, ':','LineWidth',3);
set(gca,'xlim',[1 44]);
ylabel("Efficiency (%)");
title("Speedup & efficiency for a 4000x4000 matrix, 1e-3 precision, '1' dataset");