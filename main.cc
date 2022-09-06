/******************************************************************
 * The Main program with the two functions. A simple
 * example of creating and using a thread is provided.
 ******************************************************************/

#include "helper.h" 

void *producer (void *id);
void *consumer (void *id);

  struct ProducerStruct {
    int jobsPerProducer;
    int* queue;
    int queueLength;
    int producerNumber;
    int id;
    int* producerQueueCount;
    void producerStructInit(int jobsPerProducer, int* queue, int queueLength, int producerNumber,
		       int id, int* producerQueueCount){
      this->jobsPerProducer=jobsPerProducer;
      this->queue=queue;
      this->queueLength=queueLength;
      this->producerNumber=producerNumber;
      this->id=id;
      this->producerQueueCount=producerQueueCount;
    }
  };
    
  struct ConsumerStruct {         
    int* queue;
    int queueLength;
    int consumerNumber;
    int id;
    int* consumerQueueCount;
    void consumerStructInit(int* queue, int queueLength, int consumerNumber, int id,
		       int* consumerQueueCount){
      this->queue=queue;
      this->queueLength=queueLength;
      this->consumerNumber=consumerNumber;
      this->id=id;
      this->consumerQueueCount=consumerQueueCount;
    }
  };


int main (int argc, char **argv)
{

  //errors for wrong number of/incorrect arguments

  if (argc!=5){
    cerr << "Number of command line arguments must be 4\n";
    exit(1);
  }

  for (int i=1; i<5; i++){
    if (argv[i]<0){
      cerr << "Command line argument " << i << " must be an integer greater than -1\n";
      exit(1);
    }
  }
  
  //reading command line arguments
  int queueLength = check_arg(argv[1]);
  int jobsPerConsumer = check_arg(argv[2]);
  int noProducers = check_arg(argv[3]);
  int noConsumers = check_arg(argv[4]);
 
  
  //setting up data structures and variables
  auto queue = new int[queueLength];

  int producerQueueCount=0;
  int consumerQueueCount=0;

  auto producer_structs = new ProducerStruct[noProducers];
  auto consumer_structs = new ConsumerStruct[noConsumers];

  srand(time(0));   // seeding the random number generator for use in producer threads

  //setting up semaphores
  int id = sem_create(SEM_KEY, 3);
  if (id<0){
    perror("Error creating semaphore array");
    exit(1);
  }
  if (sem_init(id, mutex, 1)<0){
    perror("Error initialising mutex semaphore");
    exit(1);
  }
  if (sem_init(id, item, 0)<0){
    perror("Error initialising item semaphore");
    exit(1);
  }
  if (sem_init(id, space, queueLength)<0){
    perror("Error initialising space semaphore");
    exit(1);
  }
  
  //initialising struct objects and creating producer and consumer threads
  for (int i=0; i<noProducers; i++){
    producer_structs[i].producerStructInit(jobsPerConsumer, queue, queueLength, i+1, id,
					 &producerQueueCount);
  }
  for (int i=0; i<noConsumers; i++){
    consumer_structs[i].consumerStructInit(queue, queueLength, i+1, id,
					 &consumerQueueCount);
  }
  auto producerids = new pthread_t [noProducers];
  auto consumerids = new pthread_t [noConsumers];
  for (int i=0; i<noProducers; i++){
    pthread_create (producerids+i, NULL, producer, (void *) &producer_structs[i]);
  }
  for (int i=0; i<noConsumers; i++){
    pthread_create (consumerids+i, NULL, consumer, (void *) &consumer_structs[i]);
  }   

  //wait until producer and consumer threads have finished before exiting
  for (int i=0; i<noProducers; i++){
      pthread_join (producerids[i], NULL);
    }
  for (int i=0; i<noConsumers; i++){
      pthread_join (consumerids[i], NULL);
    }

  //clean up
  if(sem_close(id)<0){
    perror("Error removing semaphore array");
    exit(1);
  }

  delete[] queue;
  delete[] producer_structs;
  delete[] consumer_structs;
  delete[] producerids;
  delete[] consumerids;
  return 0;
}

void *producer (void *parameter) { 
  auto producer_struct = (ProducerStruct *) parameter;
  int jobsPerProducer = producer_struct->jobsPerProducer;
  int* queue = producer_struct->queue;
  int queueLength = producer_struct->queueLength;
  int producerNumber = producer_struct->producerNumber;
  int id = producer_struct->id;
  int* producerQueueCount = producer_struct->producerQueueCount;
  
  for (int i=0; i<jobsPerProducer; i++){
    sleep (rand()%5+1);  
    int duration = rand()%10+1;
    
    if (sem_wait_20s(id, space) == -1){
      printf("Producer(%d): no slot available after 20 seconds wait.\n", producerNumber);
      pthread_exit(0);
    }
    sem_wait(id, mutex);
    
    queue[*producerQueueCount]=duration;
    int jobID = *producerQueueCount;
    *producerQueueCount = (*producerQueueCount+1)%queueLength;
    
    sem_signal(id, mutex);
    sem_signal(id, item);

    printf("Producer(%d): Job id %d duration %d\n", producerNumber, jobID, duration);
  }
  
  printf("Producer(%d): No more jobs to generate.\n", producerNumber);
  pthread_exit(0);
}

void *consumer (void *parameter) {
  auto consumer_struct = (ConsumerStruct *) parameter ;
  int* queue = consumer_struct->queue;
  int queueLength = consumer_struct->queueLength;
  int consumerNumber = consumer_struct->consumerNumber;
  int id = consumer_struct->id;
  int* consumerQueueCount = consumer_struct->consumerQueueCount;

  while(1){
    if (sem_wait_20s(id, item) == -1){
      printf("Consumer(%d): No more jobs left.\n", consumerNumber);
      pthread_exit(0);
    }
    sem_wait(id, mutex);
    
    int duration = queue[*consumerQueueCount];
    int jobID=*consumerQueueCount;
    *consumerQueueCount = (*consumerQueueCount+1)%queueLength;

    sem_signal(id, mutex);
    sem_signal(id, space);

    printf("Consumer(%d): Job id %d executing sleep duration %d\n", consumerNumber,
	   jobID, duration);    
    sleep (duration);
    printf("Consumer(%d): Job id %d completed\n",consumerNumber, jobID);      
  }
} 
  
