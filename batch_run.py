import subprocess
import time

def exp_range(start, end, mul):
    while start <= end:
        yield start
        start *= mul

f = open(".\\bin\\batch_run_result.csv", "w")
print("running batch")
for t in range(1,45):
    for s in exp_range(64, 4096, 2):
        threads = "-t " + str(t)
        size = "-s " + str(s)
        precision = "-p 1e-3"
        start = time.time()
        subprocess.check_call([r"C:\Users\\harry\OneDrive - University of Bath\CM30225\coursework\coursework 1\build\cwk1.exe", threads, size, precision])
        end = time.time()
        runtime = end-start
        f.write("{0}, {1}, {2}\n".format(t, s, runtime))


f.close()
exit()



