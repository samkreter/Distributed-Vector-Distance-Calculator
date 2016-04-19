import matplotlib.pyplot as plt
import numpy as np
import pca

ks = [ 100, 500, 1000, 1500, 2000]
ps = [ 8, 16, 24, 32, 40, 48]

float_formatter = lambda x: "%.5f" % x
np.set_printoptions(formatter={'float_kind':float_formatter})


def takeAverageOfFile(filenameIn, filenameOut):
    '''Load a csv and take the average of the sections
         with the same value for first and second columns'''

    data = np.genfromtxt(filenameIn,delimiter=',')
    avgData = []

    for k in ks:
        for p in ps:
            temp = data[np.logical_and(data[:,0] == p,data[:,1] == k)]
            N = temp[:,0].size
            tempSum = [np.sum(temp[:,i])/N for i in range(temp[0].size)]
            avgData.append(tempSum)
    finalData = np.array(avgData)
    np.savetxt(filenameOut,finalData,delimiter=",",fmt='%.5f')
    return finalData

def getSumTimes(data):
    return np.sum(data,axis=0)

def getSumTimesFile():
    data = np.genfromtxt("times.csv",delimiter=',')
    return np.sum(data,axis=0)

def getParalleSpeedUp(data):
    for i in range(data[:,0].size):
        data[i,3] = data[i,5] / data[i,2]
    return data

def plotkValues(data):
    cs = ['r','b','g','y','k']
    for c,k in zip(cs,ks):
        plt.plot(data[data[:,1] == k,0], data[data[:,1] == k,2], c+'o-',label=str(k)+" K value")
    plt.xlabel('Number of Procs')
    plt.ylabel('Time to Process')
    plt.grid(True)
    plt.legend()
    fig1 = plt.gcf()
    fig1.savefig('kValue.png')
    plt.show()

def plotPValues(data):
    cs = ['r','b','g','y','k','m']
    for c,p in zip(cs,ps):
        plt.plot(data[data[:,0] == p,1], data[data[:,0] == p,2], c+'o-',label=str(p)+" P value")

    plt.xlabel('Number of K results')
    plt.ylabel('Time to Process')
    plt.grid(True)
    plt.legend()
    fig2 = plt.gcf()
    fig2.savefig('pValue.png')
    plt.show()



def getLabels(data):
    col1 = data[:,0]
    col2 = data[:,1]

    y = np.column_stack((col1,col2))
    X = data[:,2:]

    return X,y


if __name__ == '__main__':
    test = takeAverageOfFile("times.csv","averagedData.csv")
    test2 = test[test[:,2].argsort()]
    getParalleSpeedUp(test)
    final = test[test[:,2].argsort()]
    np.savetxt("topValuesPar.csv",final[:,:4],delimiter=",",fmt='%.5f')
    # plotPValues(test)
    # plotkValues(test)
    #print(getSumTimes(test))
    #test1 = getParalleSpeedUp(test) * 100