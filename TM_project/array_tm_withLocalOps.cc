/*
 * An array access micro-benchmark for Transactional Memory studies
 */

#include <assert.h>
#include <cstdio>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "../../src/tm.h"

#define N_ELEMENTS		        3000	      // Size of the array
#define N_ELEMENTS_LOCAL_TOTAL          3000          // Size of the array for local access

#define N_ITER		            50000000	    // No of iterations

#define RAND()	((unsigned int)random())

int N_THREADS;		                // No of threads accessing the array in parallel
unsigned int WR_PERCENTAGE;             // Percentage of write accesses from the total amount of accesses
unsigned int WR_PERCENTAGE_LOCAL;       // Percentage of write local accesses from the total amount of accesses
bool random_access = false;               // Default: random accesses
bool random_access_local = false          // Default: random local accesses
int N_ACCESS;         // No. of elements to access per transaction
int N_ACCESS_LOCAL;   // No. of elements(local use) to access per transaction
int MAX_RETRIES;	  // Max retry no.

unsigned int TAG_GRAN;	// ??
unsigned int TAG_MODE;


#ifdef TM
# define INT tm_int
#else
# define INT int
#endif

INT *main_array;                          // the main array of the benchmark
int *local_array;                         // the local array for each thread

TM_INIT();                                // initialize the TM system (creating mutexes and transaction objects)
//Mutex tm_mutex;                        

#ifdef PTHREAD_FINE
pthread_mutex_t *mutex_array;			// ??
#elif PTHREAD_COARSE
pthread_mutex_t mutex_array = PTHREAD_MUTEX_INITIALIZER;
#endif

// thread actions
#if defined(PTHREAD_FINE) || defined(PTHREAD_CORASE)
void *do_pthread_work (void* id) {
#else
int do_tm_work ( void* arg, int id ) {
#endif

#ifdef TIMER
  double begin_tm = 0.0;
  double elapsed_tm = 0.0;                 // Timer with TM overhead
  struct timeval timer;
#endif

#if defined(TM) && defined(STATS)
  unsigned int per_thread_txn_started = 0;
  unsigned int *per_thread_abort_cdf = new unsigned int[MAX_RETRIES + 1]();
#endif

  int i, j, k, temp = 0, base;
  int _writes = 0, _reads = 0;
  int _writesLocal = 0, _readsLocal = 0;
  int N_ELEMENTS_LOCAL = N_ELEMENTS_LOCAL_TOTAL / N_THREADS;
  //unsigned int temp_wr, temp_rd, temp_wr1, temp_rd1;     // temp: temporary variable

  srandom(1);

  int *index_array = new int[N_ACCESS * 10];             // indexes of the elements to be accessed
  int *index_array_local = new int[N_ACCESS_LOCAL * 10]; // indexes of the "local" elements to be acessed

  int tag_array_size = N_ELEMENTS / TAG_GRAN + 1;

  ver_t *tag_array = new ver_t[tag_array_size];

  if (TAG_MODE == 0) { // continuous mode
	  for (i = 0; i < tag_array_size; ++i) {
		  for (j = i * TAG_GRAN; j < N_ELEMENTS && j < (i + 1) * TAG_GRAN; ++j) {
			  main_array[j].__meta = &tag_array[i];
		  }
	  }
  } else if(TAG_MODE == 1) { // random mode
	  for (i = 0; i < N_ELEMENTS; ++i) {
	  	  main_array[i].__meta = &tag_array[(RAND() % tag_array_size) + 1];
	  }
  } else {	//exact mode

  }

  if(random_access) {
    for (k = 0; k < 10; k++) {
      for (i = 0; i < N_ACCESS; i++) {
        if ((RAND() % 100) < WR_PERCENTAGE) {
	        index_array[k * N_ACCESS + i] = -((RAND() % N_ELEMENTS) + 1);  // index for write access
          ++_writes;        
        }
	      else {
          index_array[k * N_ACCESS + i] = ((RAND() % N_ELEMENTS) + 1);   // index for read access
          ++_reads;
        }
      }
    }
  } else {                                                        // continuous access
      for( k = 0; k < 10; k++ ) {
        int cnt_base = RAND() % (N_ELEMENTS - N_ACCESS);
        for( i = 0; i < N_ACCESS; i++ ) {
          if( (RAND() % 100) < WR_PERCENTAGE ) {
            index_array[k * N_ACCESS + i] = -(cnt_base + i);   // index for write access
            ++_writes;     
          }
          else {
            index_array[k * N_ACCESS + i] = (cnt_base + i);      // index for read access
            ++_reads;
          }
        }
      }
  }

  if(random_access_local) {
    for (k = 0; k < 10; k++) {
      for (i = 0; i < N_ACCESS_LOCAL; i++) {
        if ((RAND() % 100) < WR_PERCENTAGE_LOCAL) {
	        index_array_local[k * N_ACCESS_LOCAL + i] = -((RAND() % N_ELEMENTS_LOCAL) + 1);  // index for local write access
          ++_writes_local;        
        }
	      else {
          index_array_local[k * N_ACCESS_LOCAL + i] = ((RAND() % N_ELEMENTS_LOCAL) + 1);   // index for local read access
          ++_reads_local;
        }
      }
    }
  } else {                                                        // continuous access
      for( k = 0; k < 10; k++ ) {
        int cnt_base_local = RAND() % (N_ELEMENTS_LOCAL - N_ACCESS_LOCAL);
        for( i = 0; i < N_ACCESS_LOCAL; i++ ) {
          if( (RAND() % 100) < WR_PERCENTAGE_LOCAL ) {
            index_array_local[k * N_ACCESS_LOCAL + i] = -(cnt_base_local + i);   // index for local write access
            ++_writes_local;     
          }
          else {
            index_array_local[k * N_ACCESS_LOCAL + i] = (cnt_base_local + i);      // index for local read access
            ++_reads_local;
          }
        }
      }
  }

#ifdef TIMER
    gettimeofday(&timer, NULL);
    begin_tm = timer.tv_sec+(timer.tv_usec / 1000000.0);
#endif

  for (k = 0; k < N_ITER/N_THREADS; k++) {
    base = RAND() % 10 * N_ACCESS;
    base_local = RAND() % 10 * N_ACCESS_LOCAL;

#ifdef TM
    BEGIN_TRANSACTION();
    //Transaction tm_guard(tm_mutex); 
    //tm_guard.TransactionStart();
#endif
/*
    for (i = 0, j = index_array_rd[base + i]; i < _reads; j = index_array_rd[++i]) {
      temp = main_array[j]; 
    }

    for (i = 0, j = index_array_wr[base + i]; i < _writes; j = index_array_wr[++i]) {
      main_array[-j] = temp; 
    }
    */
    for (i = 0, j = index_array[base + i]; i < N_ACCESS; j = index_array[base + (++i)]) {
      if (j > 0) {
        temp = main_array[j]; // read access, temp is a throwaway variable 
      }
      else {
        main_array[-j] = temp; // write access
      }
    }

    // Below is the local access;
    for (i = 0, j = index_array_local[base_local + i]; i < N_ACCESS_LOCAL; j = index_array_local[base_local + (++i)]) {
      if (j > 0) {
        temp = local_array[j + id * N_ELEMENT_LOCAL]; // read local access, temp is a throwaway variable 
      }
      else {
        local_array[-j + id * N_ELEMENT_LOCAL] = temp; // write local access
      }
    }

    
#ifdef TM
    END_TRANSACTION();
    //tm_guard.TransactionEnd();
#endif
 }

#ifdef TIMER
	  gettimeofday(&timer, NULL);
    elapsed_tm += timer.tv_sec+(timer.tv_usec/1000000.0) - begin_tm;
  printf( "\n %d: Time in TXs: %lf\n", id, elapsed_tm); fflush(0);
#endif

#ifdef STATS
  printf("thread%d,%d,", id, per_thread_txn_started); fflush(0); 
  
  for (i = 1; i <= MAX_RETRIES; ++i) {  
    printf("%d,", per_thread_abort_cdf[i]); fflush(0);
  }
  if (MAX_RETRIES < 5) {
    for (i = 0; i < 5 - MAX_RETRIES; ++i) {
      printf(",");
    }
  }
#endif
  delete [] index_array;
  //delete [] index_array_wr;
  //delete [] index_array_rd;

  return 0;
}

int main(int argc, char** argv) {  
  if (argc < 8) {
    printf("argc: %d\n", argc);
    printf("usage: ./array N_Threads WR_Percentage txn_size max_retries random/continuous_access(1 or 0) elems_per_tag continuous/random/exact \n");
    assert(argc >= 6); 
  }

  N_THREADS = atoi(argv[1]);
  WR_PERCENTAGE = atoi(argv[2]);
  N_ACCESS = atoi(argv[3]);
  MAX_RETRIES = atoi(argv[4]);
  
  if (atoi(argv[5]) != 0)
    random_access = true;

  TAG_GRAN = atoi(argv[6]);
  TAG_MODE = atoi(argv[7]);
   
  // Allocate storage for the main array
  main_array = new INT[N_ELEMENTS + 1];
  local_array = new int[N_ELEMENTS_LOCAL_TOTAL + 1];

#ifdef PTHREAD_FINE
  unsigned int i;
  mutex_array = new pthread_mutex_t[N_ELEMENTS + 1];
  for (i = 0; i < N_ELEMENTS + 1; ++i)
    pthread_mutex_init(&mutex_array[i], NULL);
#endif

#if defined(PTHREAD_FINE) || defined(PTHREAD_COARSE)
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);

  pthread_t *threads = new pthread_t[N_THREADS];
  
  long args[N_THREADS];

  for (i = 0; i < N_THREADS; ++i) {
    args[i] = i;
    pthread_create(&threads[i], NULL, do_thread_work, (void *) &args[i]);
  }

  for (i = 0; i < N_THREADS; ++i)
    pthread_join(threads[i], NULL);
#elif TM
  CREATE_TM_THREADS( N_THREADS );

  PARALLEL_EXECUTE( N_THREADS, do_tm_work, NULL );

  TM_END();

  DESTROY_TM_THREADS( N_THREADS );
  printf("\n");
#endif
     
  delete [] main_array;

  return (EXIT_SUCCESS);
}
