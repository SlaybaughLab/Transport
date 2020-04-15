#include "system/system_functions.h"

#include "domain/tests/definition_mock.h"
#include "system/solution/tests/mpi_group_angular_solution_mock.h"
#include "system/solution/mpi_group_angular_solution.h"

#include "test_helpers/gmock_wrapper.h"
#include "test_helpers/dealii_test_domain.h"
#include "test_helpers/test_assertions.h"
#include "test_helpers/test_helper_functions.h"

#include "system/terms/term.h"
#include "system/moments/spherical_harmonic.h"

namespace  {

using namespace bart;
using ::testing::DoDefault;
using ::testing::Return, ::testing::ReturnRef;
using ::testing::WhenDynamicCastTo, ::testing::NotNull;

void StampMPIVector(bart::system::MPIVector &to_fill, double value = 2) {
  auto [local_begin, local_end] = to_fill.local_range();
  for (unsigned int i = local_begin; i < local_end; ++i)
    to_fill(i) += value;
  to_fill.compress(dealii::VectorOperation::add);
}

template <typename DimensionWrapper>
class SystemFunctionsSetUpMPIAngularSolutionTests :
    public ::testing::Test,
    public bart::testing::DealiiTestDomain<DimensionWrapper::value> {
 public:
  static constexpr int dim = DimensionWrapper::value;
  bart::system::solution::MPIGroupAngularSolutionMock mock_solution;
  domain::DefinitionMock<dim> mock_definition;

  const int total_angles_ = 3;
  std::map<bart::system::AngleIndex, bart::system::MPIVector> solution_map_;
  void SetUp() override;
};

template <typename DimensionWrapper>
void SystemFunctionsSetUpMPIAngularSolutionTests<DimensionWrapper>::SetUp() {
  this->SetUpDealii();
  ON_CALL(mock_solution, total_angles()).WillByDefault(Return(total_angles_));

  for (int i = 0; i < total_angles_; ++i) {
    bart::system::MPIVector mpi_vector;
    solution_map_.insert_or_assign(i, mpi_vector);
  }
  ON_CALL(mock_solution, solutions()).WillByDefault(ReturnRef(solution_map_));
  ON_CALL(mock_definition, locally_owned_dofs())
      .WillByDefault(Return(this->locally_owned_dofs_));
}

TYPED_TEST_SUITE(SystemFunctionsSetUpMPIAngularSolutionTests,
    bart::testing::AllDimensions);

TYPED_TEST(SystemFunctionsSetUpMPIAngularSolutionTests, BadNangles) {
  constexpr int dim = this->dim;

  std::array<int, 4> bad_total_angles{0, -1, 2, 4};

  for (const auto angle : bad_total_angles) {
    EXPECT_CALL(this->mock_solution, total_angles()).WillOnce(Return(angle));
    EXPECT_CALL(this->mock_solution, solutions()).WillOnce(DoDefault());
    EXPECT_ANY_THROW({
      bart::system::SetUpMPIAngularSolution<dim>(this->mock_solution,
                                                 this->mock_definition);
                     });
  }
}

TYPED_TEST(SystemFunctionsSetUpMPIAngularSolutionTests, SetUpDefaultValue) {
  constexpr int dim = this->dim;
  EXPECT_CALL(this->mock_solution, total_angles()).WillOnce(DoDefault());
  EXPECT_CALL(this->mock_solution, solutions()).WillOnce(DoDefault());
  EXPECT_CALL(this->mock_definition, locally_owned_dofs()).WillOnce(DoDefault());
  EXPECT_NO_THROW({
    bart::system::SetUpMPIAngularSolution<dim>(this->mock_solution,
                                               this->mock_definition);
  });
  bart::system::MPIVector expected_vector;
  expected_vector.reinit(this->locally_owned_dofs_, MPI_COMM_WORLD);
  StampMPIVector(expected_vector, 1.0);

  for (const auto& solution : this->solution_map_) {
    auto& mpi_vector = solution.second;
    ASSERT_GT(mpi_vector.size(), 0);
    EXPECT_TRUE(bart::test_helpers::CompareMPIVectors(expected_vector, mpi_vector));
  }
}

TYPED_TEST(SystemFunctionsSetUpMPIAngularSolutionTests, ProvidedValues) {
  constexpr int dim = this->dim;
  const double value_to_set = test_helpers::RandomDouble(0, 20);

  EXPECT_CALL(this->mock_solution, total_angles()).WillOnce(DoDefault());
  EXPECT_CALL(this->mock_solution, solutions()).WillOnce(DoDefault());
  EXPECT_CALL(this->mock_definition, locally_owned_dofs()).WillOnce(DoDefault());
  EXPECT_NO_THROW({
    bart::system::SetUpMPIAngularSolution<dim>(this->mock_solution,
                                               this->mock_definition,
                                               value_to_set);
                   });
  bart::system::MPIVector expected_vector;
  expected_vector.reinit(this->locally_owned_dofs_, MPI_COMM_WORLD);
  expected_vector = 0;
  StampMPIVector(expected_vector, value_to_set);

  for (const auto& solution : this->solution_map_) {
    auto& mpi_vector = solution.second;
    ASSERT_GT(mpi_vector.size(), 0);
    EXPECT_TRUE(bart::test_helpers::CompareMPIVectors(expected_vector, mpi_vector));
  }
}

// == InitializeSystem Tests ===================================================

class SystemInitializeSystemTest : public ::testing::Test {
 public:
  bart::system::System test_system;
};

TEST_F(SystemInitializeSystemTest, DefaultCall) {
  using VariableLinearTerms = system::terms::VariableLinearTerms;
  using ExpectedRHSType = bart::system::terms::MPILinearTerm;
  using ExpectedLHSType = bart::system::terms::MPIBilinearTerm;
  using ExpectedMomentsType = bart::system::moments::SphericalHarmonic;

  const int total_groups = bart::test_helpers::RandomDouble(1, 10);
  const int total_angles = total_groups + 1;
  std::unordered_set<VariableLinearTerms> source_terms{
      VariableLinearTerms::kScatteringSource,
      VariableLinearTerms::kFissionSource};

  system::InitializeSystem(test_system, total_groups, total_angles);

  EXPECT_EQ(test_system.total_angles, total_angles);
  EXPECT_EQ(test_system.total_groups, total_groups);
  EXPECT_EQ(test_system.k_effective, 1.0);
  ASSERT_THAT(test_system.right_hand_side_ptr_.get(),
              WhenDynamicCastTo<ExpectedRHSType *>(NotNull()));
  ASSERT_THAT(test_system.left_hand_side_ptr_.get(),
              WhenDynamicCastTo<ExpectedLHSType *>(NotNull()));
  EXPECT_EQ(test_system.right_hand_side_ptr_->GetVariableTerms(), source_terms);
  ASSERT_THAT(test_system.current_moments.get(),
              WhenDynamicCastTo<ExpectedMomentsType *>(NotNull()));
  ASSERT_THAT(test_system.previous_moments.get(),
              WhenDynamicCastTo<ExpectedMomentsType *>(NotNull()));
  EXPECT_EQ(test_system.current_moments->moments().size(), total_groups);
  EXPECT_EQ(test_system.previous_moments->moments().size(), total_groups);
}

TEST_F(SystemInitializeSystemTest, NonEigenvalueProblem) {
  using VariableLinearTerms = system::terms::VariableLinearTerms;
  using ExpectedRHSType = bart::system::terms::MPILinearTerm;
  using ExpectedLHSType = bart::system::terms::MPIBilinearTerm;
  using ExpectedMomentsType = bart::system::moments::SphericalHarmonic;

  const int total_groups = bart::test_helpers::RandomDouble(1, 10);
  const int total_angles = total_groups + 1;
  std::unordered_set<VariableLinearTerms> source_terms{
      VariableLinearTerms::kScatteringSource};

  system::InitializeSystem(test_system, total_groups, total_angles, false);

  EXPECT_EQ(test_system.total_angles, total_angles);
  EXPECT_EQ(test_system.total_groups, total_groups);
  EXPECT_EQ(test_system.k_effective, std::nullopt);
  ASSERT_THAT(test_system.right_hand_side_ptr_.get(),
              WhenDynamicCastTo<ExpectedRHSType *>(NotNull()));
  ASSERT_THAT(test_system.left_hand_side_ptr_.get(),
              WhenDynamicCastTo<ExpectedLHSType *>(NotNull()));
  EXPECT_EQ(test_system.right_hand_side_ptr_->GetVariableTerms(), source_terms);
  ASSERT_THAT(test_system.current_moments.get(),
              WhenDynamicCastTo<ExpectedMomentsType *>(NotNull()));
  ASSERT_THAT(test_system.previous_moments.get(),
              WhenDynamicCastTo<ExpectedMomentsType *>(NotNull()));
  EXPECT_EQ(test_system.current_moments->moments().size(), total_groups);
  EXPECT_EQ(test_system.previous_moments->moments().size(), total_groups);
}

TEST_F(SystemInitializeSystemTest, ErrorOnSecondCall) {
  using VariableLinearTerms = system::terms::VariableLinearTerms;
  using ExpectedRHSType = bart::system::terms::MPILinearTerm;
  using ExpectedLHSType = bart::system::terms::MPIBilinearTerm;
  using ExpectedMomentsType = bart::system::moments::SphericalHarmonic;

  const int total_groups = bart::test_helpers::RandomDouble(1, 10);
  const int total_angles = total_groups + 1;
  std::unordered_set<VariableLinearTerms> source_terms{
      VariableLinearTerms::kScatteringSource,
      VariableLinearTerms::kFissionSource};

  system::InitializeSystem(test_system, total_groups, total_angles);
  EXPECT_ANY_THROW(bart::system::InitializeSystem(test_system, total_groups,
                                                  total_angles));
}

// ==

template <typename DimensionWrapper>
class SystemSetUpTests : public ::testing::Test,
                         bart::testing::DealiiTestDomain<DimensionWrapper::value> {
 public:
  static constexpr int dim = DimensionWrapper::value;
  using DomainType = domain::DefinitionMock<dim>;

  bart::system::System test_system;
  std::shared_ptr<domain::DefinitionI<dim>> definition_ptr;
  DomainType* domain_mock_obs_ptr_;
  void SetUp() override;
};

template <typename DimensionWrapper>
void SystemSetUpTests<DimensionWrapper>::SetUp() {
  this->SetUpDealii();
  definition_ptr = std::make_shared<DomainType>();
  domain_mock_obs_ptr_ = dynamic_cast<DomainType*>(definition_ptr.get());

  auto system_matrix_ptr = std::make_shared<bart::system::MPISparseMatrix>();
  system_matrix_ptr->reinit(this->matrix_1);
  auto system_vector_ptr = std::make_shared<bart::system::MPIVector>();
  system_vector_ptr->reinit(this->vector_1);

  ON_CALL(*domain_mock_obs_ptr_, MakeSystemMatrix())
      .WillByDefault(Return(system_matrix_ptr));
  ON_CALL(*domain_mock_obs_ptr_, MakeSystemVector())
      .WillByDefault(Return(system_vector_ptr));
}

TYPED_TEST_SUITE(SystemSetUpTests, bart::testing::AllDimensions);

TYPED_TEST(SystemSetUpTests, InitializeSystem) {
  constexpr int dim = this->dim;
  auto& test_system = this->test_system;
//  To be moved to "Set up terms" test
//  EXPECT_CALL(*this->domain_mock_obs_ptr_, MakeSystemMatrix())
//      .Times(total_groups * total_angles)
//      .WillRepeatedly(DoDefault());
//  EXPECT_CALL(*this->domain_mock_obs_ptr_, MakeSystemVector())
//      .Times(3 * total_groups * total_angles)
//      .WillRepeatedly(DoDefault());
//
//  for (int group = 0; group < total_groups; ++group) {
//    for (int angle = 0; angle < total_angles; ++angle) {
//      bart::system::Index index{group, angle};
//      EXPECT_THAT(test_system.left_hand_side_ptr_->GetFixedTermPtr(index),
//                  NotNull());
//      EXPECT_THAT(test_system.right_hand_side_ptr_->GetFixedTermPtr(index),
//                  NotNull());
//      for (auto term : source_terms) {
//        EXPECT_THAT(
//            test_system.right_hand_side_ptr_->GetVariableTermPtr(index, term),
//            NotNull());
//      }
//    }
//  }
}


} // namespace