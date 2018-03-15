// KRATOS  ___|  |                   |                   |
//       \___ \  __|  __| |   |  __| __| |   |  __| _` | |
//             | |   |    |   | (    |   |   | |   (   | |
//       _____/ \__|_|   \__,_|\___|\__|\__,_|_|  \__,_|_| MECHANICS
//
//  License:             BSD License
//                                       license: StructuralMechanicsApplication/license.txt
//
//  Main authors:    Vicente Mataix Ferrandiz
// 

// System includes

// External includes

// Project includes
#include "includes/define.h"
#include "includes/define_python.h"
#include "processes/process.h"
#include "custom_python/add_mappers_to_python.h"
#include "custom_utilities/interface_preprocess.h"

namespace Kratos
{

namespace Python
{
using namespace pybind11;
void  AddCustomMappersToPython(pybind11::module& m)
{

    class_<InterfacePreprocessCondition, InterfacePreprocessCondition::Pointer>(m, "InterfacePreprocessCondition")
    .def(init<ModelPart&>())
    .def("GenerateInterfacePart2D",&InterfacePreprocessCondition::GenerateInterfacePart<2>)
    .def("GenerateInterfacePart3D",&InterfacePreprocessCondition::GenerateInterfacePart<3>)
    ;
}

}  // namespace Python.

} // Namespace Kratos

