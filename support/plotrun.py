import sys
import matplotlib.pyplot as plt

x = []
y = []

fname = sys.argv[1]
try:
    f=open(fname,'r')
except:
    print("Cannot open file " + fname)
    exit()
else:
    line = f.readline()
    while line:
        data = line.split()
        x.append( float(data[2]) )
        y.append( float(data[1]) )
        line = f.readline()
    
    plt.plot(x,y)
    
    plt.show()
