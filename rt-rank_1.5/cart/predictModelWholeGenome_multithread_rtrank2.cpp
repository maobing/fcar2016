#include "main.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include "../../fcarLib.h"

// define the size of my pool of threads, 23, one per chr
#define NUM_THREADS 23

// user extern chrlen in fcar.h
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

int main(int argc, char* argv[]) {
  // set seed
  srand(time(NULL));

  // get args etc. based on cart/main.cpp
  args_t myargs;
  init_args(myargs);
  if (!get_args(argc, argv, myargs)) {
    printf("RT-Rank Version 1.5 (alpha) usage: [-options] train.txt test.txt output.txt [test.txt output.txt]*\n");
    printf("\nRequired flags:\n");
    printf("-f int\tNumber of features in the data sets. \n");
    printf("You must use one of the following:\n");
    printf("\nOptional flags:\n");
    printf("-B \tgradient boosting mode (standalone).\n");
    printf("-F \trun random forest (standalone).\n");
    printf("-a float\t stepsize.\n");
    printf("-d int \tmax treep depth (for gradient boosting trees are typically limited to a small depth, e.g. d=4).\n");
    printf("-p int\tnumber of processors/threads to use.\n");
    printf("-k \tnumber of randomly selected features used for building each trees of a random forest.\n");
    printf("-t int \tnumber of trees for random forest.\n");
    printf("-m \tuse mode for prediction at leaf nodes (default is mean)\n");
    printf("-z \tsubstitute missing features with zeros (recommended if missing features exist).\n");
    printf("-e \tuse entropy to measure impurity for CART (default is squared loss).\n");
    printf("\nOperation in wrapper mode (e.g. wih Python scripts):\n");
    printf("-w \tread in weights.\n");
    printf("-r \tnumber of trees (/iterations).\n");
    printf("-s \tprint the set of features used to built the tree to stdout.\n");
    printf("\n\n");
    return 0;
  }

  // Here we only deal with random forest, so
  if(myargs.alg != ALG_FOREST) {
    printf("Error: we only deal with Random Forest algorithm\n");
    return 0;
  }

  // need to load data to train, but not to test
  // so may need to change the load_data function
  data_t train;
  // vec_data_t test;
  // load_data(train, test, myargs))

  // copy from cart/main.cpp
  // presort the training data for each feature
  add_idx(train);
  myargs.ntra=train.size();
  if (myargs.processors==1)
    presort(train, myargs);
  else
    presort_p(train, myargs);


  // need to follow fcar/multithread.c to setup thread data
  int kfeatures = args.kfeatures;
  int num_test = args.num_test;
  
  // need to allocate whole genome memory for saving predictions
  
  for(int i = 0; i <= numOfTrees; i++) {
    data_t sample;
    randsample(train, sample);
    dt_node* tree = new dt_node(sample, args, args.depth, 1, kfeatures, false, args);
    for(j = 1; j < numOfSeq; j++) {
      // initialize threads
      // a wrapper including loop of dt_node::classify(test)
      // dt_node::classify_all(testcase1, tree, test_preds1, args);
      // remember to using streaming average to save test_preds1 to preallocated
      // predictions.
    } 
    delete tree;
}

// write to output to binary output
