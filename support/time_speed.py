import sys
import numpy as np
import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib import cm

x = []
y = []
z = []

mpl.rcParams['axes.xmargin'] = 0
mpl.rcParams['axes.ymargin'] = 0

fig, ax1 = plt.subplots()
ax2 = ax1.twinx()


# 結果ファイル
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
        data = line.split()
        x.append(float(data[1]))
        y.append(float(data[3]))
        z.append(float(data[5])/1000.0)

    ax1.set_ylim([0, 1.2*max(z)])
    ax2.set_ylim([0, 1.5*max(y)])

    # ax1.plot(x, z, drawstyle="default", color=cm.Set1.colors[1])
    ax1.fill_between(x, z, 0, color=cm.Set1.colors[1])
    ax2.plot(x, y, drawstyle="default", color=cm.Set1.colors[2])

    plt.show()
