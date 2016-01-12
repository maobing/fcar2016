/*
 * 2015-02-13 BH
 * This script directly use liblinear class interface
 * to provide genome-wide scan prediction for ChIPseq
 */
// compile
// g++ -Wall -o predictModelWholeGenome predictModelWholeGenome.c ./liblinear-1.96/tron.o ./liblinear-1.96/linear.o ./fcarLib.o ./liblinear-1.96/blas/blas.a -L/home/bst/student/bhe2/hji/samtools-0.1.18 -lbam -lz

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "fcarLib.h"
#include "./liblinear-1.96/linear.h"
#include "./liblinear-1.96/tron.h"

// extern
extern const uint32_t chrlen[];

int menu_predictModelWholeGenome(int argc, char **argv);
int predictModelWholeGenome(char *method, char *trainingFile, char *testingFile, char *trainedModel, char *outputFile, char *paramFile);

/*------------------------*/
/*  predictModel          */
/*------------------------*/
int main(int argc, char **argv) {
	/* menu */
	menu_predictModelWholeGenome(argc, argv);

	/* exit */
	exit(EXIT_SUCCESS);
	
}


int menu_predictModelWholeGenome(int argc, char **argv) {

	/* ------------------------------- */
	/*        predictModel             */
	/* -m method name                  */
  /* -tm trainedModel                */
  /* -train trainingFile             */
  /* do not need testing file, as    */
  /* it is assumed whole genome      */
  /* -coverage coverageFileList      */
	/* -o output results               */
	/* ------------------------------- */

	if (argc == 1) {
		printf("/*------------------------------------*/\n");
		printf("/*    menu_predictModelGenomeWide     */\n");
		printf("/* -m method name                     */\n");
		printf("/*    LogisticRegressionL1            */\n");
		printf("/*    LogisticRegressionL2            */\n");
		printf("/*    SVM                             */\n");
		printf("/*    RandomFores                     */\n");
    printf("/* -tm trainedModel                   */\n");
    printf("/* -train trainFile                   */\n");
    printf("/* -coverage coverageFileList         */\n");
		printf("/* -o output results                  */\n");
    printf("/* -p param                           */\n");
		printf("/*------------------------------------*/\n");
		exit(EXIT_SUCCESS);
	}

  char *method = (char *)calloc(MAX_DIR_LEN, sizeof(char));
	char *trainedModel = (char *)calloc(MAX_DIR_LEN, sizeof(char));
	char *trainFile = (char *)calloc(MAX_DIR_LEN, sizeof(char)); 
  char *coverageFileList = (char *)calloc(MAX_DIR_LEN, sizeof(char));
	char *outputFile = (char *)calloc(MAX_DIR_LEN, sizeof(char));
  char *paramFile = (char *)calloc(MAX_DIR_LEN, sizeof(char));

	int ni;
	int mOK = 0, tmOK = 0, trainOK = 0, coverageOK = 0, oOK = 0, pOK = 0;

	ni = 1;
	while (ni < argc) {
		if (strcmp(argv[ni], "-m") == 0) {
			ni++;
			strcpy(method, argv[ni]);
			mOK = 1;
		}
		else if (strcmp(argv[ni], "-tm") == 0){
			ni++;
			strcpy(trainedModel, argv[ni]);
			tmOK = 1;
		}
		else if (strcmp(argv[ni], "-train") == 0){
			ni++;
			strcpy(trainFile, argv[ni]);
			trainOK = 1;
		}
		else if (strcmp(argv[ni], "-coverage") == 0){
			ni++;
			strcpy(coverageFileList, argv[ni]);
			coverageOK = 1;
		}
		else if (strcmp(argv[ni], "-o") == 0){
			ni++;
			strcpy(outputFile, argv[ni]);
			oOK = 1;
		}
		else if (strcmp(argv[ni], "-p") == 0){
			ni++;
			strcpy(paramFile, argv[ni]);
      pOK = 1;
    }
		else {
			printf("Error: unkown parameters!\n");
			exit(EXIT_FAILURE);
		}
		ni++;
	}

	/* check args */
	if ((mOK + tmOK + trainOK + coverageOK + oOK + pOK) < 6){
		printf("Error: input arguments not correct!\n");
		exit(EXIT_FAILURE);
	}
	else {
    if(predictModelWholeGenome(method, trainedModel, trainFile, coverageFileList, outputFile, paramFile) != 0) {
      printf("Error: predictModel failed\n");
      return 1;
    }
	}

	/* free pointers */
  free(method);
	free(trainedModel);
	free(trainFile);
  free(coverageFileList);
	free(outputFile);
  free(paramFile);

	return 0;
}

/* predictModel */
int predictModelWholeGenome(char *method, char *trainedModel, char *trainFile, char *coverageFileList, char *outputFile, char *paramFile) {

  // utility var
  int i,j,k;
  
  // trainedModel
  struct model *mymodel;
  if( (mymodel = load_model(trainedModel)) == 0) {
    printf("cannot load model from file %s\n", trainedModel);
    return -1;
  }

  /* // trainFile
  FILE *trainFileFp = NULL;
  if( (trainFileFp = fopen(trainedModel, "r")) == NULL ) {
    printf("cannot open trainedModel file %s\n", trainedModel);
    return -1;
  } */

  // coverageFileList
  int totalCoverageFiles;
  FILE *coverageFileListFp = NULL;
  if( (coverageFileListFp = fopen(coverageFileList, "r") ) == NULL) {
    printf("Cannot open file %s\n", coverageFileList);
    return -1;
  }
  char **coverageFiles = (char **)calloc(MAX_BAM_FILES,sizeof(char *));
  for(i = 0; i < MAX_BAM_FILES; i++) {
    coverageFiles[i] = (char *)calloc(MAX_DIR_LEN, sizeof(char));
  }
  
  i = 0;
  while (!feof(coverageFileListFp)) {
    if (i >= MAX_BAM_FILES) {
      printf("Error: the number of input coverages files exceeds the limit %d\n", i);
      return -1;
    }
    if( ( fscanf(coverageFileListFp, "%s\n", coverageFiles[i]) ) != 1) {
      printf("Error: reading %dth from %s\n", i, coverageFileList);
      return -1;
    }
    i++;
  }
  totalCoverageFiles = i;
  fclose(coverageFileListFp);

  // open coverage Files
  FILE *coverageFps[totalCoverageFiles];
  for(i = 0; i < totalCoverageFiles; i++) {
    if( (coverageFps[i] = fopen(coverageFiles[i], "rb")) == NULL ) {
      printf("Error opening coverage file %s\n", coverageFiles[i]);
      return -1;
    }
  }

  // paramFile
  struct extractFeatureParam *param = (struct extractFeatureParam *)calloc(1, sizeof(struct extractFeatureParam));
  parseParam(paramFile, param);

  // outputFile: use binary file ~ 12G
  FILE *outputFileFp = NULL;
  if( (outputFileFp = fopen(outputFile, "wb")) == NULL ) {
    printf("Error: cannot write to outputFile %s\n", outputFile);
    return -1;
  }

  // predict model: by default: predict probability
  int nr_class = get_nr_class(mymodel);
  double *prob_estimates = (double *)calloc(nr_class, sizeof(double));

  // predResult for storing results
  int totalBins = 0;
  int cumBins[NUM_SEQ];
  for (i = 0; i < NUM_SEQ; i++) {
    totalBins += (int)(chrlen[i] / param->resolution) + 1;
    cumBins[i] = totalBins;
  }

  float **predResult = (float **)calloc(NUM_SEQ, sizeof(float *)); 
  for(i = 0; i < NUM_SEQ; i++) {
    predResult[i] = (float *)calloc( (int)(chrlen[i] / param->resolution) + 1, sizeof(float));
  }

  // read in feature for each bin and do prediction
  // for(i = 0; i < NUM_SEQ; i++) {
  for(i = 20; i < 21; i++) {
    for(j = 0; j < (int)(chrlen[i] / param->resolution) + 1; j++) {
      if(j % 100000 == 0) {
        printf("Predicting chr%d:%dth bin\n", i,j);
      }
      int max_nr_feature = 100;
      struct feature_node *myX = (struct feature_node *)calloc(max_nr_feature, sizeof(struct feature_node));
      int idx = 0;
      for(k = 0; k < totalCoverageFiles; k++) {
        float *buffer = (float *)calloc( param->windowSize/param->resolution,sizeof(float));
        int offset = j;
        offset += -(int)((float)(param->windowSize / 2) / (float)param->resolution + 0.5);
        if(offset < 0 || offset + (int)((float)(param->windowSize) / (float)param->resolution + 0.5) > (int)(chrlen[i] / param->resolution) + 1) {
          // printf("offset is %d\n", offset);
          free(buffer);
          continue;
        }
        if(i != 0) offset += cumBins[i-1];
        // printf("offset is %d\n", offset);
        fseek(coverageFps[k], offset*sizeof(float), SEEK_SET);
        fread(buffer, sizeof(float), param->windowSize/param->resolution, coverageFps[k]);
        int l;
        // printf("buffer[%d] is:",l);
        for(l = 0; l < param->windowSize/param->resolution; l++) {
          // if(j == 289540) printf("%f,",buffer[l]);
          if(buffer[l] != 0) {
            myX[idx].index = k*(param->windowSize/param->resolution) + l + 1;
            myX[idx].value = buffer[l];
            idx++;
          }
          if(idx >= max_nr_feature -2) { // feature_node is not long enough
            max_nr_feature *= 2;
            myX = (struct feature_node *)realloc(myX, max_nr_feature*sizeof(struct feature_node));
          }
        }
        free(buffer);
      } // end of loop through coverageFiles
      // printf("\n");
      myX[idx].index = -1; // a flag for end of features
      if(idx == 0) {
        // printf("idx is %d\n",idx);
        predResult[i][j] = 0.0;
        free(myX);
        continue;
      }
      // printf("nr_feature is %d\n", idx);
      predict_probability(mymodel, myX, prob_estimates);
      // printf("num of feature is %d\n", get_nr_feature(mymodel));
      // printf("num of class is %d\n", get_nr_class(mymodel));
      predResult[i][j] = prob_estimates[0];
      free(myX);
    }
  }

  // print output
  printf("writing to output %s\n", outputFile);
  for(i = 0; i < NUM_SEQ; i++) {
    fwrite(predResult[i], sizeof(float),(int)(chrlen[i] / param->resolution)+1 , outputFileFp);
  }

  // close file
  fclose(outputFileFp);
  for(i = 0; i < totalCoverageFiles; i++) {
    fclose(coverageFps[i]);
  }
  // free pointers
  for(i = 0; i < MAX_BAM_FILES; i++) {
    free(coverageFiles[i]);
  }
  free(coverageFiles);
  free(param);
  free(prob_estimates);
  for(i = 0; i < NUM_SEQ; i++) {
    free(predResult[i]);
  }
  free(predResult);

  return 0;
}

