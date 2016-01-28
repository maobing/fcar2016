#include "stdlib.h"
#include "stdio.h"
#include "math.h"

// complie: gcc coverage2bw.c -o coverage2bw -lm
// after that run:
//  bedGraphToBigWig in.bedGraph chrom.sizes out.bw
int main(int argc, char **argv) {
 if(argc != 4) {
    printf("usage: coverage2bw coverageFile resolution outputBedGraph\n");
    return 0;
  }

  // hg19 genome size
  int chrlen[] = { 249250621, 243199373, 198022430, 191154276, 180915260, \
    171115067, 159138663, 146364022, 141213431, 135534747, 135006516, \
    133851895, 115169878, 107349540, 102531392, 90354753, 81195210, \
    78077248, 59128983, 63025520, 48129895, 51304566, 155270560 };

  char *coverageFile = argv[1];
  int resolution = atoi(argv[2]);
  char *outBedgraph = argv[3];

  FILE *coverageFp = NULL;
  if( (coverageFp = fopen(coverageFile, "rb")) == NULL) {
    printf("cannot open file %s\n", coverageFile);
    return 0;
  }

  FILE *outFp = NULL;
  if( (outFp = fopen(outBedgraph, "w")) == NULL) {
    printf("cannot open file %s\n", outBedgraph);
    return 0;
  }
  
  int i, j;
for(i = 0; i < 23; ++i) {
//  for(i = 0; i < 1; ++i) {
    printf("processing %d th chromosome\n", i);
    int binNum = chrlen[i]/resolution + 1;
    float *buffer = (float *)calloc( binNum , sizeof(float));
    fread(buffer, sizeof(float), binNum , coverageFp);
    for(j = 0; j < binNum - 1; j++) { // since the last bin might be incomplete
      int tmpCnt = (int)pow(2, buffer[j])-1;
      if(tmpCnt == 0) continue;
      if(j % 1000000 == 0) printf(" - processing %d th bin\n", j);
      if(i < 22) {
        fprintf(outFp, "chr%d %d %d %d\n", i+1, j*resolution,(j+1)*resolution, 
          tmpCnt);
      } else {
        fprintf(outFp, "chrX %d %d %d\n", j*resolution,(j+1)*resolution, tmpCnt);
      }
    }
  free(buffer);
  }

  close(coverageFp);
  close(outFp);
}
