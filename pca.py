import numpy as np
import matplotlib.pyplot as plt
import math
from sklearn.cross_validation import train_test_split
from sklearn.preprocessing import StandardScaler
import pandas as pd
from sklearn.datasets import make_moons

def generateDataSeparableData(N, l, u,mu,Var):
    '''Generate a dataset from a gaussian distribution separable by PCA'''

    #compute the error for the y value based off a guassian dist
    e = np.random.normal(0,1,N/2)

    #compute the value for the first class with specified mu
    value1 = np.random.normal(mu,Var,N/2)
    #add the error and merge the x and y value
    x1 = np.column_stack((value1,np.add(np.copy(value1),e)))
    #add 5 to the mu value to offset the set
    value2 = np.random.normal(mu+5,Var,N/2)
    x2 = np.column_stack((value2,np.add(np.copy(value2),e)))
    #combine the classes
    x = np.concatenate([x1,x2])
    #classiffy half ones half zeros
    y1 = np.ones(N/2)
    y2 = np.zeros(N/2)
    y = np.concatenate([y1,y2])

    return x,y

def generateDataNonSeparableData(N, l, u,mu,Var):
    '''Generate a dataset from a gaussian distribution Not separable by PCA'''

    #set how much to move the top distribution above the lower
    step = np.array([N/2])
    step.fill(5)
    #find the value from the distro
    value = np.random.normal(mu,Var,N/2)
    #add the step to the first dataset's y value
    x1 = np.column_stack((value,np.add(np.copy(value),step)))
    x2 = np.column_stack((value,np.copy(value)))
    x = np.concatenate([x1,x2])
    #same as above just create equal classifications
    y1 = np.ones(N/2)
    y2 = np.zeros(N/2)
    y = np.concatenate([y1,y2])

    return x,y


if __name__ == '__main__':

    #getting the user option for seperable or non seperable
    # inp = input("enter 1 for seperable dataset and 2 for non: ");

    # if(inp == "1"):
    #     X,y = generateDataSeparableData(100,2,5,3,1)
    # else:
    #     X,y = generateDataNonSeparableData(100,2,5,3,1)

    #plot the original dataset
    plt.scatter(X[y==0,0], X[y==0, 1], color='red', marker = '^', alpha=0.5)
    plt.scatter(X[y==1,0], X[y==1, 1], color='blue', marker = 'o', alpha=0.5)
    plt.title("Original 2D dataset")
    plt.show()

    X_train, X_test, y_train, y_test = train_test_split(X,y,test_size=0.3)

    sc      = StandardScaler()
    X_train_std = sc.fit_transform(X_train)
    X_test_std  = sc.fit_transform(X_test)


    #compute covariance, eigenvals and eigenvecs
    cov_mat = np.cov(X_train_std.T)
    # print(cov_mat)
    eigen_vals, eigen_vecs = np.linalg.eig(cov_mat)
    # print('\nEigenvalues \n%s' % eigen_vals)

    # #compute explained variance and plot
    tot         = sum(eigen_vals)
    var_exp     = [(i/tot) for i in sorted(eigen_vals, reverse=True)]
    cum_var_exp = np.cumsum(var_exp)


    # #perform dimensionality reduction
    eigen_pairs = [(np.abs(eigen_vals[i]), eigen_vecs[:,i]) for i in range(len(eigen_vals))]
    eigen_pairs.sort(reverse=True)

    #get the eigen vector multiplier
    ###################
    #This gets 2 eigaen vectors but I am only using the top eigen vector for plotting since
    # I'm already in 2D and want to go to 1D
    ####################
    w = np.hstack((eigen_pairs[0][1][:, np.newaxis], eigen_pairs[1][1][:, np.newaxis]))

    X_train_pca = X_train_std.dot(w)
    cov_mat = np.cov(X_train_pca.T)

    #plot the pca results
    colors = ['r','b']
    markers = ['s', 'x']
    for l,c,m in zip(np.unique(y_train), colors, markers):
      plt.scatter(X_train_pca[y_train==l,0],np.zeros((len(X_train_pca[y_train==l,0]),1)), c=c, label=l, marker=m)
    plt.title("After PCA from 2D to 1D")
    plt.legend(loc='lower left')
    plt.show()

