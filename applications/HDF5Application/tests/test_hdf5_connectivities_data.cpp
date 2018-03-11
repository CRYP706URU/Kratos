//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:		 BSD License
//					 Kratos default license: kratos/license.txt
//
//  Main authors:    Michael Andre, https://github.com/msandre
//

// System includes

// External includes

// Project includes
#include "testing/testing.h"
#include "includes/model_part.h"

// Application includes
#include "tests/test_utils.h"
#include "custom_io/hdf5_connectivities_data.h"
#include "custom_utilities/factor_elements_and_conditions_utility.h"

namespace Kratos
{
namespace Testing
{

KRATOS_TEST_CASE_IN_SUITE(HDF5_Internals_ConnectivitiesData1, KratosHDF5TestSuite)
{
    ModelPart test_model_part;
    TestModelPartFactory::CreateModelPart(test_model_part, {{"Element2D3N"}});
    KRATOS_CHECK(test_model_part.Elements().size() > 0);
    HDF5::Internals::ConnectivitiesData data;
    data.SetData(FactorElements(test_model_part.Elements()).front());
    auto test_file = GetTestSerialFile();
    HDF5::WriteInfo info;
    data.WriteData(test_file, "/Elements", info);
    data.Clear();
    KRATOS_CHECK(data.size() == 0);
    data.ReadData(test_file, "/Elements", info.StartIndex, info.BlockSize);
    HDF5::ElementsContainerType new_elements;
    data.CreateEntities(test_model_part.Nodes(), test_model_part.rProperties(), new_elements);
    CompareElements(new_elements, test_model_part.Elements());
}

KRATOS_TEST_CASE_IN_SUITE(HDF5_Internals_ConnectivitiesData2, KratosHDF5TestSuite)
{
    ModelPart test_model_part;
    TestModelPartFactory::CreateModelPart(test_model_part, {}, {{"SurfaceCondition3D3N"}});
    KRATOS_CHECK(test_model_part.Conditions().size() > 0);
    HDF5::Internals::ConnectivitiesData data;
    data.SetData(FactorConditions(test_model_part.Conditions()).front());
    auto test_file = GetTestSerialFile();
    HDF5::WriteInfo info;
    data.WriteData(test_file, "/Conditions", info);
    data.Clear();
    KRATOS_CHECK(data.size() == 0);
    data.ReadData(test_file, "/Conditions", info.StartIndex, info.BlockSize);
    HDF5::ConditionsContainerType new_conditions;
    data.CreateEntities(test_model_part.Nodes(), test_model_part.rProperties(), new_conditions);
    CompareConditions(new_conditions, test_model_part.Conditions());
}

} // namespace Testing
} // namespace Kratos.