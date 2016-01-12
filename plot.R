#!/bin/Rscript
args <- commandArgs(TRUE)

print(args)

# args = c('','')
# args[1] = "../trainCMYCpredictCTCFLogisticRegressionL1,RandomForest,voting,Benchmark_result"
# args[2] =  "RandomForest,Benchmark,LogisticRegressionL1_0.005,LogisticRegressionL1_0.01,LogisticRegressionL1_0.05,LogisticRegressionL1_0.1,LogisticRegressionL1_0.5,LogisticRegressionL1_1,voting"

outputFile = args[1]
outputFileControl = args[2]
models = strsplit(x = args[3],split=",")[[1]]
weights = as.numeric(strsplit(x = args[4],split=",")[[1]])
top = as.numeric(args[5])

testResult = read.table(outputFile, stringsAsFactor = FALSE)
for(i in 2:ncol(testResult)) {
	testResult[,i] = as.numeric(unlist(strsplit(x = testResult[,i], split = ":"))[(1:(nrow(testResult)*2)) %% 2 == 0])
}

controlResult =  read.table(outputFileControl, stringsAsFactor = FALSE)
for(i in 2:ncol(controlResult)) {
  controlResult[,i] = as.numeric(unlist(strsplit(x = controlResult[,i], split = ":"))[(1:(nrow(controlResult)*2)) %% 2 == 0])
}

# roc function
roc <- function(prediction, truth) {
  prediction = as.numeric(prediction)
  truth = as.numeric(truth)
	knots = unique(prediction)
  if(length(knots) <= 1) {
    cutoff = seq(0,1,by=0.1)
  } else {
    cutoff = sort(unique(prediction))
  }
	TPR = vector("numeric", length(cutoff))
	FPR = vector("numeric", length(cutoff))
	for(i in 1:length(cutoff)) {
		TPR[i] <- sum(truth[prediction >= cutoff[i]])/sum(truth)
		FPR[i] <- sum(truth[prediction >= cutoff[i]] == 0)/sum(truth == 0)
	}
	roc = cbind(TPR, FPR)
	names(roc) = c("TPR","FPR")
	return(roc)
}

# auc function: calculate area under curve
auc <- function(TPR, FPR) {
  area = 0
  for(i in 2:length(TPR)) {
    area = area + 1/2*(TPR[i-1]+TPR[i])*(FPR[i]-FPR[i-1])*(-1)
  }
  return(area)
}


# library(RColorBrewer)
# cols = brewer.pal(ncol(testResult)-1,"Set1")
aucs = numeric(length(models))
cols = rainbow(ncol(testResult) - 1)
png(file = paste0(outputFile, ".png"), type = "cairo", bg = "transparent",
	width = 480*1.2*3, height = 480*1.2)
par(mfrow = c(1,3))
## plot 1 ROC
plot(NA, xlim=c(0,1),ylim=c(0,1), xlab = "FPR", ylab = "TPR", 
	main = paste0(substr(args[1],1,23),"\n",substr(args[1],24,nchar(args[1]))))

for(i in 2:ncol(testResult)) {
  # i = 3; mylty = 1
	if(models[i-1] == "voting" || models[i-1] == "Benchmark" || models[i-1] == "RandomForest") {
		mylty = 1
	}
	else {
		mylty = 2
	}
	tmp = roc(prediction = testResult[,i], truth = testResult[,1])
  aucs[i-1] = auc(TPR = tmp[,1], FPR = tmp[,2])
  cat(paste0("Model ", models[i-1], "'s auc is ", aucs[i-1],"\n"))
	lines(tmp[,1]~tmp[,2], col = cols[i-1], lty = mylty)
}

legend("bottomright", legend=c(models[-1], paste0(models[length(models)],'Top',top)), col = cols, lty=1, lwd=2)


## plot 2 TPR v.s. estFDR
aucsEstFDR = numeric(length(models))
plot(NA, xlim=c(0,1),ylim=c(0,1), xlab = "est FDR", ylab = "TPR",
  main = paste0(substr(args[1],1,23),"\n",substr(args[1],24,nchar(args[1]))))

for(i in 2:ncol(controlResult)) {
  controlEcdf = ecdf(controlResult[,i])
  pvalue = 1 - controlEcdf(testResult[,i])
  FDR = p.adjust(pvalue, "fdr")
  sens = 

  if(models[i-1] == "voting" || models[i-1] == "Benchmark" || models[i-1] == "RandomForest") {
    mylty = 1
  }
  else {
    mylty = 2
  }
  
  cat(paste0("Model ", models[i-1], "'s aucsEstFDR is ", aucsEstFDR[i-1],"\n"))
  sens = cumsum(testResult[order(FDR,decreasing=F),1] == 1)/sum(testResult[,1] == 2)
  lines(sens ~ FDR, col = cols[i-1], lty = mylty)
  aucsEstFDR[i-1] = auc(TPR = sens, FPR = FDR)

}

legend("bottomright", legend=c(models[-1], paste0(models[length(models)],'Top',top)), col = cols, lty=1, lwd=2)


## plot 3 TPR v.s. trueFDR
## process FDR fun
processTrueFDR <- function(trueFDR) {
  for(i in (length(trueFDR)-1):1) {
    if(trueFDR[i] > trueFDR[i+1]) {
      trueFDR[i] = trueFDR[i+1]
    }
  }
  return(trueFDR)
}


aucsTrueFDR = numeric(length(models))
plot(NA, xlim=c(0,1),ylim=c(0,1), xlab = "true FDR", ylab = "TPR",
  main = paste0(substr(args[1],1,23),"\n",substr(args[1],24,nchar(args[1]))))

for(i in 2:ncol(controlResult)) {
  tFDR =  cumsum(testResult[order(testResult[,i],decreasing=T),1] == 0)/(1:nrow(testResult))
  tFDR = processTrueFDR(tFDR)  
  sens = cumsum(testResult[order(testResult[,i],decreasing=T),1] == 1)/sum(testResult[,i] == 1)

  if(models[i-1] == "voting" || models[i-1] == "Benchmark" || models[i-1] == "RandomForest") {
    mylty = 1
  }
  else {
    mylty = 2
  }

  cat(paste0("Model ", models[i-1], "'s aucsTrueFDR is ", aucsTrueFDR[i-1],"\n"))
  lines(sens ~ tFDR, col = cols[i-1], lty = mylty)
  aucsTrueFDR[i-1] = auc(TPR = sens, FPR = tFDR)
 
}

legend("bottomright", legend=c(models[-1], paste0(models[length(models)],'Top',top)), col = cols, lty=1, lwd=2)

## SINK aucs

sink(file = paste0(args[1],"_auc"), append = TRUE)
cat(paste0(args[1],"\n"))
cat(paste0(models,collapse = " "))
cat("\n")
cat(paste0(paste0(aucs,collapse = " "),"\n"))
cat(paste0(paste0(aucsEstFDR,collapse = " "),"\n"))
cat(paste0(paste0(aucsTrueFDR,collapse = " "),"\n"))
sink(NULL)
