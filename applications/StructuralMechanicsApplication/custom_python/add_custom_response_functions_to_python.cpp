// KRATOS  ___|  |                   |                   |
//       \___ \  __|  __| |   |  __| __| |   |  __| _` | |
//             | |   |    |   | (    |   |   | |   (   | |
//       _____/ \__|_|   \__,_|\___|\__|\__,_|_|  \__,_|_| MECHANICS
//
//  License:        BSD License
//	                license: structural_mechanics_application/license.txt
//
//  Main authors:    Armin Geiser
//

// System includes

// External includes
#include <pybind11/stl.h>

// Project includes
#include "includes/define_python.h"
#include "custom_python/add_custom_response_functions_to_python.h"

//Utilities
#include "custom_response_functions/custom_utilities/finite_differences_utilities.h" //M.Fusseder TODO: maybe remove this (used only for controlling results)

#include "custom_response_functions/adjoint_processes/replace_elements_and_conditions_for_adjoint_problem_process.h"

//Response Functions
#include "custom_response_functions/response_utilities/adjoint_structural_response_function.h"
#include "custom_response_functions/response_utilities/adjoint_local_stress_response_function.h"
#include "custom_response_functions/response_utilities/adjoint_nodal_displacement_response_function.h"
#include "custom_response_functions/response_utilities/adjoint_strain_energy_response_function.h"
#include "custom_response_functions/response_utilities/strain_energy_response_function_utility.h"
#include "custom_response_functions/response_utilities/mass_response_function_utility.h"
#include "custom_response_functions/response_utilities/eigenfrequency_response_function_utility.h"


namespace Kratos
{
namespace Python
{

using namespace pybind11;

void  AddCustomResponseFunctionUtilitiesToPython(pybind11::module& m)
{

    /// Processes
    class_<ReplaceElementsAndConditionsForAdjointProblemProcess, Process >
        (m, "ReplaceElementsAndConditionsForAdjointProblemProcess")
        .def(init<ModelPart&, Parameters>());

    //Response Functions
    class_<AdjointStructuralResponseFunction, AdjointStructuralResponseFunction::Pointer>
      (m, "AdjointStructuralResponseFunction")
      .def(init<ModelPart&, Parameters&>())
      .def("Initialize", &AdjointStructuralResponseFunction::Initialize)
      .def("FinalizeSolutionStep", &AdjointStructuralResponseFunction::FinalizeSolutionStep)
      .def("CalculateValue", &AdjointStructuralResponseFunction::CalculateValue);

    class_<AdjointLocalStressResponseFunction, AdjointLocalStressResponseFunction::Pointer, AdjointStructuralResponseFunction>
      (m, "AdjointLocalStressResponseFunction")
      .def(init<ModelPart&, Parameters&>());

    class_<AdjointNodalDisplacementResponseFunction, AdjointNodalDisplacementResponseFunction::Pointer, AdjointStructuralResponseFunction>
      (m, "AdjointNodalDisplacementResponseFunction")
      .def(init<ModelPart&, Parameters&>());

    class_<AdjointStrainEnergyResponseFunction, AdjointStrainEnergyResponseFunction::Pointer, AdjointStructuralResponseFunction>
      (m, "AdjointStrainEnergyResponseFunction")
      .def(init<ModelPart&, Parameters&>());


    //For global finite differences
    class_<FiniteDifferencesUtilities>(m, "FiniteDifferencesUtilities")
        .def(init< >())
        .def("SetDesignVariable", &FiniteDifferencesUtilities::SetDesignVariable)
        .def("GetDesignVariable", &FiniteDifferencesUtilities::GetDesignVariable)
        .def("SetDerivedObject", &FiniteDifferencesUtilities::SetDerivedObject)
        .def("GetDerivedObject", &FiniteDifferencesUtilities::GetDerivedObject)
        .def("DisturbElementDesignVariable", &FiniteDifferencesUtilities::DisturbElementDesignVariable)
        .def("UndisturbElementDesignVariable", &FiniteDifferencesUtilities::UndisturbElementDesignVariable)
        .def("GetStressResultantBeam", &FiniteDifferencesUtilities::GetStressResultantBeam)
        .def("GetStressResultantShell", &FiniteDifferencesUtilities::GetStressResultantShell)
        .def("GetNodalDisplacement", &FiniteDifferencesUtilities::GetNodalDisplacement)
        .def("GetStrainEnergy", &FiniteDifferencesUtilities::GetStrainEnergy);

    // Response Functions
    class_<StrainEnergyResponseFunctionUtility, StrainEnergyResponseFunctionUtility::Pointer >
      (m, "StrainEnergyResponseFunctionUtility")
      .def(init<ModelPart&, Parameters>())
      .def("Initialize", &StrainEnergyResponseFunctionUtility::Initialize)
      .def("CalculateValue", &StrainEnergyResponseFunctionUtility::CalculateValue)
      .def("CalculateGradient", &StrainEnergyResponseFunctionUtility::CalculateGradient);

    class_<MassResponseFunctionUtility, MassResponseFunctionUtility::Pointer >
      (m, "MassResponseFunctionUtility")
      .def(init<ModelPart&, Parameters>())
      .def("Initialize", &MassResponseFunctionUtility::Initialize)
      .def("CalculateValue", &MassResponseFunctionUtility::CalculateValue)
      .def("CalculateGradient", &MassResponseFunctionUtility::CalculateGradient);

    class_<EigenfrequencyResponseFunctionUtility, EigenfrequencyResponseFunctionUtility::Pointer >
      (m, "EigenfrequencyResponseFunctionUtility")
      .def(init<ModelPart&, Parameters>())
      .def("Initialize", &EigenfrequencyResponseFunctionUtility::Initialize)
      .def("CalculateValue", &EigenfrequencyResponseFunctionUtility::CalculateValue)
      .def("CalculateGradient", &EigenfrequencyResponseFunctionUtility::CalculateGradient);
}

}  // namespace Python.

} // Namespace Kratos

