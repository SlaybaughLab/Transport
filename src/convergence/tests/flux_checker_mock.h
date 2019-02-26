#ifndef BART_SRC_CONVERGENCE_TESTS_FLUX_CHECKER_MOCK_H_
#define BART_SRC_CONVERGENCE_TESTS_FLUX_CHECKER_MOCK_H_

#include "../flux_checker_i.h"

#include "../../test_helpers/gmock_wrapper.h"

namespace bart {

namespace convergence {

class FluxCheckerMock : public FluxCheckerI {
 public:
  MOCK_METHOD2(CheckIfConverged, bool(data::Flux &, data::Flux &));
};

} // namespace convergence

} // namespace bart

#endif // BART_SRC_CONVERGENCE_TESTS_FLUX_CHECKER_MOCK_H_