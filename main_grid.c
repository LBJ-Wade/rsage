#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <signal.h>
#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>

#ifdef MPI
#include <mpi.h>
#endif

#include "core_allvars_grid.h"
#include "core_proto_grid.h"

char buf[MAXLEN];

struct sigaction saveaction_XCPU;
volatile sig_atomic_t gotXCPU = 0;
int exitfail = 1;

void termination_handler(int signum)
{
  gotXCPU = 1;
  sigaction(SIGXCPU, &saveaction_XCPU, NULL);
  if(saveaction_XCPU.sa_handler != NULL)
    (*saveaction_XCPU.sa_handler) (signum);
}

void myexit(int signum)
{
#ifdef MPI
  printf("Task: %d\tnode: %s\tis exiting\n\n\n", ThisTask, ThisNode);
#else
  printf("We're exiting\n\n\n");
#endif    
  exit(signum);
}

void bye()
{
#ifdef MPI
  MPI_Finalize();
  free(ThisNode);
#endif

  if(exitfail)
  {
    unlink(buf);

#ifdef MPI
    if(ThisTask == 0 && gotXCPU == 1)
      printf("Received XCPU, exiting. But we'll be back.\n");
#endif
  }
}





int main(int argc, char **argv)
{

  struct sigaction current_XCPU;

#ifdef MPI
  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &ThisTask);
  MPI_Comm_size(MPI_COMM_WORLD, &NTask);

  ThisNode = malloc(MPI_MAX_PROCESSOR_NAME * sizeof(char));

  MPI_Get_processor_name(ThisNode, &nodeNameLen);
  if (nodeNameLen >= MPI_MAX_PROCESSOR_NAME)
  {
    printf("Node name string not long enough!...\n");
    ABORT(0);
  }
#endif

  atexit(bye);

  sigaction(SIGXCPU, NULL, &saveaction_XCPU);
  current_XCPU = saveaction_XCPU;
  current_XCPU.sa_handler = termination_handler;
  sigaction(SIGXCPU, &current_XCPU, NULL);


  int filenr, p, i, GridNr, ThisTask_GridNr = 0;
 
  read_parameter_file(argv[1]);
  init(); // Initialize all the parameters (set units, create scale factor/age arrays etc).
#ifdef MPI
  for (GridNr = ThisTask; GridNr < NGrid; GridNr += NTask)
#else
  for (GridNr = 0; GridNr < NGrid; ++GridNr)
#endif
  {
    if (Verbose == 1)
	fprintf(stderr, "Task %d is doing redshift %.3f\n", ThisTask, ZZ[ListOutputGrid[GridNr]]);
    init_grid(GridNr, ThisTask_GridNr); // Initialize the grid. 
//    update_grid_diffuse(GridNr); // Read in all the diffuse gas.
    for (filenr = FirstFile; filenr < LastFile + 1; ++filenr)
    {
      if(filenr % 30 == 0)
      printf("Doing file %d.\n", filenr);

    //  load_halos(filenr); // Load the halos.
    //  update_grid_halo(totNHalos, GridNr); // Update the properties associated with halos
    //  myfree(Halo); // Don't need halos anymore.

      for (i = 0; i < 2; ++i) // i = 0 does the normal galaxies, i = 1 does the merged galaxies.
      {
        if(i == 0)      
	  snprintf(buf, MAXLEN, "%s/%s_%d", GalaxiesInputDir, FileNameGalaxies, filenr);
        else       
	  snprintf(buf, MAXLEN, "%s/%s_%d", GalaxiesInputDir, FileNameMergedGalaxies, filenr); 

        if ( access(buf, F_OK ) == -1) // Sanity check.
        {
          printf("-- input for file %s does not exist, exiting now.\n", buf);
          exit(0); 
        }

//        if (Verbose == 1)
//         printf("Loading galaxies for file %d, name '%s'\n", filenr, buf); 
        load_gals(buf);    

//        if (Verbose == 1)
//          printf("Gridding properties.\n");

        for(p = 0; p < NtotGals; ++p)
        {    	
          update_grid_properties(p, i, GridNr); // Go through each galaxy and read it's grid history and grid the properties.
        }

	free_gals();	

      }

//      printf("Done File %d.\n\n", filenr);
    }
  
//    update_grid_density(GridNr); // Go through the grid and convert to overdensity.
 
    count_grid_properties(GridNr); // Counts how many halos/galaxies/Photons are in the grid at each redshift.
    save_grid(GridNr); // Saves grid.

    ++ThisTask_GridNr; 
  }

  if (ThisTask == 0)
  {
    save_redshift();
  }
  myfree(Grid);

  exitfail = 0;
  gsl_rng_free(random_generator); 
  return 0;

} 
