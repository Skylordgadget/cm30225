clf; clear
fpath = "../res/";
m = readtable(fpath + "ultra_test_10_2000_44t_g.csv");
M = m{:,:};
x = M(:,1);
y = M(:,2);
z = M(:,3);

xi = unique(x); yi = unique(y);
[X,Y] = meshgrid(xi,yi);
Y = sortrows(Y);
Z = reshape(z,size(X));
Z = sortrows(Z);

times = Z(6,1:44);
threads = 1:44;

figure(1);
plot(threads,times);
set(gca,'xlim',[1 44],'ylim',[min(times) max(times)]);
xlabel("Threads");
ylabel("Time (s)");

figure(2);
subplot(1, 3, 1);
seq_time_g = 529.040801;
speedup = seq_time_g ./ times;
eff = speedup ./ threads;
yyaxis('left');
plot(threads,speedup);
set(gca,'xlim',[1 44]);
xlabel("Threads");
ylabel("Speedup");
yyaxis('right');
plot(threads,eff.*100, '--');
set(gca,'xlim',[1 44]);
ylabel("Efficiency (%)");
title("Starting array: g");



m = readtable(fpath + "ultra_test_10_2000_44t_1s.csv");
M = m{:,:};
x = M(:,1);
y = M(:,2);
z = M(:,3);

xi = unique(x); yi = unique(y);
[X,Y] = meshgrid(xi,yi);
Y = sortrows(Y);
Z = reshape(z,size(X));
Z = sortrows(Z);

times = Z(6,1:44);
threads = 1:44;

subplot(1, 3, 2);
seq_time_1 = 0.672817;
speedup = seq_time_1 ./ times;
eff = speedup ./ threads;
yyaxis('left');
plot(threads,speedup);
set(gca,'xlim',[1 44]);
xlabel("Threads");
ylabel("Speedup");
yyaxis('right');
plot(threads,eff.*100, '--');
set(gca,'xlim',[1 44]);
ylabel("Efficiency (%)");
title("Starting array: 1");

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

times = Z(6,1:44);
threads = 1:44;

subplot(1, 3, 3);
seq_time_r = 3.18306;
speedup = seq_time_r ./ times;
eff = speedup ./ threads;
yyaxis('left');
plot(threads,speedup);
set(gca,'xlim',[1 44]);
xlabel("Threads");
ylabel("Speedup");
yyaxis('right');
plot(threads,eff.*100, '--');
set(gca,'xlim',[1 44]);
ylabel("Efficiency (%)");
title("Starting array: r");



