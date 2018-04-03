#include "../../src/mesh/mesh_generator.h"
#include "../test_utilities.h"

#include <deal.II/base/types.h>

template <int dim>
void SetupParameters (dealii::ParameterHandler &prm) {
  prm.declare_entry ("is mesh generated by deal.II", "true",
                     dealii::Patterns::Bool(), "");
  prm.declare_entry ("have reflective BC", "true",
                     dealii::Patterns::Bool(), "");
  prm.declare_entry ("uniform refinements", "1",
                     dealii::Patterns::Integer(), "");
  prm.declare_entry ("x, y, z max values of boundary locations", "1.0,1.0,1.0",
                     dealii::Patterns::List (dealii::Patterns::Double ()), "");
  prm.declare_entry ("number of cells for x, y, z directions", "2,2,2",
                     dealii::Patterns::List (dealii::Patterns::Integer ()), "");
  prm.declare_entry ("reflective boundary names", "xmin, ymax",
                     dealii::Patterns::List(dealii::Patterns::Anything ()), "");
  prm.enter_subsection ("material ID map");
  {
    std::string id_fname = SOURCE_DIR + std::string ("/matid.homogeneous.")
        + std::to_string (dim) + std::string ("d");
    prm.declare_entry ("material id file name", id_fname,
                       dealii::Patterns::FileName(), "file name for material id map");
  }
  prm.leave_subsection ();
}

template <int dim>
void Test (dealii::ParameterHandler &prm) {
  dealii::deallog.push (dealii::Utilities::int_to_string(dim)+"D");

  // triangulation for the grid in the scope of the test function
  dealii::Triangulation<dim> tria;

  // make grid
  std::unique_ptr<MeshGenerator<dim>> msh_ptr =
      std::unique_ptr<MeshGenerator<dim>> (new MeshGenerator<dim>(prm));
  msh_ptr->MakeGrid (tria);

  AssertThrow (tria.n_active_cells()==std::pow(2, 2*dim),
               dealii::ExcInternalError());

  tria.refine_global (2);

  AssertThrow (tria.n_active_cells()==std::pow(2, 4*dim),
               dealii::ExcInternalError());

  dealii::deallog << "Global refinements check OK." << std::endl;

  for (typename dealii::Triangulation<dim>::active_cell_iterator
       cell=tria.begin_active(); cell!=tria.end(); ++cell)
    if (cell->is_locally_owned())
      AssertThrow (cell->material_id()==111, dealii::ExcInternalError());

  dealii::deallog << "Material ID check OK." << std::endl;

  dealii::deallog.pop ();
}

int main () {
  // initialize log and declare ParameterHandler object
  testing::InitLog ();
  dealii::ParameterHandler prm;

  // parameter processing
  SetupParameters<1> (prm);

  // testing 2D
  Test<1> (prm);

  // clearing prm so new parameters can be set for other dimensions if needed
  prm.clear ();
  testing::deallogstream << std::endl;

  // 2D testing section
  SetupParameters<2> (prm);
  Test<2> (prm);
  prm.clear ();
  testing::deallogstream << std::endl;

  // 3D testing section
  SetupParameters<3> (prm);
  Test<3> (prm);
  prm.clear ();

  return 0;
}
