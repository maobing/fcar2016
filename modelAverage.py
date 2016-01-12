#!/usr/bin/python
#----------------------------------------
# model stacking
# @NOTE: this script uses
#    a naive linear combination
#    to integrate results from 
#    base learner (not boosting)
# @NOTE: benchmark is automatically added
# @NOTE: train data's control is automaticall
#   extracted and used for inference
# @NOTE: only top 3 models are averaged
# @NOTE: pass weights and control prediction
#   to R too
# @NOTE: R is too slow, only using it for plot
#   add the other functions here
#-----------------------------------------
import shlex, subprocess, sys
import os.path, re, time
import collections
import createBenchmarkFeature as bm

#---
# check args
#---
def menu(argv) :
  if len(argv) == 1 :
    print "/*----------------------------------------*/"
    print "/*              model stack               */"
    print "/*----------------------------------------*/"
    print "/* -model model1,model2,model3,...        */"
    print "/* -train trainFile: train file           */"
    print "/* -test testFile: test file              */"
    print "/* -output outputFile: output file        */"
    print "/* -k k: benchmark average features to k group */"
    print "/*----------------------------------------*/"
    return -1

  idx = 1 # argv[0] is the script name
  modelOK = 0; trainOK = 0; testOK = 0; outOK = 0; kOK = 0

  model = []
  trainFile = None; testFile = None; outputFile = None; k = None

  while idx < len(argv) :
    if sys.argv[idx] == '-model' :
      idx += 1
      model = sys.argv[idx].split(',')
      modelOK = 1
    elif sys.argv[idx] == '-train' :
      idx += 1
      trainFile = sys.argv[idx]
      trainOK = 1
    elif sys.argv[idx] == '-test' :
      idx += 1
      testFile = sys.argv[idx]
      testOK = 1
    elif sys.argv[idx] == '-output' :
      idx += 1
      outputFile = sys.argv[idx]
      outOK = 1
    elif sys.argv[idx] == '-k' :
      idx += 1
      k = int(sys.argv[idx])
      kOK = 1
    else : 
      print "Unknow parameters", sys.argv[idx]
      return -1
    idx += 1

  if modelOK+trainOK+testOK+outOK+kOK < 5 :
    print "Missing one parameter! See below: "
    print "/*----------------------------------------*/"
    print "/*              model stack               */"
    print "/*----------------------------------------*/"
    print "/* -model model1,model2,model3,...        */"
    print "/* -train trainFile: train file           */"
    print "/* -test testFile: test file              */"
    print "/* -output outputFile: output file        */"
    print "/* -k k: benchmark average features to k group */"
    print "/*----------------------------------------*/"
    return -1
  return (model,trainFile,testFile,outputFile,k)

#---
# add benchmark automatically
#---
def addBenchmark(featureFile, k) :
  print '-----------------------------------'
  bmFeatureFile = ''.join([featureFile,'_Benchmark'])
  print '- adding benchmark ', bmFeatureFile
  if os.path.isfile(bmFeatureFile) == False :
    bmFeatureFile = bm.createBenchmarkFeature(featureFile, k)
  if bmFeatureFile < 0:
    print 'Error when creating benchmark features for %s' % featureFile
    return -1
  return 0

#---
# addDiffPenalty
#---
def addDiffPenalty(model,cs) :
  print '-----------------------------------'
  tmpModel = []
  rmModel = []
  for m in model :
    if m != 'RandomForest' and m != 'Benchmark' : # rf is the only model without c
      tmpModel += [ m+'_'+repr(c) for c in cs ]
      rmModel.append(m)

  for a in rmModel :
    model.remove(a) # remove

  model.extend(tmpModel)

  print '- Models added for different values of c: %s' % ','.join(tmpModel)
  return 0

#---
# trainModels
#---
def trainModels(trainFile, testFile, model) :
  for mymodel in model :
    print '------------------------------------'
    print '- training %s using %s' % (mymodel, trainFile)

    if os.path.isfile('./%s_trained%s' % (trainFile, mymodel)) :
      continue
    if mymodel == 'Benchmark' :
      cmd = './trainModel -m LogisticRegressionL1 -c 10000 -t %s -o ./%s_trained%s' \
        % (trainFile+'_Benchmark', trainFile, mymodel)
    elif mymodel == 'RandomForest' :
      cmd = './trainModel -m %s -c 1 -t %s -o ./%s_trained%s' % \
        (mymodel, trainFile, trainFile, mymodel)
    else : # model with c value added
      (modelname, modelc) = mymodel.split('_')
      print '-- with c = %s' % modelc
      cmd = './trainModel -m %s -c %s -t %s -o ./%s_trained%s' %  \
        (modelname, modelc, trainFile, trainFile, mymodel)

    print '- cmd is %s' % cmd

    # run model
    myargs = shlex.split(cmd);
    start = time.time()
    p = subprocess.Popen(myargs, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
    for x in p.stdout.read().split('\n') :
      if re.search(r"Accuracy|nonzero", x) :
        print x
    for x in p.stderr.read().split('\n') :
       if re.search(r"Accuracy|core", x) :
         print x
    elapsed = time.time() - start
    print 'elapsed time: %.8f' % elapsed

#---
# predictModels
#---
def predictModels(trainFile, testFile, model):
  for mymodel in model :
    print '------------------------------------'
    print '- predicting %s using %s' % (testFile, mymodel)
    if os.path.isfile('./%s_result%sBy%s' % (testFile,mymodel, os.path.basename(trainFile)[:4])) :
      continue
    if mymodel == 'Benchmark':
      cmd = './predictModel -m LogisticRegressionL1 -tm ./%s_trained%s -train %s -test %s -o ./%s_result%sBy%s' \
        % (trainFile, mymodel, trainFile+'_Benchmark', testFile+'_Benchmark', testFile, \
            mymodel, os.path.basename(trainFile)[:4])
    else :
      modelname = mymodel.split('_')[0]
      cmd = './predictModel -m %s -tm ./%s_trained%s -train %s -test %s -o ./%s_result%sBy%s' \
        % (modelname, trainFile, mymodel, trainFile, testFile, \
          testFile, mymodel, os.path.basename(trainFile)[:4])

    print '- cmd is %s' % cmd

    # run predict model
    myargs = shlex.split(cmd)
    start = time.time()
    p = subprocess.Popen(myargs, stdout = subprocess.PIPE, stderr = subprocess.PIPE)
    for x in p.stdout.read().split('\n') :
      if re.search(r"Accuracy", x) :
        print x  
    for x in p.stderr.read().split('\n') :
      if re.search(r"Accuracy|core", x) :
        print x
    elapsed = time.time() - start
    print 'elapsed time: %.8f' % elapsed

#---
# collectResults
#---
def collectResults(trainFile, testFile, model) :
  
  print '------------------------------------'
  print '- collecting results for %s ' % testFile

  # first read ys
  p = subprocess.Popen(shlex.split('cut -f 1 -d\  %s' % testFile),stdout=subprocess.PIPE)
  ys = [ int(a) for a in p.stdout.read().split('\n') if len(a) > 0 ] # last element is ''
  
  result = []
  # then read result
  # print 'models is ', model
  for mymodel in model :
    tmpResultFile = './%s_result%sBy%s' % (testFile, mymodel, os.path.basename(trainFile)[:4])
    file = open(tmpResultFile, "r")
    if re.match(r"LogisticRegression|Benchmark", mymodel) :
      tmp = file.read().splitlines()
      labels = tmp[0].split(' ')
      if labels[1] == '1':
        labelIdx = 1
      if labels[2] == '1':
        labelIdx = 2
      result.append([ float(x.split(' ')[labelIdx]) for x in tmp[1:] ])
    else :
      result.append([ float(x) for x in file.read().splitlines()] )
    file.close()
  
  return (ys, result)

#---
# err func
#    low-level
#---
def err(prediction, truth) :
  if len(prediction) != len(truth) :
    print "length of prediction and truth are not the same\n"
    print 'len of prediction is %d and len of truth is %d' % (len(prediction),len(truth))
    return -1
  err = 0.0
  for i in range(len(prediction)) :
    err += (float(prediction[i]) - float(truth[i]))**2 # mse
  err = err/(len(prediction)-1)
  return err

#---
# getVotingWeights
#---
def getVotingWeights(ys, result) :
  print '------------------------------------'
  print '- calculating voting weights'
  errs = [ err(yhat, ys) for yhat in result ]
  precisions = [ 1/a for a in errs ]
  weights = [ a/sum(precisions) for a in precisions ]
  print "weights:", weights
  return weights

#---
# getVotingResults
#---
def getVotingResults(result, weights, top) :
  print '------------------------------------'
  print '- conducting model voting'
  votedResult = []
  if len(result) != len(weights) :
    print "len of result is not equal to len of weights"
    return -1
  topWeights = sorted(weights, reverse = True)[:3]
  print 'topWeights is:', topWeights
  for i in range(len(result[0])) :
    tmp = 0.0
    for j in range(len(weights)) :
      if weights[j] in topWeights :
        # renomalize topWeights
        tmp += float(result[j][i])*(weights[j]/sum(topWeights))
    votedResult.append(tmp)

  return votedResult

#---
# writeFormatted
# format: 1 1:0.123 2:1.234 ..
#--
def writeFormatted(ys, result, outputFile) :
  print '------------------------------------'
  print '- writing to ', outputFile
  fp = open(outputFile, 'w')
  for i in range(len(ys)) :
    fp.write(repr(ys[i])+' ')
    for j in range(len(result)) :
      if j == len(result) - 1 :
        fp.write(''.join( [repr(j+1),':', repr(result[j][i]), '\n'] ))
      else :
        fp.write(''.join( [repr(j+1),':', repr(result[j][i]), ' '] ))
  return 0

#---
# reserveControl
# split train into train and control
# include m controls in trainFile
#---
def reserveControl(trainFile, m) :
  f = open(trainFile, 'r')
  f_train = open(trainFile + '_train', 'w')
  f_ctrl = open(trainFile + '_control', 'w')
  ctrl = 0
  idx = 0
  for line in f :
    if ctrl == 0:
      f_train.write(line)
      if idx == 0 and re.match(r"0 ", line) :
        ctrl = 1
    else :
      f_ctrl.write(line)
      idx += 1
      if idx > m :
        ctrl = 0
  f_train.close()
  f_ctrl.close()
  return 0


#----------------------------------
# @UPDATE: combine ROC,estFDR,trueFDR
#   into one inference function
#   to speed up the code
#-----------------------------------
def calcAUC(y,x) :
  print '--------------------------------'
  print '- calcAUC'
  if len(x) != len(y):
    print 'x y lens differ!'
    return -1
  # assumed that y, x are sorted in decreasing order
  area = 0.0
  for i in range(1,len(y)):
    area += 1.0/2.0*(y[i-1]+y[i])*(x[i]-x[i-1])*(-1)
  return area


## get index to unique values in a list
## ref: http://stackoverflow.com/questions/27411142/how-to-get-lists-of-indices-to-unique-values-efficiently
def uniqueValueIdx(a):
  d = collections.defaultdict(list)
  for i,j in enumerate(a):
    d[j].append(i)
  return d

def doInference(Y, pred, ctrl):
  # y axis for all 3 plots
  sens = []

  # x axis for each of 3 plots
  FPR = []
  estFDR = []
  trueFDR = []

  # auc summary for each plot
  auc = []

  # all positive, all negative
  all = len(Y)
  allP = sum(Y)
  allN = all - allP

  # sort Y according to pred
  predY = sorted(zip(pred, Y))
  predSorted = [ a for (a,b) in predY ]
  YSorted = [ b for (a,b) in predY ]

  # use only unqiue values by get set()
  uniquePred = sorted(list(set(pred)))
  uniquePredIdx = uniqueValueIdx(predSorted)
  # get p value for each unique pred values for estFDR
  # @IMPORTANT: uniquePvalues have duplicates, when get
  ##  their rank (k) to adjust for pvalue, be careful
  uniquePvalue = [ calcPvalue(a,ctrl) for a in uniquePred ]   
  # @ADDED: the BH method of p.adjust is not merely p*m/k
  # REF: R source code for p.adjust fdr,
  # http://r.789695.n4.nabble.com/BH-correction-with-p-adjust-tc4671988.html#none
  tmpMin = 0.0

  # inference
  for i in range(len(uniquePred)) :

    tmpIdx = uniquePredIdx[uniquePred[i]][0]
    tmpY = YSorted[tmpIdx:]
    sumTmpY = sum(tmpY)
    lenTmpY = len(tmpY)

    # sens
    sens.append( float(sumTmpY) / float(allP) )
    # FPR
    FPR.append( float((lenTmpY-sumTmpY)) / float(allN) )
    # true FDR
    trueFDR.append( float(lenTmpY-sumTmpY) / float(len(tmpY)) )
    # est FDR
    tmpEstFDR = uniquePvalue[i] * float(all) / ( float(all - tmpIdx) )
    if i == 0: 
      tmpMin = tmpEstFDR
    elif tmpEstFDR < tmpMin :
      tmpMin = tmpEstFDR
    estFDR.append( min(tmpMin, 1.0) )

  # add last values
  sens.append( 0.0 )
  FPR.append( 0.0 )
  trueFDR.append( 0.0 )
  estFDR.append( 0.0 )

  # make sure true FDR is decreasing
  ##  need adjust from last FDR to first FDR
  for i in range(1,len(trueFDR)) :
    if trueFDR[i-1] < trueFDR[i] :
      trueFDR[i] = trueFDR[i-1]

  # adjustPvalue has a problem, rank (k) is discrete
  uniqueEstFDR = uniqueValueIdx(uniquePvalue)


  # return
  return (sens, FPR, estFDR, trueFDR)

def calcPvalue(value, ctrl) :
  return float(sum(( int(a >= value) for a in ctrl )))/float(len(ctrl))

# main
# @NOTE: after adding inference
#   this script implicitly assumes that
#   final train data ends with `_train`
#   control data for inference ends with `_control`
#   test data: original test data name
#---
def main(argv) :

  if menu(argv) != -1:
    (model, trainFile, testFile, outputFile, k) = menu(argv)
  else :
    sys.exit(-1)

  # reserve control for inference
  ## split train into train+'_train'
  ## and train + '_control'
  m = 500
  reserveControl(trainFile, m)

  # add benchmark
  ## automatically add '_Benchmark' to end of filename
  model.append('Benchmark')
  addBenchmark(trainFile + '_train', k)
  addBenchmark(trainFile + '_control', k)
  addBenchmark(testFile, k)

  # add penalty for penalty-based method
  cs = [0.001, 0.01, 0.1]
  addDiffPenalty(model, cs)

  # train model
  trainModels(trainFile + '_train', testFile,  model)

  # predict model
  ## predict train to get error
  predictModels(trainFile + '_train', trainFile + '_train', model) 
  ## predict train control to get inference
  predictModels(trainFile + '_train', trainFile + '_control', model) 
  ## predict test
  predictModels(trainFile + '_train', testFile, model)

  # collect results
  start = time.time()
  ## collect train
  (trainYs, trainResult) = collectResults(trainFile + '_train', \
      trainFile + '_train', model) 
  elapsed = time.time() - start
  print '- time elapsed ', elapsed

  ## collect control
  start = time.time()
  (controlYs, controlResult) = collectResults(trainFile + '_train', \
      trainFile + '_control', model) 
  elapsed = time.time() - start
  print '- time elapsed ', elapsed

  ## collect test
  start = time.time()
  (testYs, testResult) = collectResults(trainFile + '_train', \
      testFile, model) 
  elapsed = time.time() - start
  print '- elapsed time is ', elapsed

  weights = getVotingWeights(trainYs, trainResult)
  top = 3
  votedResult = getVotingResults(testResult, weights, top)
  votedResultControl = getVotingResults(controlResult, weights, top)

  # write result
  testResult.append(votedResult)
  controlResult.append(votedResultControl)

  model.append('voting')

  tmpmodel = list(set([ a.split('_')[0] for a in model]))
  outputFile = ''.join( ['../',os.path.basename(trainFile), \
      os.path.basename(testFile),','.join(tmpmodel), '_result'] )

  writeFormatted(testYs, testResult, outputFile)
  
  # conduct inference
  start = time.time()

  ## ROC
  testROC = []
  testAUC = []
  ## est FDR
  estFDR = []
  estFDRAUC = []
  ## true FDR
  trueFDR = []
  trueFDRAUC = []

  for i in range(len(testResult)) :
    print '--------------------------------------'
    print '- doing Inference for ', model[i]

    (sensi, FPRi, estFDRi, trueFDRi) = doInference(testYs, testResult[i], controlResult[i])

    # test results ROC, plot1
    testAUC.append(calcAUC(sensi,FPRi))
    print "- %s's auc is %f" % (model[i], testAUC[-1])
    testROC.append(sensi)
    testROC.append(FPRi)

    # test results, estFDR, plot2
    estFDRAUC.append(calcAUC(sensi,estFDRi))
    print "- %s's est FDR auc is %f" % (model[i], estFDRAUC[-1])
    estFDR.append(sensi)
    estFDR.append(estFDRi)

    # test results, trueFDR, plot 3
    trueFDRAUC.append(calcAUC(sensi,trueFDRi))
    print "- %s's true FDR auc is %f" % (model[i], trueFDRAUC[-1])
    trueFDR.append(sensi)
    trueFDR.append(trueFDRi)

  elapsed = time.time() - start
  print 'elapsed time is ', elapsed

  # write ROC
  outputFileROC = ''.join( ['../',os.path.basename(trainFile), \
      os.path.basename(testFile),','.join(tmpmodel), '_rocResult'] )

  f = open(outputFileROC, 'w')

  ## now ROC have different lens for different models
  ## as we use the unique values
  lenROC = [ len(a) for a in testROC ]

  for j in range( max(lenROC) ):
    for i in range(len(testROC)):
      if i == len(testROC) - 1:
        try: 
          f.write(repr(testROC[i][j])+'\n')
        except IndexError:
          f.write('NA'+'\n')
      else:
        try: 
          f.write(repr(testROC[i][j])+' ')
        except IndexError:
          f.write('NA'+' ')
  f.close()

  # write est FDR
  outputFileEstFDR = ''.join( ['../',os.path.basename(trainFile), \
      os.path.basename(testFile),','.join(tmpmodel), '_estFDRResult'] )

  f = open(outputFileEstFDR, 'w')

  ## now estFDR have different lens for different models
  ## as we use the unique values
  lenEstFDR = [ len(a) for a in estFDR ]

  for j in range( max(lenEstFDR) ):
    for i in range(len(estFDR)):
      if i == len(estFDR) - 1:
        try:
          f.write(repr(estFDR[i][j])+'\n')
        except IndexError:
          f.write('NA'+'\n')
      else:
        try:
          f.write(repr(estFDR[i][j])+' ')
        except IndexError:
          f.write('NA'+' ')

  f.close()

  # write true FDR
  outputFileTrueFDR = ''.join( ['../',os.path.basename(trainFile), \
      os.path.basename(testFile),','.join(tmpmodel), '_trueFDRResult'] )

  f = open(outputFileTrueFDR, 'w')

  ## now trueFDR have different lens for different models
  ## as we use the unique values
  lenTrueFDR = [ len(a) for a in trueFDR ]

  for j in range( max(lenTrueFDR) ):
    for i in range(len(trueFDR)):
      if i == len(trueFDR) - 1:
        try:
          f.write(repr(trueFDR[i][j])+'\n')
        except IndexError:
          f.write('NA'+'\n')
      else:
        try:
          f.write(repr(trueFDR[i][j])+' ')
        except IndexError:
          f.write('NA'+' ')

  f.close()

  # write AUC
  outputFileAUC = ''.join( ['../',os.path.basename(trainFile), \
      os.path.basename(testFile),','.join(tmpmodel), '_AUC'] )
  f = open(outputFileAUC, 'w')
  f.write(' '.join(model))
  f.write('\n')
  f.write(' '.join([ repr(a) for a in testAUC ]))
  f.write('\n')
  f.write(' '.join([ repr(a) for a in estFDRAUC ]))
  f.write('\n')
  f.write(' '.join([ repr(a) for a in trueFDRAUC ]))
  f.write('\n')
 
  f.close()
  
  # plot
  ## R script: plot 3 figures
  ## ROC, TPR~estFDR, TPR~trueFDR
  print '------------------------------------'
  print "- ploting "
  cmd = 'Rscript plot_v2.R %s %s %s %s %s' % (outputFileROC, outputFileEstFDR, \
      outputFileTrueFDR, ','.join(model), repr(top))
  print '- cmd is %s' % cmd
  subprocess.call(shlex.split(cmd))

  return 0

if __name__ == '__main__':
  main(sys.argv)
