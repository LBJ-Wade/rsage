#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <assert.h>

#include "core_allvars.h"
#include "core_proto.h"


// keep a static file handle to remove the need to do constant seeking
FILE* load_fd = NULL;


void load_tree_table(int filenr)
{
  int i, n, totNHalos;
  char buf[MAXLEN], tag[MAXLEN];
  FILE *fd;

  if(use_tiamat == 1)
  {
    if(filenr < 10)
      snprintf(tag, MAXLEN, "00%d", filenr);
    else if (filenr >= 10 && filenr < 100)
      snprintf(tag, MAXLEN, "0%d", filenr);
    else
      snprintf(tag, MAXLEN, "%d", filenr);
    sprintf(buf, "%s/%s_%s.dat", SimulationDir, TreeName, tag);
  }
  else
  {  
    snprintf(buf, MAXLEN, "%s/%s.%d", SimulationDir, TreeName, filenr);
  }
  fprintf(stderr, "Reading file %s\n", buf);
  if(!(load_fd = fopen(buf, "r")))
  {
    printf("can't open file `%s'\n", buf);
    ABORT(0);
  }

  myfread(&Ntrees, 1, sizeof(int), load_fd);
  myfread(&totNHalos, 1, sizeof(int), load_fd);

  TreeNHalos = mymalloc(sizeof(int) * Ntrees);
  TreeFirstHalo = mymalloc(sizeof(int) * Ntrees);

  for(n = 0; n < NOUT; n++)
    TreeNgals[n] = mymalloc(sizeof(int) * Ntrees);
  TreeNMergedgals = mymalloc(sizeof(int)* Ntrees);
  myfread(TreeNHalos, Ntrees, sizeof(int), load_fd); 

  if(Ntrees)
    TreeFirstHalo[0] = 0;
  for(i = 1; i < Ntrees; i++)
    TreeFirstHalo[i] = TreeFirstHalo[i - 1] + TreeNHalos[i - 1];

  for(n = 0; n < NOUT; n++)
  {
    for(i = 0; i < Ntrees; i++)
    {
      TreeNgals[n][i] = 0;
      TreeNMergedgals[i] = 0;
    }
    sprintf(buf, "%s/%s_z%1.3f_%d", OutputDir, FileNameGalaxies, ZZ[ListOutputSnaps[n]], filenr);

    if(!(fd = fopen(buf, "w")))
    {
      printf("can't open file `%s'\n", buf);
      ABORT(0);
    }
    fclose(fd);
    TotGalaxies[n] = 0;
  }
  TotMerged = 0;
  fprintf(stderr, "Read the table\n");
}



void free_tree_table(void)
{
  int n;

  myfree(TreeNMergedgals); 
  for(n = NOUT - 1; n >= 0; n--)
    myfree(TreeNgals[n]);

  myfree(TreeFirstHalo);
  myfree(TreeNHalos);
	
	// Don't forget to free the open file handle
	if(load_fd) {
		fclose(load_fd);
		load_fd = NULL;
	}
}



void load_tree(int filenr, int nr)
{
  int i;

  // must have an FD
  assert( load_fd );

  Halo = mymalloc(sizeof(struct halo_data) * TreeNHalos[nr]);  
  myfread(Halo, TreeNHalos[nr], sizeof(struct halo_data), load_fd);

  MaxGals = (int)(MAXGALFAC * TreeNHalos[nr]);

  if(MaxGals < 10000)  
    MaxGals = 10000;

  MaxMergedGals = MaxGals;
  FoF_MaxGals = 10000; 

  HaloAux = mymalloc(sizeof(struct halo_aux_data) * TreeNHalos[nr]);
  HaloGal = mymalloc(sizeof(struct GALAXY) * MaxGals);
  Gal = mymalloc(sizeof(struct GALAXY) * FoF_MaxGals);
  MergedGal = mymalloc(sizeof(struct GALAXY) * MaxMergedGals);   

 
  for(i = 0; i < TreeNHalos[nr]; i++)
  {
    //if(Halo[i].NextHaloInFOFgroup != -1)
     // fprintf(stderr, "%d\n", nr);
  
   // fprintf(stderr, "Halo: %d. FirstProg = %d, NextProg = %d, FirstHaloInFOFgroup = %d, NextHaloInFOFgroup = %d SnapNum = %d, Mvir = %.4e\n", i, Halo[i].FirstProgenitor, Halo[i].NextProgenitor, Halo[i].FirstHaloInFOFgroup, Halo[i].NextHaloInFOFgroup, Halo[i].SnapNum, Halo[i].Mvir);
    //if(Halo[i].FirstProgenitor != -1)
    //XASSERT(Halo[i].SnapNum != Halo[Halo[i].FirstProgenitor].SnapNum, "Halo[%d].SnapNum = %d, Halo[%d].FirstProgenitor = %d, Halo[Halo[%d].FirstProgenitor].SnapNum = %d\n", i, Halo[i].SnapNum, i, Halo[i].FirstProgenitor, i, Halo[Halo[i].FirstProgenitor].SnapNum);
    HaloAux[i].DoneFlag = 0;
    HaloAux[i].HaloFlag = 0;
    HaloAux[i].NGalaxies = 0;
    if (Halo[i].SnapNum != LastSnapShotNr && Halo[i].Descendant == -1)
fprintf(stderr, "STUPID TREES! Halo[%d].SnapNum = %d Halo[%d].Descendant = %d\n", i, Halo[i].SnapNum, i, Halo[i].Descendant); 
  }
 
//  exit(EXIT_FAILURE);
}



void free_galaxies_and_tree(void)
{
  int i;

  for(i = 0; i < NumGals; ++i)
  { 
    if(HaloGal[i].SnapNum == LastSnapShotNr) // The pointed to memory that we malloced is NOT copied over when we generate a new MergedGal entry. 
    {
      XPRINT(HaloGal[i].IsMalloced == 1, "HaloGal %d doesn't have grids mallocced but we're trying to free it.\n", i); 
      free_grid_arrays(&HaloGal[i]); 
     ++gal_frees;
    } 
  }

  for(i = 0; i < MergedNr; ++i)
  {
      XPRINT(MergedGal[i].IsMalloced == 1, "MergedGal %d doesn't have grids mallocced but we're trying to free it.\n", i); 
    free_grid_arrays(&MergedGal[i]); // These are the Gal[xxx] entries that were copied over to  
    ++mergedgal_frees;
  } 

  myfree(MergedGal);
  myfree(Gal);
  myfree(HaloGal);
  myfree(HaloAux);
  myfree(Halo);
}

void free_grid_arrays(struct GALAXY *g)
{
  free(g->GridHistory);
  free(g->GridStellarMass);
  free(g->GridSFR);
  free(g->GridZ);
  free(g->GridCentralGalaxyMass);
  free(g->MfiltGnedin);
  free(g->MfiltSobacchi);
  free(g->EjectedFraction);
  free(g->LenHistory);
  free(g->Stars);
//  free(g->GridOutflowRate);
  g->IsMalloced = 0;
}

void malloc_grid_arrays(struct GALAXY *g)
{
  if(NULL == (g->GridHistory = malloc(sizeof(*(g->GridHistory)) * MAXSNAPS)))
  {
    fprintf(stderr, "Out of memory allocating %ld bytes, could not allocate GridHistory.", sizeof(*(g->GridHistory))*MAXSNAPS); 
    exit(EXIT_FAILURE);
  }

  if(NULL == (g->GridStellarMass = malloc(sizeof(*(g->GridStellarMass)) * MAXSNAPS)))
  {
    fprintf(stderr, "Out of memory allocating %ld bytes, could not allocate GridStellarMass.", sizeof(*(g->GridStellarMass))*MAXSNAPS); 
    exit(EXIT_FAILURE);
  } 

  if(NULL == (g->GridSFR = malloc(sizeof(*(g->GridSFR)) * MAXSNAPS)))
  {
    fprintf(stderr, "Out of memory allocating %ld bytes, could not allocate GridSFR.", sizeof(*(g->GridSFR))*MAXSNAPS);
    exit(EXIT_FAILURE);
  }

  if (NULL == (g->GridZ = malloc(sizeof(*(g->GridZ)) * MAXSNAPS)))
  { 
    fprintf(stderr, "Out of memory allocating %ld bytes, could not allocate GridSFR.", sizeof(*(g->GridZ))*MAXSNAPS);
    exit(EXIT_FAILURE);
  }
 
  if (NULL == (g->GridCentralGalaxyMass = malloc(sizeof(*(g->GridCentralGalaxyMass)) * MAXSNAPS)))
  { 
    fprintf(stderr, "Out of memory allocating %ld bytes, could not allocate GridCentralGalaxyMass.", sizeof(*(g->GridCentralGalaxyMass))*MAXSNAPS);
    exit(EXIT_FAILURE);
  }

  if (NULL == (g->MfiltGnedin = malloc(sizeof(*(g->MfiltGnedin)) * MAXSNAPS)))
  {
    fprintf(stderr, "Out of memory allocating %ld bytes, could not allocate MfiltGnedin.", sizeof(*(g->MfiltGnedin))*MAXSNAPS);
    exit(EXIT_FAILURE);
  }

  if (NULL == (g->MfiltSobacchi = malloc(sizeof(*(g->MfiltSobacchi)) * MAXSNAPS)))
  {   
    fprintf(stderr, "Out of memory allocating %ld bytes, could not allocate MfiltSobacchi.", sizeof(*(g->MfiltSobacchi))*MAXSNAPS);
    exit(EXIT_FAILURE);
  }
 
  if (NULL == (g->EjectedFraction = malloc(sizeof(*(g->EjectedFraction)) * MAXSNAPS)))
  { 
    fprintf(stderr, "Out of memory allocating %ld bytes, could not allocate EjectedFraction.", sizeof(*(g->EjectedFraction))*MAXSNAPS);
    exit(EXIT_FAILURE);
  }
 
  if (NULL == (g->LenHistory = malloc(sizeof(*(g->LenHistory)) * MAXSNAPS)))
  { 
    fprintf(stderr, "Out of memory allocating %ld bytes, could not allocate LenHistory.", sizeof(*(g->LenHistory))*MAXSNAPS);
    exit(EXIT_FAILURE);
  }

  if (NULL == (g->Stars = malloc(sizeof(*(g->Stars)) * SN_Array_Len)))
  { 
    fprintf(stderr, "Out of memory allocating %ld bytes, could not allocate Stars.", sizeof(*(g->Stars))*SN_Array_Len);
    exit(EXIT_FAILURE);
  }


  g->IsMalloced = 1;
}


size_t myfread(void *ptr, size_t size, size_t nmemb, FILE * stream)
{
  return fread(ptr, size, nmemb, stream);
}

size_t myfwrite(void *ptr, size_t size, size_t nmemb, FILE * stream)
{
  return fwrite(ptr, size, nmemb, stream);
}

int myfseek(FILE * stream, long offset, int whence)
{
  return fseek(stream, offset, whence);
}
