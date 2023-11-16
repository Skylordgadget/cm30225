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
subplot(1,2,1);
plot(threads,times, 'LineWidth',3);
set(gca,'xlim',[1 44],'ylim', [0 600]);
xlabel("Threads");
ylabel("Time (s)");
[~,idx] = min(times);
xline(threads(idx),'--r');
text(12,300,"Best performance: " + threads(idx) + " threads",'Color','red');
title("Completion time for a 1000x1000 matrix, 1e-3 precision, 'g' dataset");

subplot(1,2,2);
seq_time_g = 529.040801;
speedup = seq_time_g ./ times;
eff = speedup ./ threads;
yyaxis('left');
plot(threads,speedup,'LineWidth',3);
set(gca,'xlim',[1 44]);
xlabel("Threads");
ylabel("Speedup");
yyaxis('right');
plot(threads,eff.*100, ':','LineWidth',3);
set(gca,'xlim',[1 44]);
ylabel("Efficiency (%)");
title("Speedup & efficiency for a 1000x1000 matrix, 1e-3 precision, 'g' dataset");

speedup(44)

figure(2);
subplot(1,3,1);
seq_time_g = 529.040801;
speedup = seq_time_g ./ times;
eff = speedup ./ threads;
yyaxis('left');
plot(threads,speedup,'LineWidth',2);
set(gca,'xlim',[1 44]);
xlabel("Threads");
ylabel("Speedup");
yyaxis('right');
plot(threads,eff.*100, ':','LineWidth',2);
set(gca,'xlim',[1 44]);
ylabel("Efficiency (%)");
title("Speedup & efficiency for a 1000x1000 matrix, 1e-3 precision, 'g' dataset");

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
plot(threads,speedup,'LineWidth',2);
set(gca,'xlim',[1 44]);
xlabel("Threads");
ylabel("Speedup");
yyaxis('right');
plot(threads,eff.*100, ':','LineWidth',2);
set(gca,'xlim',[1 44]);
ylabel("Efficiency (%)");
title("Speedup & efficiency for a 1000x1000 matrix, 1e-3 precision, '1' dataset");

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
plot(threads,speedup,'LineWidth',2);
set(gca,'xlim',[1 44]);
xlabel("Threads");
ylabel("Speedup");
yyaxis('right');
plot(threads,eff.*100, ':','LineWidth',2);
set(gca,'xlim',[1 44]);
ylabel("Efficiency (%)");
title("Speedup & efficiency for a 1000x1000 matrix, 1e-3 precision, 'r' dataset");



