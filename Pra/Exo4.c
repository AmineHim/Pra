#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <sys/time.h>

#define MAX_NUM_OBJ 100

int num_obj = 0;
int capacity;
int weight[MAX_NUM_OBJ];
int utility[MAX_NUM_OBJ];
int NB_THREADS = 8;


void read_problem(char *filename){
  char line[256];

  FILE *problem = fopen(filename,"r");
  if (problem == NULL){
    fprintf(stderr,"File %s not found.\n",filename);
    exit(-1);
  }

  while (fgets(line, 256, problem) != NULL){
    switch(line[0]){
    case 'c': // capacity
      if (sscanf(&(line[2]),"%d\n", &capacity) != 1){
	fprintf(stderr,"Error in file format in line:\n");
	fprintf(stderr, "%s", line);
	exit(-1);
      }
      break;

    case 'o': // graph size
      if (num_obj >= MAX_NUM_OBJ){
	fprintf(stderr,"Too many objects (%d): limit is %d\n", num_obj, MAX_NUM_OBJ);
	exit(-1);
      }
      if (sscanf(&(line[2]),"%d %d\n", &(weight[num_obj]), &(utility[num_obj])) != 2){
	fprintf(stderr,"Error in file format in line:\n");
	fprintf(stderr, "%s", line);
	exit(-1);
      }
      else
	num_obj++;
      break;

    default:
      break;
    }
  }
  if (num_obj == 0){
    fprintf(stderr,"Could not find any object in the problem file. Exiting.");
    exit(-1);
  }
}


int ** calculMat(){

    // Init
    int ** matrice = malloc(num_obj * sizeof(int*));
    for(int i = 0; i < num_obj; i++){
        matrice[i] = malloc((capacity + 1) * sizeof(int));
    }

    // Calcul Ligne 0
    #pragma omp parallel for num_threads(NB_THREADS)
    for(int j = 0; j < (capacity + 1); j++){
        if(weight[0] <= j) matrice[0][j] = utility[0];
        else matrice[0][j] = 0;
    }

    // Calcul S[i][j]
    #pragma omp parallel num_threads(NB_THREADS)
    for(int i = 1; i < num_obj; i++){
        #pragma omp for
        for(int j = 0; j < capacity + 1; j++){

            if(j < weight[i]){
                matrice[i][j] = matrice[i-1][j];
            }else{
              int utilite = 0;
                if(weight[i] < j){
                  utilite = utility[i] + matrice[i-1][j - weight[i]];
                }else{
                  utilite = utility[i];
                }
              if (utilite > matrice[i-1][j]){
                matrice[i][j] = utilite;
              }else{
                matrice[i][j] = matrice[i-1][j];
              }
            }


        }
    }
    return matrice;
}

int * calculE(int ** matrice){
  int * E = malloc(sizeof(int) * (num_obj));
  int i = num_obj - 1;
  int j = capacity;
  while(i != 0){
    if(matrice[i-1][j] != matrice[i][j]){
      E[i] = 1;
      j -= weight[i];
      i--;
    }else{
      i--;
    }
  }
  if (matrice[i][j] > 0){
    E[i] = 1;
  }
  #pragma omp parallel for num_threads(NB_THREADS)
  for(int i = 0; i < num_obj; i++){
    if (E[i] != 1){
      E[i] = 0;
    }
  }
  printf("Utilite = %d\n", matrice[num_obj-1][capacity]);
  return E;
}


void printE(int * E){
  for(int i = 0; i < num_obj; i++){
    if(E[i] == 1){
      printf("%d ", i);
    }
  }
  printf("\n");
}


void doProb(char * file){
  double t = 0, start = 0, stop = 0;
  read_problem(file);
  NB_THREADS = 2;
  while (NB_THREADS <= 16){
    start = omp_get_wtime();
    int ** matrice = calculMat();
    int * E = calculE(matrice);
    //printE(E);
    free(E);
    free(matrice);
    stop=omp_get_wtime();
    t=stop-start;
    printf("Pour le probleme %s et avec %d threads c'est fini en %f\n", file, NB_THREADS, t);
    NB_THREADS *= 2;
  }
}


void clearProb(){
  num_obj = 0;
  capacity = 0;
  for(int i = 0; i < MAX_NUM_OBJ; i++){
    weight[i] = 0;
    utility[i] = 0;
  }
}
int main(){
  doProb("exo4Pb1");
  printf("\n\n\n");
  clearProb();
  doProb("exo4Pb2");
  clearProb();
  printf("\n\n\n");
  doProb("exo4Pb3");
  return 0;
}
