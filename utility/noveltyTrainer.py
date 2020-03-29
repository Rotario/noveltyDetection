#!/usr/bin/env python
# coding: utf-8

# In[25]:


#!/usr/bin/env python
#Imports for helper functions
from sklearn.preprocessing import MinMaxScaler

from sklearn import svm
import pandas as pd
import numpy as np
from io import StringIO
import sys
import ast
import json

import PySimpleGUI as sg  
from io import StringIO


# In[26]:



def svm_scale(X, path="svm.par", new_range = (-1, 1)):
    #Taken from geompalik, https://stackoverflow.com/questions/48425128/converting-cli-libsvm-to-python-how-to-use-svm-scale
    if (isinstance(path, str)):
        file = open(path)
    elif (isinstance(path, StringIO)):
        file = path
        file.seek(0)
    f = file.read().splitlines()[2:] 
    f = np.array([x.split() for x in f]).astype(float)
    my_min, my_max = f[:,1], f[:,2] # second/third col is the feat. mins/max
    X_std = (X - my_min) / (my_max - my_min) # This brings at (0,1)
    X_scaled = X_std * (new_range[1] - new_range[0]) + new_range[0] # Modifies min/max

    #If file was opened needs to be closed
    if (isinstance(path, str)):
        file.close()
    return X_scaled.tolist()

def svm_save_scale_params(scale_min, scale_max, scale_range = [-1, 1], path = 'svm.par'):
    assert(len(scale_min) == len(scale_max))

    if (isinstance(path, str)):
        f = open(path, 'w', newline='\r\n')
    elif (isinstance(path, StringIO)):
        f = path
        f.seek(0)
    else:
        raise Exception('No correct path specified, neither StringIO or string')

    f.write('x\n')
    f.write(' '.join(map(str, scale_range)) + '\n')
    #iterate through ranges
    for idx, item in enumerate(scale_min):
        f.write(str(idx+1) + ' ' + str(scale_min[idx]) + ' ' + str(scale_max[idx]) + '\n')

    if (isinstance(path, str)):
        f.close()
    elif (isinstance(path, StringIO)):
        f.seek(0)
        return f
    return path

def svm_save_model_params(clf, path='svm.mod'):

    if (isinstance(path, str)):
        f = open(path, 'w', newline='\r\n')
    elif (isinstance(path, StringIO)):
        f = path
        f.seek(0)
    else:
        raise Exception('No correct path specified, neither StringIO or string')

    f.write('svm_type one_class \n')
    f.write('kernel_type %s \n' % clf.get_params(True)['kernel'])
    f.write('gamma %s \n' % clf.get_params(True)['gamma'])
    f.write('nr_class %s \n' % 2)
    f.write('total_sv %s \n' % len(clf.dual_coef_[0]))
    f.write('rho %s \n' % clf.offset_[0])
    f.write('SV \n')
    for i, vecs in enumerate(clf.support_vectors_):
        f.write('%.12g ' % clf.dual_coef_[0][i])
        for j, vec in enumerate(vecs):
            f.write('%i:%.8g ' % (j+1, vec,))
        f.write('\n')

    if (isinstance(path, str)):
        f.close()
    elif isinstance(path, StringIO):
        f.seek(0)
        #return StringIO
        return f
    #Return string as we're using filenames not StringIOs
    return path

def generateSVMOneClass(X_train, save=1):
    scale_range = [-1, 1]
    scaler = MinMaxScaler(feature_range=scale_range, copy=True)
    scaler.fit(X_train)
    #Generate file like string for params
    if (save==1):
        scaleF = "svm.par"
    else:
        scaleF = StringIO()
    scaleF = svm_save_scale_params(scaler.data_min_, scaler.data_max_, path=scaleF)
    dataScaled = svm_scale(X_train, path=scaleF)

    #all data is good (as far as we know!)
    buf = np.ones(len(X_train))
    #Define problem with scaled data

    #USE SKLEARN
    # fit the model
    clf = svm.OneClassSVM(nu=0.01, kernel="rbf", gamma=1/np.shape(X_train)[1])
    clf.fit(dataScaled)
    #Generate file like string for model
    if (save == 1):
        modelF = "svm.mod"
    else:
        modelF = StringIO()
    modelF = svm_save_model_params(clf, path=modelF)
    #labels_pred, acc, probs = svm_predict([-1, 1], [[1, 1, 1], [0, 0, 1]], m)              
    return scaler, clf, scaleF, modelF;

def isIndex(col):#used to check if first col is an index
    first = 0
    num = 0
    for i in range(len(col)):
        if first == 0:
            first = 1
            num = col[i]
        else:
            if col[i] != num:
                return 0
        num += 1
    return 1

def readCSV(file_path):
    try:
        df = pd.read_csv(file_path, header=None)
        if isIndex(df.iloc[:,0]):
            df = df.drop(columns=0)
        train = df.to_numpy()
    except Exception as e:
        raise Exception(e)
    return train







#scaler, clf, scaleF, modelF = generateSVMOneClass(X_train)


# In[27]:


err = 0
sg.ChangeLookAndFeel("Reddit")
layout = [[sg.Text('Pick your data file', font=("Helvetica", 15))],      
                 [sg.Input(), sg.FileBrowse(), sg.Text('', size=(16,1), key='_ERROR_', text_color='red', font=("Helvetica", 12))], 
                 [sg.Text('Info', font=("Helvetica", 12))],
                [sg.Text('', size=(32,5), key='_SCORE_', font=("Helvetica", 12))],
                 [sg.Button('Submit', font=("Helvetica", 12)), sg.Button('Cancel', font=("Helvetica", 12))]]
window = sg.Window('Simple SVM Trainer', layout)    

while True:
    
    event, values = window.Read()    
    
    if event == None or event == 'Cancel':
          break

    if event == 'Submit':
        if values[0]:
            file_path = values[0]
            try:
                train = readCSV(file_path)
                generateSVMOneClass(train)

            except Exception as e:
                err = 1
                window.Element('_SCORE_').Update("Reading Data File Failed:\n%s" % (e,))
                
            if not err:
                window.Element('_SCORE_').Update("svm.mod and svm.par written!")
            #window.Element('_ERROR_').Update('')
            
        else:
            window.Element('_ERROR_').Update("No File Selected")
    
window.Close()

