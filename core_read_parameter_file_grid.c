#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

#include "core_allvars_grid.h"
#include "core_proto_grid.h"

#define DOUBLE 1
#define STRING 2
#define INT 3
#define MAXTAGS 300



void read_parameter_file(char *fname)
{
  FILE *fd;
  char buf[400], buf1[400], buf2[400], buf3[400];
  int i, j, nt = 0, done;
  int id[MAXTAGS];
  void *addr[MAXTAGS];
  char tag[MAXTAGS][50];
  int errorFlag = 0;


#ifdef MPI
  if(ThisTask == 0)
#endif
    printf("\nreading parameter file:\n\n");

  strcpy(tag[nt], "GridOutputDir");
  addr[nt] = GridOutputDir;
  id[nt++] = STRING;

  strcpy(tag[nt], "GalaxiesInputDir");
  addr[nt] = GalaxiesInputDir;
  id[nt++] = STRING;

  strcpy(tag[nt], "FileNameGalaxies");
  addr[nt] = FileNameGalaxies;
  id[nt++] = STRING;

  strcpy(tag[nt], "FileNameMergedGalaxies");
  addr[nt] = FileNameMergedGalaxies;
  id[nt++] = STRING;

  strcpy(tag[nt], "TreeName");
  addr[nt] = TreeName;
  id[nt++] = STRING;

  strcpy(tag[nt], "SimulationDir");
  addr[nt] = SimulationDir;
  id[nt++] = STRING;

  strcpy(tag[nt], "DiffuseDir");
  addr[nt] = DiffuseDir;
  id[nt++] = STRING;

  strcpy(tag[nt], "FileWithSnapList");
  addr[nt] = FileWithSnapList;
  id[nt++] = STRING;

  strcpy(tag[nt], "LastSnapShotNr");
  addr[nt] = &LastSnapShotNr;
  id[nt++] = INT;

  strcpy(tag[nt], "FirstFile");
  addr[nt] = &FirstFile;
  id[nt++] = INT;

  strcpy(tag[nt], "LastFile");
  addr[nt] = &LastFile;
  id[nt++] = INT;

  strcpy(tag[nt], "UnitVelocity_in_cm_per_s");
  addr[nt] = &UnitVelocity_in_cm_per_s;
  id[nt++] = DOUBLE;

  strcpy(tag[nt], "UnitLength_in_cm");
  addr[nt] = &UnitLength_in_cm;
  id[nt++] = DOUBLE;

  strcpy(tag[nt], "UnitMass_in_g");
  addr[nt] = &UnitMass_in_g;
  id[nt++] = DOUBLE;

  strcpy(tag[nt], "Hubble_h");
  addr[nt] = &Hubble_h;
  id[nt++] = DOUBLE;

  strcpy(tag[nt], "BaryonFrac");
  addr[nt] = &BaryonFrac;
  id[nt++] = DOUBLE;

  strcpy(tag[nt], "Omega");
  addr[nt] = &Omega;
  id[nt++] = DOUBLE;

  strcpy(tag[nt], "OmegaLambda");
  addr[nt] = &OmegaLambda;
  id[nt++] = DOUBLE;

  strcpy(tag[nt], "PartMass");
  addr[nt] = &PartMass;
  id[nt++] = DOUBLE;

  strcpy(tag[nt], "BoxSize");
  addr[nt] = &BoxSize;
  id[nt++] = DOUBLE;

  strcpy(tag[nt], "GridSize");
  addr[nt] = &GridSize;
  id[nt++] = INT;

  strcpy(tag[nt], "NGrid");
  addr[nt] = &NGrid; 
  id[nt++] = INT;

  strcpy(tag[nt], "LastOutputSnap");
  addr[nt] = &LastOutputSnap; 
  id[nt++] = INT;

  strcpy(tag[nt], "SourceEfficiency");
  addr[nt] = &SourceEfficiency;
  id[nt++] = DOUBLE;

  strcpy(tag[nt], "FeedbackReheatingEpsilon");
  addr[nt] = &QuasarModeEfficiency;

  strcpy(tag[nt], "OutputMode");
  addr[nt] = &OutputMode;
  id[nt++] = INT;

  strcpy(tag[nt], "LowSnap");
  addr[nt] = &LowSnap;
  id[nt++] = INT;

  strcpy(tag[nt], "HighSnap");
  addr[nt] = &HighSnap;
  id[nt++] = INT;

  strcpy(tag[nt], "PhotonPrescription");
  addr[nt] = &PhotonPrescription;
  id[nt++] = INT;

  strcpy(tag[nt], "Verbose");
  addr[nt] = &Verbose;
  id[nt++] = INT;

  strcpy(tag[nt], "fescPrescription");
  addr[nt] = &fescPrescription;
  id[nt++] = INT;

  strcpy(tag[nt], "MH_min");
  addr[nt] = &MH_min;
  id[nt++] = DOUBLE;

  strcpy(tag[nt], "MH_max");
  addr[nt] = &MH_max;
  id[nt++] = DOUBLE;

  strcpy(tag[nt], "kappa");
  addr[nt] = &kappa;
  id[nt++] = DOUBLE;

  strcpy(tag[nt], "delta");
  addr[nt] = &delta; 
  id[nt++] = DOUBLE;

  strcpy(tag[nt], "fesc");
  addr[nt] = &fesc;
  id[nt++] = DOUBLE;

  strcpy(tag[nt], "HaloPartCut");
  addr[nt] = &HaloPartCut; 
  id[nt++] = INT;

  if((fd = fopen(fname, "r")))
  {
    while(!feof(fd))
    {
      *buf = 0;
      fgets(buf, 200, fd);
      if(sscanf(buf, "%s%s%s", buf1, buf2, buf3) < 2)
        continue;

      if(buf1[0] == '%' || buf1[0] == '-')
        continue;

      for(i = 0, j = -1; i < nt; i++)
        if(strcmp(buf1, tag[i]) == 0)
      {
        j = i;
        tag[i][0] = 0;
        break;
      }

      if(j >= 0)
      {
#ifdef MPI
        if(ThisTask == 0)
#endif
          printf("%35s\t%10s\n", buf1, buf2);

        switch (id[j])
        {
          case DOUBLE:
          *((double *) addr[j]) = atof(buf2);
          break;
          case STRING:
          strcpy(addr[j], buf2);
          break;
          case INT:
          *((int *) addr[j]) = atoi(buf2);
          break;
        }
      }
      else
      {
        printf("Error in file %s:   Tag '%s' not allowed or multiple defined.\n", fname, buf1);
        errorFlag = 1;
      }
    }
    fclose(fd);

    i = strlen(GridOutputDir);
    if(i > 0)
      if(GridOutputDir[i - 1] != '/')
      strcat(GridOutputDir, "/");

    i = strlen(GalaxiesInputDir);
    if(i > 0)
      if(GalaxiesInputDir[i - 1] != '/')
      strcat(GalaxiesInputDir, "/");


  }
  else
  {
    printf("Parameter file %s not found.\n", fname);
    errorFlag = 1;
  }

  for(i = 0; i < nt; i++)
  {
    if(*tag[i])
    {
      printf("Error. I miss a value for tag '%s' in parameter file '%s'.\n", tag[i], fname);
      errorFlag = 1;
    }
  }
	
	assert(!errorFlag);
	printf("\n");
	
	assert(LastSnapShotNr+1 > 0 && LastSnapShotNr+1 < ABSOLUTEMAXSNAPS);
	MAXSNAPS = LastSnapShotNr + 1;

	XASSERT(OutputMode == 0 || OutputMode == 1, "OutputMode must have a value of either 0 or 1.");
	XASSERT(LowSnap < HighSnap, "LowSnap must be less than HighSnap.");
	
	// read in the output snapshot list
	if(OutputMode == 1)
        {
	  printf("Selecting sequential snapshots from Snapshot %d to %d.\n", LowSnap, HighSnap);
	  for (i = HighSnap; i > LowSnap - 1; --i)
          {
            ListOutputGrid[HighSnap - i] = i;
          }
          NGrid = HighSnap - LowSnap + 1;
        }
        else if(OutputMode == 0 && NGrid == -1)
        {
	  NGrid = MAXSNAPS;
	  for (i = NGrid-1; i >= 0; --i)
		  ListOutputGrid[i] = i;
	  printf("All %d Snapshots selected for grid output: ", MAXSNAPS);
	}
	else 
	{
	  printf("%d Snapshots selected for grid output: ", NGrid);
	  fd = fopen(fname, "r");

	  done = 0;
	  while(!feof(fd) && !done)
	  {
	    fscanf(fd, "%s", buf);
	    if(strcmp(buf, "->") == 0)
	    {
	      for (i = 0; i < NGrid; ++i)
              {
		fscanf(fd, "%d", &ListOutputGrid[i]);
		printf("%d ", ListOutputGrid[i]);

		/*
		if (i == 0)
	          LastOutputSnap = ListOutputGrid[i];
		else 
	        {
	          if (ListOutputGrid[i] > LastOutputSnap)
			  LastOutputSnap = ListOutputGrid[i];
		}
		*/
	      }
		done = 1;
	    }
	  }

	  fclose(fd); 
	  printf("\n");
	}
}
