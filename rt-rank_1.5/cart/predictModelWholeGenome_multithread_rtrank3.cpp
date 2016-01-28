// main.cpp
// BH modified
// * use main thread to create all trees
// * pass all trees to subthread with different testing data
// * get final results

#include "main.h"

#define REG

int main(int argc, char* argv[]) {
  int i;
  srand(time(NULL));

	// get command line args
  // BH: modification, use test_files[0] as coverageFileList,
  //      test_files[1] as fcar param file!
  // BH: modification, use test_outs as outputbinary file
	args_t myargs;
	init_args(myargs);
	if (!get_args(argc, argv, myargs)) {
			printf("wrong rtrank cmd args, check ./rtrank/\n");
			return 0;
	}
  if (myargs.test_files[0].size() != 2) {
    cout << "wrong test files: 1st should be coverageFileList,"
      << "\n 2nd should be fcar param file\n" << endl;
  }
  if (myargs.processors != 23) {
    cout << "currently we only deals with 23 processors case \n" << endl;
  }

	// load train data
  data_t train;

	/**** start of load train ****/
	int numfeatures = myargs.features;
	int missing = myargs.missing;
	char* missing_file = myargs.missing_file;

	int m = myargs.ones ? -1 : myargs.missing;

	printf("loading training data...");
	if (!tuple::read_input(train, myargs.train_file, numfeatures, 1, m, missing_file))
			return 0;
	printf("done\n");
	/**** end of load train ****/

  // presort the training data for each feature
	add_idx(train);
	myargs.ntra = train.size();
	if (myargs.processors == 1) presort(train, myargs);
	else presort_p(train, myargs);
	
	// BH: we only deals with random forest
	if (myargs.alg != ALG_FOREST) {
		printf("we only deals with random forest\n");
		return 0;
	}

	// random forest: begin
	int N = train.size();
	int numtrees = args.trees;
  int numthreads = args.processors;
  int kfeatures = args.kfeatures;

  // initialize threads
  thread** threads = new thread*[numthreads];

  // initialize preds for storing results
  char* paramFile = myargs.test_files[1];
  struct extractFeatureParam *param = (struct extractFeatureParam *)calloc(1, sizeof(struct extractFeatureParam));
  parseParam(paramFile, param);
  
  float **predResult = (float **)calloc(numthreads, sizeof(float *));
  for (int i = 0; i < numthreads; i++){
    predResult[i] = (float *)calloc(param->windowSize / param->resolution, sizeof(float));
  }

  // build all trees, directly use classify
  vector<dt_node*> trees(numtrees, 0);
  for (int t = 0; t < numtrees; t++) {
    data_t sample;
    randsample(train, sample);
    trees.push(new dt_node(sample, args, args.depth, 1, kfeatures, false, args));
    if(t % 50 == 0) printf("creating %dth tree\n", t);
  }

  // start threading
	for (int i = 0; i < numthreads; i++) {
    threads[i] = new thread(bind(classify_BH, coverageFileList, thread, cref(trees),
      ref(preds[i]));
	}
  // finish threading
	for (int i = 0; i < numthreads; i++) {
		threads[i]->join();
		delete threads[i];
	}
	fprintf(stderr, "done threading!\n");
  
  // delete threads
  delete[] threads;

  // need to write output here

  // free memory and exit
  tuple::delete_data(train);
  // do not forget to delete predResult too!

  return 0;
}

void classify_BH(thread, trees, preds, myargs) {
  // coverageFilelist: open coverageFile inside each thread
  // thread: use thread to decide which data setgmentation to use as test
  // trees: all the built trees
  // preds: allocated memory in main thread for storing preds for test
  // myargs
  
  // open all coverage files
  char *coverageFileList = myargs.test_files[0];
  char *paramFile = myargs.test_files[1];

  ifstream coverageFile(coverageFileList);
  vector<ifsteam> coverageFileFp;
  string tmpFile;
  if (coverageFile.is_open()) {
    while (getline(coverageFile, tempFile)) {
      coverageFileFp.push(ifsrteam(tempFile, ios::binary));
      if (!coverageFileFp.back().is_open()) {
        cout << "cannot open " << tempFile << endl;
        return 0;
      }
    };
  }
  coverageFile.close();

  // open and read in paramFile fcar
  struct extractFeatureParam *param = (struct extractFeatureParam *)calloc(1, sizeof(struct extractFeatureParam));
  parseParam(paramFile, param);

  // calculate bins for seeking positions
  int totalBins = 0;
  int cumBins[NUM_SEQ];
  for (i = 0; i < NUM_SEQ; i++) {
    totalBins += (int)(chrlen[i] / param->resolution) + 1;
    cumBins[i] = totalBins;
  }

  // make predictions
  int numfeatures = myargs.features;
  int missing = myargs.missing;
  char* missing_file = myargs.missing_file;
  int m = myargs.ones ? -1 : myargs.missing;
  
  for (int j = 0; j < (int)(chrlen[chr] / param->resolution) + 1; j++) {
    
    // initialize one test case tuple*
    double *init_values = (double *)calloc(numfeatures, sizeof(double));
    tuple* test = new tuple(numfeatures, m, init_values);
    free(init_values);

    // calculate seek position
    int offset = j;
    offset += -(int)((float)(param->windowSize / 2) / (float)param->resolution + 0.5);
    if (offset < 0 || offset + (int)((float)(param->windowSize) / (float)param->resolution + 0.5) >(int)(chrlen[i] / param->resolution) + 1)
      continue;
    if (chr != 0) offset += cumBins[chr - 1];

    // loop through coverage files to fill in double* tuple->features
    for (int k = 0; k < coverageFileFp.size(); k++) {
      coverageFileFp[k].fseekg(offset*sizeof(float));
      coverageFileFp[k].read(&test->feature[numfeatures / coverageFileFp.size() * k], sizeof(float), param->windowSize / param->resolution);
    }

    for (int i = 0; i < trees.size(); i++) {  
      // make predictions for tree i for bin j
      preds[j] += args.alpha * dt_node::classify(test, tree[i]);
    }
  }
  return;
}
