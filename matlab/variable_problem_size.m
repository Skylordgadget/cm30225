clf; clear;
fpath = "../res/";
m = readtable(fpath + "variable_size_1e-3p_1s.csv");
M = m{:,:};

t1 = M(1:10,2:3);
T1 = sortrows(t1,1);
t4 = M(11:20,2:3);
T4 = sortrows(t4,1);
t8 = M(21:30,2:3);
T8 = sortrows(t8,1);
t16 = M(31:40,2:3);
T16 = sortrows(t16,1);
t32 = M(41:50,2:3);
T32 = sortrows(t32,1);

S = 1000:1000:10000;

hold on;
plot(S,T1(:,2),'-o','LineWidth',2);
plot(S,T4(:,2),'-o','LineWidth',2);
plot(S,T8(:,2),'-o','LineWidth',2);
plot(S,T16(:,2),'-o','LineWidth',2);
plot(S,T32(:,2),'-o','LineWidth',2);
yline(5,'--r', 'LineWidth', 2);
xlabel("Square Matrix Width");
ylabel("Completion Time (s)");
title("Completion time for 1e-3 precision of '1' dataset");
set(gca,'ylim',[0,30])
legend("1 thread", "4 threads", "8 threads", "16 threads", "32 threads");
grid ON;
