import numpy as np

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
    results = getSumTimes(data)
    return results[5] / results[2]


if __name__ == '__main__':
    test = takeAverageOfFile("times.csv","averagedData.csv")
    test2 = test[test[:,2].argsort()]
    print(test2[])
    #print(getSumTimes(test))
    #print(getSumTimes())
    #test1 = getParalleSpeedUp(test) * 100