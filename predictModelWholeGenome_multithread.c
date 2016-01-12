/*
 * 2015-02-13 BH
 * This script directly use liblinear class interface
 * to provide genome-wide scan prediction for ChIPseq
 * with multithread
 */
// compile
// g++ -Wall -o predictModelWholeGenome_multithread predictModelWholeGenome_multithread.c ./liblinear-1.96/tron.o ./liblinear-1.96/linear.o ./fcarLib.o ./liblinear-1.96/blas/blas.a -L/home/bst/student/bhe2/hji/samtools-0.1.18 -lbam -lz -lpthread

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include "fcarLib.h"
#include "./liblinear-1.96/linear.h"
#include "./liblinear-1.96/tron.h"

// define the size of my pool of threads, 23, one per chr
#define NUM_THREADS 23

// use extern in fcar.h
// Note: this can be shared among all threads
extern const uint32_t chrlen[];

// create thread argument struct for thr_func()
// which contains args to be passed to thr_func()
typedef struct _thread_data_t {
  char *method;
  char *trainedModel;
  char *trainFile;
  char *coverageFileList;
  char *paramFile;
  int chr; // specifying which chr for which thread
} thread_data_t;

// thread function 
void *predictModelWholeGenome( void *arg );

// other function declaration
int menu_predictModelWholeGenome(int argc, char **argv);

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
		return(EXIT_SUCCESS);
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
			return(EXIT_FAILURE);
		}
		ni++;
	}

	/* check args */
	if ((mOK + tmOK + trainOK + coverageOK + oOK + pOK) < 6){
		printf("Error: input arguments not correct!\n");
		exit(EXIT_FAILURE);
	}

  // init thr
  pthread_t thr[NUM_THREADS];
  // thread_data_t to be passed to each thread
  thread_data_t thr_data[NUM_THREADS];
  
  /* creat thread */
  int i, rc; // rc for holding error code
  for(i = 0; i < NUM_THREADS; i++) {
    thr_data[i].chr = i;
    thr_data[i].method = method;
    thr_data[i].trainedModel = trainedModel;
    thr_data[i].trainFile = trainFile;
    thr_data[i].coverageFileList = coverageFileList;
    thr_data[i].paramFile = paramFile;
    printf("creating %dth thread\n", i);
    fflush(stdout); // flush stdout
    if( (rc = pthread_create(&thr[i], NULL, predictModelWholeGenome, &thr_data[i])) ) {
      printf("error: pthread_create, rc:%d\n", rc);
      return EXIT_SUCCESS;
    }
  }

  /* block until all threads complete */
  /* then write to output in sequential order */
  float **predResult = (float **)calloc(NUM_THREADS, sizeof(float *));
  for(i = 0; i < NUM_THREADS; i++) {
    pthread_join(thr[i], (void **) &predResult[i]);
  }

  /* write to output */
  struct extractFeatureParam *param = (struct extractFeatureParam *)calloc(1,sizeof(struct extractFeatureParam));
  parseParam(paramFile,param);

  FILE *outputFileFp = NULL;
  if( (outputFileFp = fopen(outputFile, "wb")) == NULL ) {
    printf("Error: cannot write to outputFile %s\n", outputFile);
    return EXIT_SUCCESS;
  }
  printf("writing to output %s\n", outputFile);
  for(i = 0; i < NUM_THREADS; i++) {
    fwrite(predResult[i], sizeof(float),(int)(chrlen[i] / param->resolution)+1 , outputFileFp);
    free(predResult[i]);
  }
  fclose(outputFileFp);
  
	/* free pointers */
  free(method);
	free(trainedModel);
	free(trainFile);
  free(coverageFileList);
	free(outputFile);
  free(paramFile);
  free(param);
  free(predResult);

	return 0;
}

/* thr_func: predictModel */
// NOTE that pthread_create(pthread_t *thread, pthread_att_t *attr, 
//    void * (*start_routine) (void * ), viod *arg)
//    requires a pointer to a function which takes in and returns void pointers

void *predictModelWholeGenome(void *arg) {
  thread_data_t *data = (thread_data_t *) arg;

  printf("data->trainedModel is %s\n", data->trainedModel);
  printf("data->coverageFileList is %s\n", data->coverageFileList);
  printf("data->trainFile %s\n", data->trainFile);
  printf("data->paramFile %s\n", data->paramFile);
  printf("data->chr is %d\n", data->chr);

  char *trainedModel = data->trainedModel;
  char *coverageFileList = data->coverageFileList;
  // char *trainFile = data->trainFile;
  char *paramFile = data->paramFile;
  int chr = data->chr;

  // utility var
  int i,j,k;
  
  // trainedModel
  struct model *mymodel;
  if( (mymodel = load_model(trainedModel)) == 0) {
    printf("cannot load model from file %s\n", trainedModel);
    return EXIT_SUCCESS;
  }

  // coverageFileList
  int totalCoverageFiles;
  FILE *coverageFileListFp = NULL;
  if( (coverageFileListFp = fopen(coverageFileList, "r") ) == NULL) {
    printf("Cannot open file %s\n", coverageFileList);
    return EXIT_SUCCESS;
  }
  char **coverageFiles = (char **)calloc(MAX_BAM_FILES,sizeof(char *));
  for(i = 0; i < MAX_BAM_FILES; i++) {
    coverageFiles[i] = (char *)calloc(MAX_DIR_LEN, sizeof(char));
  }
  
  i = 0;
  while (!feof(coverageFileListFp)) {
    if (i >= MAX_BAM_FILES) {
      printf("Error: the number of input coverages files exceeds the limit %d\n", i);
      return EXIT_SUCCESS;
    }
    if( ( fscanf(coverageFileListFp, "%s\n", coverageFiles[i]) ) != 1) {
      printf("Error: reading %dth from %s\n", i, coverageFileList);
      return EXIT_SUCCESS;
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
      return EXIT_SUCCESS;
    }
  }

  // paramFile
  struct extractFeatureParam *param = (struct extractFeatureParam *)calloc(1, sizeof(struct extractFeatureParam));
  parseParam(paramFile, param);

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

  // allocate memory for result based on thread data chr
  // as we are using one thread for each chr
  float *predResult = (float *)calloc( (int)(chrlen[chr] / param->resolution) + 1, sizeof(float));

  // read in feature for each bin and do prediction
  for(j = 0; j < (int)(chrlen[chr] / param->resolution) + 1; j++) {
    if(j % 100000 == 0) {
      printf("Predicting chr%d:%dth bin\n", chr,j);
      fflush(stdout);
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
      if(chr != 0) offset += cumBins[chr-1];
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
      predResult[j] = 0.0;
      free(myX);
      continue;
    }
    // printf("nr_feature is %d\n", idx);
    predict_probability(mymodel, myX, prob_estimates);
    // printf("num of feature is %d\n", get_nr_feature(mymodel));
    // printf("num of class is %d\n", get_nr_class(mymodel));
    int *mylabel = (int *)calloc(10, sizeof(int));
    get_labels(mymodel, mylabel);
    if(mylabel[0] == 1) {
      predResult[j] = prob_estimates[0];
    } else {
      predResult[j] = prob_estimates[1];
    }
 
    free(myX);
    free(mylabel);
  }


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
  // give address of pointer to this function, so that the function can free the pointer.
  free_and_destroy_model(&mymodel); 
  pthread_exit((void *) predResult);
}

