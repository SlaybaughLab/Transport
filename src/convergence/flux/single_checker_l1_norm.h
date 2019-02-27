#ifndef BART_SRC_CONVERGENCE_FLUX_SINGLE_CHECKER_L1_NORM_H_
#define BART_SRC_CONVERGENCE_FLUX_SINGLE_CHECKER_L1_NORM_H_

#include "single_checker.h"

namespace bart {

namespace convergence {

namespace flux {

/*! \brief Checks for convergence between two fluxes using the percentage
 * change in the L1 norms */

class SingleCheckerL1Norm : public SingleChecker {
 public:
  SingleCheckerL1Norm() = default;
  ~SingleCheckerL1Norm() = default;
  bool CheckIfConverged(data::Flux &current_iteration,
                        data::Flux &previous_iteration) override;
};

} // namespace flux
  
} // namespace convergence

} // namespace bart

#endif // BART_SRC_CONVERGENCE_FLUX_SINGLE_CHECKER_L1_NORM_H_
