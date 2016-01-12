/*Compile: gcc trainBenchmark.c -o trainBenchmark */

#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"

int readData(FILE *fp, double **X, int *y, int n, int p);

int main (int argc, char **argv) {
  int i, j;
  
  if (argc != 5) {
	  printf("/*---------------------------------------------*/\n");
      printf("/* usage: benchmark trainFile n p trainedModel */\n");
	  printf("/* trainFile: original train file              */\n");
	  printf("/* n: num of observations,                     */\n");
	  printf("/* p: num of features                          */\n");
	  printf("/* trainedModel: output file                   */\n");
      printf("/*---------------------------------------------*/\n");
	  return EXIT_SUCCESS;
  }

  char *trainFile = argv[1];
  char *trainedModelFile = argv[4];
  int n = atoi(argv[2]);
  int p = atoi(argv[3]);
  
  char *trainFileBenchmark = (char *)calloc(255, sizeof(char));
  strcpy(trainFileBenchmark, trainFile);
  strcat(trainFileBenchmark, "_benchmark");
  
  char *cmd = (char *)calloc(255, sizeof(char));
  strcpy(cmd, "./liblinear-1.96/train -s 0 -c 1000 ");
  strcat(cmd, trainFileBenchmark);
  strcat(cmd, " ");
  strcat(cmd, trainedModelFile);
  printf("cmd is %s\n", cmd);
  
  // if benchmark has already been calculated
  //	directly train model
  if( access(trainFileBenchmark, F_OK ) != -1 ) {
    // C = 100 to put all weight on likelihood
	system(cmd);
	free(trainFileBenchmark);
	free(cmd);
	return 0;
  }
  
  double **X = (double **)calloc(n,sizeof(double *));
  for (i = 0; i < n; i++) {
	X[i] = (double *)calloc(p,sizeof(double));
  }
  
  int *y = (int *)calloc(n,sizeof(int));


  // read in data
  FILE *trainFileFp = NULL;
  if((trainFileFp = fopen(trainFile, "r")) == NULL) {
	printf("Cannot open data file %s\n", trainFile);
	return EXIT_SUCCESS;
  }

  if(readData(trainFileFp, X, y, n, p) < 0) {
	printf("Error reading data %s\n", trainFile);
	return EXIT_SUCCESS;
  }
  fclose(trainFileFp);
  
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
  FILE *trainFileBenchmarkFp = NULL;
  if((trainFileBenchmarkFp = fopen(trainFileBenchmark, "w")) == NULL) {
	printf("Cannot open data file %s\n", trainFileBenchmark);
	return EXIT_SUCCESS;
  }
  
  for(i = 0; i < n; i++) {
    fprintf(trainFileBenchmarkFp, "%d ", y[i]);
	for(j = 0; j < 5; j++) {
	  fprintf(trainFileBenchmarkFp, "%d:%lf", j+1, meanCnt[i][j]);
	  if(j == 4) {
		fprintf(trainFileBenchmarkFp, "\n");
	  }
	  else {
	    fprintf(trainFileBenchmarkFp, " ");
	  }
	}
  }
  fclose(trainFileBenchmarkFp);
  
  // call liblinear logistic with c = 0 to fit model
  system(cmd);
  
  for (i = 0; i < n; i++) {
	free(X[i]);
  }
  free(X);
  free(y);
  free(trainFileBenchmark);
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
