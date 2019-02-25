#ifndef BART_SRC_CONVERGENCE_FLUX_CHECKER_L1_THRESHOLD_H_
#define BART_SRC_CONVERGENCE_FLUX_CHECKER_L1_THRESHOLD_H_

#include "flux_checker_i.h"

namespace bart {

namespace convergence {

/*! \brief Checks for convergence between two provided fluxes. */

class FluxCheckerL1Threshold : public FluxCheckerI {
 public:
  FluxCheckerL1Threshold() = default;
  ~FluxCheckerL1Threshold() = default;
  bool isConverged(data::Flux &, data::Flux &) override;
  bool isConverged() const override { return converged_; }
  void SetThreshold(double value) { threshold_ = value; };
  double GetThreshold() const {return threshold_; };
 private:
  double threshold_ = 1e-6;
  bool converged_ = false;
};
  
} // namespace convergence

} // namespace bart

#endif // BART_SRC_CONVERGENCE_FLUX_CHECKER_L1_THRESHOLD_H_
