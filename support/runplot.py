import sys
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt

if len(sys.argv) < 3:
    print("Usage: python runplot.py line_file output_file")
    exit()

x = []
y = []
x2 = []
y2 = []

mpl.rcParams['axes.xmargin'] = 0
mpl.rcParams['axes.ymargin'] = 0
plt.ylim([0,120])
plt.xlim([0,3000])
# Load the line data file
try:
    fname = sys.argv[1]
    f=open(fname,'r')
except:
    print("Cannot open file " + fname)
    exit()
else:
    lines = f.readlines()
    f.close()

    for line in lines:
        s = line.lstrip()
        if len(s) > 1 and s[0] != "#":
            data = s.split()
            x.append(float(data[1]))
            y.append(float(data[3]))

    plt.plot(x, y, drawstyle="steps-post")
# Load the output file
try:
    fname = sys.argv[2]
    f=open(fname,'r')
except:
    print("Cannot open file " + fname)
    exit()
else:
    lines = f.readlines()
    f.close()

    for line in lines:
        data = line.split()
        x2.append(float(data[2]))
        y2.append(float(data[3]))

    plt.plot(x2, y2, drawstyle="default")

plt.xlabel('distance(m)')
plt.ylabel('speed(km/h)')
plt.show()
