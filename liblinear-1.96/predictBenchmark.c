/*Compile: gcc predictBenchmark.c -o predictBenchmark */

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"

int readData(FILE *fp, double **X, int *y, int n, int p);

int main (int argc, char **argv) {
  int i, j;
  
  if (argc != 6) {
	  printf("/*---------------------------------------------------*/\n");
      printf("/* usage: benchmark testFile n p trainedModel result */\n");
	  printf("/* testFile: original test   file                    */\n");
	  printf("/* n: num of observations,                           */\n");
	  printf("/* p: num of features                                */\n");
	  printf("/* trainedModel:                                     */\n");
	  printf("/* result: output                                    */\n");
      printf("/*---------------------------------------------------*/\n");
	  return EXIT_SUCCESS;
  }

  char *testFile = argv[1];
  char *trainedModelFile = argv[4];
  int n = atoi(argv[2]);
  int p = atoi(argv[3]);
  char *outputFile = argv[5];
  
  char *testFileBenchmark = (char *)calloc(255, sizeof(char));
  strcpy(testFileBenchmark, testFile);
  strcat(testFileBenchmark, "_benchmark");
  
  char *cmd = (char *)calloc(255, sizeof(char));
  strcpy(cmd, "./liblinear-1.96/predict -b 1 ");
  strcat(cmd, testFileBenchmark);
  strcat(cmd, " ");
  strcat(cmd, trainedModelFile);
  strcat(cmd, " ");
  strcat(cmd, outputFile);
  printf(cmd);
  
  // if benchmark has already been calculated
  //	directly train model
  if( access(testFileBenchmark, F_OK ) != -1 ) {
	system(cmd);
	free(testFileBenchmark);
	free(cmd);
	return 0;
  }
  
  double **X = (double **)calloc(n,sizeof(double *));
  for (i = 0; i < n; i++) {
	X[i] = (double *)calloc(p,sizeof(double));
  }
  
  int *y = (int *)calloc(n,sizeof(int));

  // read in data
  FILE *testFileFp = NULL;
  if((testFileFp = fopen(testFile, "r")) == NULL) {
	printf("Cannot open data file %s\n", testFile);
	return EXIT_SUCCESS;
  }

  if(readData(testFileFp, X, y, n, p) < 0) {
	printf("Error reading data %s\n", testFile);
	return EXIT_SUCCESS;
  }
  fclose(testFileFp);

  double **meanCnt = (double **)calloc(n, sizeof(double *));
  for(i = 0; i < n; i++){
	meanCnt[i] = (double *)calloc(5, sizeof(double));
  }
  
  // get average log2 counts and save
  for(i = 0; i < n; i++) {
	for(j = 0; j < p; j++) {
		meanCnt[i][(int)(j/200)] += X[i][j];
	}
  }
  for(i = 0; i < n; i++) {
	for(j = 0; j < 5; j++) {
		meanCnt[i][j] = meanCnt[i][j]/200;
	}
  }
  
  // write benchmark to trainFileBenchmark
  FILE *testFileBenchmarkFp = NULL;
  if((testFileBenchmarkFp = fopen(testFileBenchmark, "w")) == NULL) {
	printf("Cannot open data file %s\n", testFileBenchmark);
	return EXIT_SUCCESS;
  }
  
  for(i = 0; i < n; i++) {
    fprintf(testFileBenchmarkFp, "%d ", y[i]);
	for(j = 0; j < 5; j++) {
	  fprintf(testFileBenchmarkFp, "%d:%lf", j+1, meanCnt[i][j]);
	  if(j == 4) {
		fprintf(testFileBenchmarkFp, "\n");
	  }
	  else {
	    fprintf(testFileBenchmarkFp, " ");
	  }
	}
  }
  fclose(testFileBenchmarkFp);
  
  // call liblinear logistic with c = 0 to fit model
  system(cmd);
  
  for (i = 0; i < n; i++) {
	free(X[i]);
  }
  free(X);
  free(y);
  free(testFileBenchmark);
  free(meanCnt);
  for(i = 0; i < 5; i++) {
	free(meanCnt[i]);
  }
  free(meanCnt);
  
  return 0;
}

/*----------*/
/* fun def  */
/*----------*/
int readData(FILE *fp, double **X, int *y, int n, int p) {
	int i, j;
	double xij, yi;
	
	for (i = 0; i < n; i++) {
		for(j = 1; j < p+1; j++) {
			if(j == 1) {
				if((fscanf (fp, "%lf 1:%lf ", &yi, &xij)) !=2 ) {
					printf("Error reading %d,%d from data\n", i,j);
					return -1;
				}
			} else if(j == p) {
				if((fscanf (fp, "%*d:%lf\n", &xij)) !=1) {
					printf("Error reading %d,%d from data\n", i,j);
					return -1;
				}
			} else {
				if((fscanf(fp, "%*d:%lf ", &xij)) != 1) {
					printf("Error reading %d,%d from data\n", i,j);
					return -1;					
				}
			}
			X[i][j] = xij;
		}
		y[i] = yi;
	}
	return 0;
}
