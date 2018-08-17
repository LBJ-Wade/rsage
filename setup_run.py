"""
Creates directories and updates ini files with the correct file names.

The user specifies the base output directory and the .ini files for SAGE and
cifog.

The script then creates directories such as '<BaseDir>/galaxies'.  It then
reads the escape fraction prescription and constants, and determines what the
eventual name for the output nion files will be. Finally it updates the .ini
files with these nion filepaths.
"""

#!/usr/bin/env python
from __future__ import print_function

import numpy as np
import sys
import os
from shutil import copyfile
import subprocess

sys.path.append('./output/')
import ReadScripts
import AllVars


def create_directories(run_directory):
    """
    Creates the directories to house all the ``RSAGE`` outputs.   
 
    Parameters
    ----------

    run_directory: String 
        Path to the base ``RSAGE`` directory. 

    Returns
    ----------
    
    None.
    """

    # First create the base directory.
    base_dir = run_directory 
    if not os.path.exists(base_dir):
        os.makedirs(base_dir)
        print("Created directory {0}".format(base_dir))

    # Directory that will contain all the SAGE output.
    gal_dir = "{0}/galaxies".format(base_dir)
    if not os.path.exists(gal_dir):
        os.makedirs(gal_dir)
        print("Created directory {0}".format(gal_dir))

    # Directory that will contain all the grids. 
    grids_dir = "{0}/grids".format(base_dir)
    if not os.path.exists(grids_dir):
        os.makedirs(grids_dir)
        print("Created directory {0}".format(grids_dir))
       
        # If the grids directory didn't exist, there's no way these will.

        dirs = ["grids/nion", "grids/cifog",
                "grids/cifog/reionization_modifiers",
                "ini_files", "slurm_files", "log_files"]
        for directory in dirs:
            dir_ = "{0}/{1}".format(base_dir, directory)
            os.makedirs(dir_)
            print("Created directory {0}".format(dir_))

   
def update_ini_files(base_SAGE_ini, base_cifog_ini,
                     SAGE_fields_update, cifog_fields_update,
                     run_directory):
    """
    Rewrites the ini files to point to the correct output files.  

    Parameters
    ----------

    args: Dictionary.  Required.
        Dictionary containing the input parameters specified at runtime.
    
    Returns
    ----------

    None.
    """

    SAGE_params, SAGE_params_names = ReadScripts.read_SAGE_ini(base_SAGE_ini)
    cifog_params, cifog_params_names, cifog_headers = ReadScripts.read_cifog_ini(base_cifog_ini)

    # These are paths and don't depend on `FileNameGalaxies`. 
    SAGE_params["OutputDir"] = "{0}/galaxies".format(run_directory)
    SAGE_params["GridOutputDir"] = "{0}/grids/nion".format(run_directory)
    SAGE_params["PhotoionDir"] = "{0}/grids/cifog".format(run_directory)

    # Now go through the parameters and update them.
    for name in SAGE_fields_update:
        SAGE_params[name] = SAGE_fields_update[name] 

    for name in cifog_fields_update:
        cifog_params[name] = cifog_fields_update[name] 

    # The unique identifier amongst each run will be `FileNameGalaxies`. 
    prefix_tag = SAGE_params["FileNameGalaxies"][0]

    SAGE_params["PhotoionName"] = "{0}_photHI".format(prefix_tag)
    SAGE_params["ReionRedshiftName"] = "{0}_reionization_redshift" \
                                       .format(prefix_tag)

    nion_fname = get_nion_fname(SAGE_params) 
    cifog_params["inputNionFile"] = "{0}/grids/nion/{1}" \
                                    .format(run_directory, nion_fname)
    cifog_params["output_XHII_file"] = "{0}/grids/cifog/{1}_XHII" \
                                       .format(run_directory,
                                               prefix_tag)
    cifog_params["output_photHI_file"] = "{0}/grids/cifog/{1}_photHI" \
                                         .format(run_directory,
                                                 prefix_tag)
    cifog_params["output_restart_file"] = "{0}/grids/cifog/{1}_restart" \
                                          .format(run_directory,
                                                  prefix_tag)

    # Write out the new ini files, using `FileNameGalaxies` as the tag.
    SAGE_fname = "{0}/ini_files/{1}_SAGE.ini".format(run_directory,
                                                     prefix_tag) 

    cifog_fname = "{0}/ini_files/{1}_cifog.ini".format(run_directory,
                                                       prefix_tag) 

    with open (SAGE_fname, "w+") as f:
        for name in SAGE_params_names:
            string = "{0} {1}\n".format(name, SAGE_params[name][0])
            f.write(string)

    with open (cifog_fname, "w+") as f:
        for name in cifog_params_names:
            if name in cifog_headers:
                header_string = "{0}".format(cifog_headers[name])
                f.write(header_string)
            string = "{0} = {1}\n".format(name, cifog_params[name][0])
            f.write(string)

    return SAGE_fname, cifog_fname


def get_nion_fname(SAGE_params):
    """
    Using the fescPrescription specified in the SAGE.ini file, determines the
    name of the output nion files. 

    NOTE: fescPrescription == 1 is deprecated and is not allowed. If the .ini
    file uses this fescPrescription a ValueError will be raised.

    Parameters
    ----------

    SAGE_params: Dictionary.  Required.
        Dictionary containing the SAGE .ini file parameters. 
    
    Returns
    ----------

    nion_fname: String. Required.
        Base name of the eventual output nion files. 
    """

    fesc_prescription = SAGE_params["fescPrescription"]
    
    if fesc_prescription == 0:
        nion_fname = "{0}_fesc{1:.2f}_HaloPartCut{2}_nionHI" \
                      .format(SAGE_params["FileNameGalaxies"][0],
                              SAGE_params["fesc"][0],
                              SAGE_params["HaloPartCut"][0])

    elif fesc_prescription == 1:
        print("Using fesc_prescription of 1 is deprecated.")
        raise ValueError 

    elif fesc_prescription == 2:
        alpha, beta = determine_fesc_constants(SAGE_params)              
        nion_fname = "{0}_MH_{1:.3e}_{2:.2f}_{3:.3e}_{4:.2f}_HaloPartCut{5}_nionHI" \
                     .format(SAGE_params["FileNameGalaxies"][0],
                             SAGE_params["MH_low"][0],
                             SAGE_params["fesc_low"][0],
                             SAGE_params["MH_high"][0],
                             SAGE_params["fesc_high"][0],
                             SAGE_params["HaloPartCut"][0])

    elif fesc_prescription == 3:
        nion_fname = "{0}_ejected_{1:.3f}_{2:.3f}_HaloPartCut{3}_nionHI" \
                     .format(SAGE_params["FileNameGalaxies"][0],
                             SAGE_params["alpha"][0],
                             SAGE_params["beta"][0],
                             SAGE_params["HaloPartCut"][0])

    elif fesc_prescription == 4:
        nion_fname = "{0}_quasar_{1:.2f}_{2:.2f}_{3:.2f}_HaloPartCut{4}_nionHI" \
                     .format(SAGE_params["FileNameGalaxies"][0],
                             SAGE_params["quasar_baseline"][0],
                             SAGE_params["quasar_boosted"][0],
                             SAGE_params["N_dyntime"][0],
                             SAGE_params["HaloPartCut"][0])

    elif fesc_prescription == 5 or fesc_prescription == 6:
        nion_fname = "{0}_AnneMH_{1:.3e}_{2:.2f}_{3:.3e}_{4:.2f}_HaloPartCut{5}_nionHI" \
                     .format(SAGE_params["FileNameGalaxies"][0],
                             SAGE_params["MH_low"][0],
                             SAGE_params["fesc_low"][0],
                             SAGE_params["MH_high"][0],
                             SAGE_params["fesc_high"][0],
                             SAGE_params["HaloPartCut"][0])

    elif fesc_prescription == 7:
        nion_fname = "{0}_ejectedpower_{1:.3e}_{2:.2f}_{3:.3e}_{4:.2f}_HaloPartCut{5}_nionHI" \
                     .format(SAGE_params["FileNameGalaxies"][0],
                             SAGE_params["MH_low"][0],
                             SAGE_params["fesc_low"][0],
                             SAGE_params["MH_high"][0],
                             SAGE_params["fesc_high"][0],
                             SAGE_params["HaloPartCut"][0])
        
    elif fesc_prescription == 8:
        nion_fname = "{0}_mstar_{1:.3e}_{2:.3e}_{3:.2f}_{4:.2f}_HaloPartCut{5}_nionHI" \
                     .format(SAGE_params["FileNameGalaxies"][0],
                             SAGE_params["fesc_Mstar_low"][0],
                             SAGE_params["fesc_Mstar_high"][0],
                             SAGE_params["fesc_Mstar"][0],
                             SAGE_params["fesc_not_Mstar"][0],
                             SAGE_params["HaloPartCut"][0])

    elif fesc_prescription == 9:
        nion_fname = "{0}_ejectedSN_{1:.3f}_{2:.3f}_HaloPartCut{3}_nionHI" \
                     .format(SAGE_params["FileNameGalaxies"][0],
                             SAGE_params["alpha"][0],
                             SAGE_params["beta"][0],
                             SAGE_params["HaloPartCut"][0])

    elif fesc_prescription == 10:
        nion_fname = "{0}_ejectedQSO_{1:.3f}_{2:.3f}_HaloPartCut{3}_nionHI" \
                     .format(SAGE_params["FileNameGalaxies"][0],
                             SAGE_params["alpha"][0],
                             SAGE_params["beta"][0],
                             SAGE_params["HaloPartCut"][0])

    elif fesc_prescription == 11:
        nion_fname = "{0}_SFR_{1:.3f}_{2:.3f}_HaloPartCut{3}_nionHI" \
                     .format(SAGE_params["FileNameGalaxies"][0],
                             SAGE_params["alpha"][0],
                             SAGE_params["beta"][0],
                             SAGE_params["HaloPartCut"][0])

    else:
        print("Select a valid fescPrescription (0 to 7 inclusive).")
        raise ValueError

    return nion_fname


def determine_fesc_constants(SAGE_params):
    """
    If the fescPrescription depends on Halo mass, the functional form is fesc =
    alpha*MH^(beta).  This function determines the values of alpha and beta
    depending upon the fixed points specified in the SAGE parameter file.

    If the fixed points have had their halo mass specified in log units a
    ValueError will be raised.

    Parameters
    ----------

    SAGE_params: Dictionary.  Required.
        Dictionary containing the SAGE .ini file parameters. 
    
    Returns
    ----------

    alpha, beta: Floats. Required.
        Constants for fesc equation. 
    """

    # The SAGE ini file specifies two fixed points.
    fesc_high = SAGE_params["fesc_high"][0]
    MH_high = SAGE_params["MH_low"][0]

    fesc_low = SAGE_params["fesc_low"][0]    
    MH_low = SAGE_params["MH_high"][0]

    # The values for halo mass should be in non-log units. Do a quick check.
    if (MH_high < 1e6 or MH_low < 1e6):
        print("If using fescPrescription == 2 (fesc depends on halo mass) the "
              "fixed points need to have their halo mass specified in Msun, NOT "
              "LOG MSUN.")
        raise ValueError 
    

    log_A = (np.log10(fesc_high) - np.log10(fesc_low)*np.log10(MH_high) / np.log10(MH_low)) \
            * pow(1 - np.log10(MH_high) / np.log10(MH_low), -1)

    B = (np.log10(fesc_low) - log_A) / np.log10(MH_low);
    A = pow(10, log_A);

    alpha = A;
    beta = B;

    return alpha, beta


def make_ini_files(base_SAGE_ini, base_cifog_ini, 
                   SAGE_fields_update, cifog_fields_update,
                   run_directory):

    SAGE_ini_names = []
    cifog_ini_names = []

    # Now for each run, create a unique dictionary containing the fields for 
    # this run, update the ini files then create all the output directories. 
    for run_number in range(len(run_directory)):

        create_directories(run_directory[run_number])

        thisrun_SAGE_update = {}
        for name in SAGE_fields_update.keys():
            thisrun_SAGE_update[name] = SAGE_fields_update[name][run_number]

        thisrun_cifog_update = {}
        for name in cifog_fields_update.keys():
            thisrun_cifog_update[name] = cifog_fields_update[name][run_number]

        SAGE_fname, cifog_fname = update_ini_files(base_SAGE_ini, base_cifog_ini,
                                                   thisrun_SAGE_update, thisrun_cifog_update,
                                                   run_directory[run_number])        

        SAGE_ini_names.append(SAGE_fname)
        cifog_ini_names.append(cifog_fname)

    return SAGE_ini_names, cifog_ini_names


def make_slurm_files(base_slurm_file, SAGE_ini_names, cifog_ini_names, 
                     run_directory, Nproc): 

    slurm_names = []

    for run_number in range(len(SAGE_ini_names)):
        
        SAGE_params, SAGE_params_names = ReadScripts.read_SAGE_ini(SAGE_ini_names[run_number])
        run_name = SAGE_params["FileNameGalaxies"][0]

        slurm_fname = "{0}/slurm_files/{1}.slurm".format(run_directory[run_number],
                                                         run_name) 

        tmp_slurm_fname = "{0}.tmp".format(base_slurm_file)
        copyfile(base_slurm_file, tmp_slurm_fname)

        # Now want to replace lines in the slurm file. Set up the strings. 
        job_name = "#SBATCH --job-name={0}".format(run_name) 
        ntask = "#SBATCH --ntasks={0}".format(Nproc)
        NUMPROC = "NUMPROC={0}".format(Nproc)
        SAGE_ini = 'SAGE_ini="{0}"'.format(SAGE_ini_names[run_number])
        cifog_ini = 'cifog_ini="{0}"'.format(cifog_ini_names[run_number])
        run_prefix = 'run_prefix="{0}"'.format(run_name) 
        path_to_log = 'path_to_log="{0}/log_files/{1}.log"'.format(run_directory[run_number], run_name)

        # Now replace strings.
        line_numbers = [2, 4, 17, 19, 20, 24, 25]  
        string_names = [job_name, ntask, NUMPROC, SAGE_ini, cifog_ini, 
                        run_prefix, path_to_log]
        for line, name in zip(line_numbers, string_names): 
            command = "sed -i '{0}s@.*@{1}@' {2} ".format(line, name,
                                                          tmp_slurm_fname)
            subprocess.call(command, shell=True)

        command = "mv {0} {1}".format(tmp_slurm_fname, slurm_fname)
        subprocess.call(command, shell=True)
        print("Created {0}".format(slurm_fname))

        slurm_names.append(slurm_fname)

    return slurm_names


def submit_slurm_jobs(slurm_names):

    for slurm_fname in slurm_names:

        command = "sbatch {0}".format(slurm_fname)
        print(command)
        #subprocess.call(command, shell=True)
         
 
if __name__ == '__main__':

    fescPrescription = [3, 3, 3]
    alpha = [0.4, 0.6, 0.8]
    beta = [0.05, 0.05, 0.05]
    FileNameGalaxies = ["test_alpha0.4_beta0.05",
                        "test_alpha0.6_beta0.05",
                        "test_alpha0.8_beta0.05"] 

    base_SAGE_ini = "/home/jseiler/tmp/rsage/ini_files/kali_SAGE.ini"
    base_cifog_ini = "/home/jseiler/tmp/rsage/ini_files/kali_cifog.ini"
    base_slurm_file = "/home/jseiler/tmp/rsage/run_rsage.slurm"

    SAGE_fields_update = {"alpha" : alpha,
                          "beta" : beta,
                          "FileNameGalaxies" : FileNameGalaxies}

    cifog_fields_update = {}

    run_directory = np.full(len(FileNameGalaxies),
                            "/fred/oz004/jseiler/kali/self_consistent_output/new_rsage_test")

    Nproc = 32

    SAGE_ini_names, cifog_ini_names = make_ini_files(base_SAGE_ini, base_cifog_ini, 
                                                     SAGE_fields_update, cifog_fields_update,
                                                     run_directory)

    slurm_names = make_slurm_files(base_slurm_file, SAGE_ini_names, 
                                    cifog_ini_names, run_directory, Nproc)

    submit_slurm_jobs(slurm_names)
