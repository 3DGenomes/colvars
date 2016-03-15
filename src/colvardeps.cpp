#include "colvardeps.h"

void deps::provide(int feature_id) {
  feature_states[feature_id]->available = true;
}

int deps::require(int feature_id, bool dry_run /* default: false */) {  // Enable given feature
  // dry_run: fail silently, do not enable if available
  // flag is passed recursively to deps of this feature

  int res, i, j;
  bool ok;
  feature *f = features()[feature_id];
  feature_state *fs = feature_states[feature_id];

  cvm::log("~~~ " + description +
    (dry_run ? " testing " : " requiring ") +
    "\"" + f->description + "\" ~~~");

  if (fs->enabled) {
    // Do not try to solve deps if already enabled
    return COLVARS_OK;
  }

  if (!fs->available) {
    if (!dry_run) {
      cvm::error("Feature unavailable: \"" + f->description + "\" in " + description);
    }
    return COLVARS_ERROR;
  }

  // 1) enforce exclusions
  for (i=0; i<f->requires_exclude.size(); i++) {
    feature *g = features()[f->requires_exclude[i]];
    cvm::log(f->description + " requires exclude " + g->description);
    if (is_enabled(f->requires_exclude[i])) {
      if (!dry_run) {
          cvm::error("Features \"" + f->description + "\" and \""
          + g->description + "\" are incompatible in " + description);
      }
      return COLVARS_ERROR;
    }
  }

  // 2) solve internal deps (self)
  for (i=0; i<f->requires_self.size(); i++) {
    cvm::log(f->description + " requires self " + features()[f->requires_self[i]]->description);
    cvm::increase_depth();
    res = require(f->requires_self[i], dry_run);
    cvm::decrease_depth();
    if (res != COLVARS_OK) {
      if (!dry_run) {
          cvm::error("Unsatisfied dependency for \"" + f->description + "\" in " + description);
      }
      return res;
    }
  }

  // 3) solve internal alternate deps
  for (i=0; i<f->requires_alt.size(); i++) {

    // test if one is available; if yes, enable and exit w/ success
    ok = false;
    for (j=0; j<f->requires_alt[i].size(); j++) {
      int g = f->requires_alt[i][j];
      cvm::log(f->description + " requires alt " + features()[g]->description);
      cvm::increase_depth();
      res = require(g, true);  // see if available
      cvm::decrease_depth();
      if (res == COLVARS_OK) {
        ok = true;
        cvm::increase_depth();
        if (!dry_run) require(g); // Require again, for real
        cvm::decrease_depth();
        break;
      }
    }
    if (!ok) {
      if (!dry_run) {
        cvm::log("In " + description + ", no dependency satisfied among alternates:");
        for (j=0; j<f->requires_alt[i].size(); j++) {
          int g = f->requires_alt[i][j];
          cvm::log(cvm::to_str(j+1) + ". " + features()[g]->description);
        }
      }
      return COLVARS_ERROR;
    }
  }

  // 4) solve deps in children
  for (i=0; i<f->requires_children.size(); i++) {
    int g = f->requires_children[i];
    cvm::log("requires children " + features()[g]->description);
//     cvm::log("children " + cvm::to_str(g));
    for (j=0; j<children.size(); j++) {
//       cvm::log("child " +  children[j]->description);
      cvm::increase_depth();
      res = children[j]->require(g, dry_run);
      cvm::decrease_depth();
      if (res != COLVARS_OK) {
        if (!dry_run) {
          cvm::error("Unsatisfied dependency for \"" + f->description + "\" in " + description);
        }
        return res;
      }
    }
  }

  // Actually enable feature only once everything checks out
  if (!dry_run) fs->enabled = true;
  return COLVARS_OK;
}


//     disable() {
//
//       // we need refs to parents to walk up the deps tree!
//       // or refresh
//     }

   // Shorthand macros for describing dependencies
#define f_description(f, d) features()[f]->description = d;
#define f_req_self(f, g) features()[f]->requires_self.push_back(g);
// This macro ensures that exclusions are symmetric
#define f_req_exclude(f, g) features()[f]->requires_exclude.push_back(g); \
                            features()[g]->requires_exclude.push_back(f);
#define f_req_children(f, g) features()[f]->requires_children.push_back(g);
#define f_req_alt2(f, g, h) features()[f]->requires_alt.push_back(std::vector<int>(2));\
  features()[f]->requires_alt.back()[0] = g;                                           \
  features()[f]->requires_alt.back()[1] = h;


void deps::init_cvb_requires() {
  int i;
  if (features().size() == 0) {
    for (i = 0; i < f_cv_ntot; i++) {
      features().push_back(new feature);
    }
  }

  f_description(f_cvb_active, "active")
  f_req_children(f_cvb_active, f_cv_active)


  // Initialize feature_states for each instance
  for (i = 0; i < f_cvb_ntot; i++) {
    feature_states.push_back(new feature_state);
    // Most features are available, so we set them so
    // and list exceptions below
    feature_states.back()->available = true;
  }
}


void deps::init_cv_requires() {
  int i;
  if (features().size() == 0) {
    for (i = 0; i < f_cv_ntot; i++) {
      features().push_back(new feature);
    }

    f_description(f_cv_active, "active")
    f_req_children(f_cv_active, f_cvc_active)

    f_description(f_cv_gradient, "gradient")
    f_req_children(f_cv_gradient, f_cvc_gradient)

    f_description(f_cv_collect_gradient, "collect gradient")
    f_req_self(f_cv_collect_gradient, f_cv_gradient)

    f_description(f_cv_fdiff_velocity, "fdiff_velocity")

    // System force: either trivial (spring force) through extended Lagrangian, or calculated explicitly
    f_description(f_cv_system_force, "system force")
    f_req_alt2(f_cv_system_force, f_cv_extended_Lagrangian, f_cv_system_force_calc)
    f_req_self(f_cv_system_force, f_cv_Jacobian)

    // Deps for explicit system force calculation
    f_description(f_cv_system_force_calc, "system force calculation")
    f_req_self(f_cv_system_force_calc, f_cv_scalar)
    f_req_self(f_cv_system_force_calc, f_cv_linear)
    f_req_children(f_cv_system_force_calc, f_cvc_system_force)

    // Jacobian force: either trivial (zero) through extended Lagrangian, or calculated explicitly
    f_description(f_cv_Jacobian, "Jacobian derivative")
    f_req_alt2(f_cv_Jacobian, f_cv_extended_Lagrangian, f_cv_Jacobian_calc)

    // Deps for explicit Jacobian calculation
    f_description(f_cv_Jacobian_calc, "Jacobian derivative calculation")
    f_req_self(f_cv_Jacobian_calc, f_cv_scalar)
    f_req_self(f_cv_Jacobian_calc, f_cv_linear)
    f_req_children(f_cv_Jacobian_calc, f_cvc_Jacobian)

    f_description(f_cv_hide_Jacobian, "hide Jacobian force")
    f_req_self(f_cv_hide_Jacobian, f_cv_Jacobian_calc) // can only hide if explicitly calculated

    f_description(f_cv_extended_Lagrangian, "extended Lagrangian")

    f_description(f_cv_Langevin, "Langevin dynamics")

    f_description(f_cv_linear, "linear")

    f_description(f_cv_scalar, "scalar")

    f_description(f_cv_output_energy, "output energy")

    f_description(f_cv_output_value, "output value")

    f_description(f_cv_output_velocity, "output velocity")

    f_description(f_cv_output_applied_force, "output applied force")

    f_description(f_cv_output_system_force, "output system force")
    f_req_self(f_cv_output_system_force, f_cv_system_force)

    f_description(f_cv_lower_boundary, "lower boundary")
    f_req_self(f_cv_lower_boundary, f_cv_scalar)

    f_description(f_cv_upper_boundary, "upper boundary")
    f_req_self(f_cv_upper_boundary, f_cv_scalar)

    f_description(f_cv_grid, "grid")
    f_req_self(f_cv_grid, f_cv_lower_boundary)
    f_req_self(f_cv_grid, f_cv_upper_boundary)

    f_description(f_cv_lower_wall, "lower wall")
    f_req_self(f_cv_lower_wall, f_cv_lower_boundary)
    f_req_self(f_cv_lower_wall, f_cv_gradient)

    f_description(f_cv_upper_wall, "upper wall")
    f_req_self(f_cv_upper_wall, f_cv_upper_boundary)
    f_req_self(f_cv_upper_wall, f_cv_gradient)

    f_description(f_cv_runave, "running average")

    f_description(f_cv_corrfunc, "correlation function")

    // The features below are set programmatically
    f_description(f_cv_scripted, "scripted")
    f_description(f_cv_periodic, "periodic")
    f_description(f_cv_scalar, "scalar")
    f_description(f_cv_linear, "linear")
    f_description(f_cv_homogeneous, "homogeneous")
  }

  // Initialize feature_states for each instance
  for (i = 0; i < f_cv_ntot; i++) {
    feature_states.push_back(new feature_state);
    // Most features are available, so we set them so
    // and list exceptions below
    feature_states.back()->available = true;
  }

  // properties that may NOT be enabled as a dependency
  int unavailable_deps[] = {
    f_cv_extended_Lagrangian,
    f_cv_Langevin,
    f_cv_scripted,
    f_cv_periodic,
    f_cv_scalar,
    f_cv_linear,
    f_cv_homogeneous
  };
  for (i = 0; i < sizeof(unavailable_deps) / sizeof(unavailable_deps[0]); i++) {
    feature_states[unavailable_deps[i]]->available = false;
  }
}


void deps::init_cvc_requires() {
  int i;
  // Initialize static array once and for all
  if (features().size() == 0) {
    for (i = 0; i < deps::f_cvc_ntot; i++) {
      features().push_back(new feature);
    }

    f_description(f_cvc_active, "active")
//     The dependency below may become useful if we use dynamic atom groups
//     f_req_children(f_cvc_active, f_ag_active)

    f_description(f_cvc_scalar, "scalar")

    f_description(f_cvc_gradient, "gradient")

    f_description(f_cvc_system_force, "system force")
    f_req_self(f_cvc_system_force, f_cvc_inv_gradient)

    f_description(f_cvc_inv_gradient, "inverse gradient")
    f_req_self(f_cvc_inv_gradient, f_cvc_gradient)

    f_description(f_cvc_Jacobian, "Jacobian")
    f_req_self(f_cvc_Jacobian, f_cvc_inv_gradient)
  }

  // Initialize feature_states for each instance
  for (i = 0; i < deps::f_cvc_ntot; i++) {
    feature_states.push_back(new feature_state);
  }

  // Features that are implemented by all cvcs by default
  feature_states[f_cvc_active]->available = true;
  feature_states[f_cvc_gradient]->available = true;
  // implemented, subject to requirements
  feature_states[f_cvc_system_force]->available = true;
}


void deps::init_ag_requires() {
  int i;
  // Initialize static array once and for all
  if (features().size() == 0) {
    for (i = 0; i < f_ag_ntot; i++) {
      features().push_back(new feature);
    }

    f_description(f_ag_active, "active")
    f_description(f_ag_center, "translational fit")
    f_description(f_ag_rotate, "rotational fit")
    f_description(f_ag_ref_pos_group, "reference positions group")
    f_description(f_ag_fit_gradient_group, "fit gradient for main group")
    f_description(f_ag_fit_gradient_ref, "fit gradient for reference group")
    f_description(f_ag_atom_forces, "atomic forces")
  }

  // Initialize feature_states for each instance
  for (i = 0; i < deps::f_ag_ntot; i++) {
    feature_states.push_back(new feature_state);
  }

  // Features that are implemented by all cvcs by default
  feature_states[f_ag_active]->available = true;
}


void deps::print_state() {
  int i;
  cvm::log("Enabled features of " + description);
  for (i = 0; i<feature_states.size(); i++) {
    if (feature_states[i]->enabled)
      cvm::log("- " + features()[i]->description);
  }
  for (i=0; i<children.size(); i++) {
    cvm::log("* child " + cvm::to_str(i+1));
    cvm::increase_depth();
    children[i]->print_state();
    cvm::decrease_depth();
  }
}
