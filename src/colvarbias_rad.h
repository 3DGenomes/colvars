// -*- c++ -*-

#ifndef COLVARBIAS_RAD_H
#define COLVARBIAS_RAD_H

#include "colvarbias.h"


/// \brief Restrained Average Dynamics (RAD)
/// Marinelli and Fiorin, Structure, 2018

class colvarbias_rad : public colvarbias {

public:

  colvarbias_rad(char const *key);
  virtual int init(std::string const &conf);
  virtual int init_centers(std::string const &conf);
  virtual int clear();
  virtual int update();
  virtual std::string const get_state_params() const;
  virtual int set_state_params(std::string const &state_conf);
  virtual int setup_output();
  virtual int write_traj_files();

  /// \brief Restraint centers
  std::vector<colvarvalue> colvar_centers;

  /// \brief Restraint original centers
  std::vector<colvarvalue> colvar_orig_centers;

  /// \brief Scale factors for the restraint
  std::vector<cvm::real> colvar_widths_c;

  /// Average of colvar_widths_c
  cvm::real colvar_cum_uscale;

  /// \brief Errors associated with the restraint centers
  std::vector<cvm::real> colvar_centers_errors;

  /// Types of averaging
  enum kernel_type_e {
    kt_none,
    kt_uniform,
    kt_inv_sqrt_time,
    kt_ntot
  };

  /// Specify which type of averaging is being done
  kernel_type_e kernel_type;

  /// Characteristic time for the kernel
  cvm::real kernel_coupling_time;

  /// Characteristic time for parameters optimization
  cvm::real params_coupling_time;

  /// experimental files for time dependent observables (e.g. DEER)
  std::vector<std::string> time_files;

  /// experimental times for time dependent variables
  std::vector<colvarvalue> colvar_times;

  /// experimental values for time dependent variables
  std::vector<colvarvalue> colvar_expval;

  /// Specify type of colvar for parameters optimization
  std::vector<std::string> colvar_types;

  /// number of variable of a certain type
  std::vector<int> numtypes;

  cvm::matrix2d<int> whichtypes;


  /// frequency at which to write output
  int rad_out_freq;

  /// write progressive output of RAD convergence
  std::ostream *rad_out_os;

  /// Name of output file
  inline std::string rad_out_file_name() const
  {
    return cvm::output_prefix() + "." + this->name + ".traj";
  }

  /// optimize parameters of the associated variables
  bool opt_params;

  /// write progressive output of CVC parameter optimization by RAD
  std::ostream *rad_param_os;

  /// Name of parameter optimization output file
  inline std::string rad_param_file_name() const
  {
    return cvm::output_prefix() + "." + this->name + ".params.traj";
  }

  /// equilibration steps
  int equil_steps;

  /// whether set starting parameters from initial equilibration
  bool set_params_equil;

  /// mdepth of deer
  std::vector<cvm::real> mdepth_deer;

  /// backgroud parameter of deer
  std::vector<cvm::real> alpha_deer;

  /// maximum and minimum values of error scale
  cvm::real colvar_maxerror_scale;

  /// average experimental error
  cvm::real colvar_cum_error;

  /// unique error scale
  cvm::real colvar_errors_scale;

  /// local deviation from experiment
  std::vector<colvarvalue> colvar_deviation;

  /// average deviation from experiment
  cvm::real colvar_aver_deviation;

  /// total dimensionality of all involved collective variables
  int colvar_size_tot;

  /// update unique error scale to fix chi to one
  bool fix_chi_square_one;

  /// update individual error scales to fix chi to one
  bool fix_chi_square_multi;

  /// use norm 1 to calculate deviation from experiment
  bool use_norm_1;

  /// Mean of the deviation according to the chosen kernel times number of samples
  std::vector<colvarvalue> colvar_total_deviations;

};

#endif
