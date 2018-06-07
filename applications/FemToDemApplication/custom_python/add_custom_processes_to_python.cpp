//   
//   Project Name:           
//   Last modified by:    $Author:  $
//   Date:                $Date:  $
//   Revision:            $Revision: $

// External includes


// Project includes

#include "includes/model_part.h"
#include "fem_to_dem_application_variables.h"
//#include "processes/process.h"
#include "custom_python/add_custom_processes_to_python.h" 
#include "custom_processes/stress_to_nodes_process.hpp"  
#include "custom_processes/dem_after_remesh_identificator_process.hpp" 

namespace Kratos
{
	namespace Python
	{
		void AddCustomProcessesToPython(pybind11::module& m)
		{
			using namespace pybind11;

			typedef Process                           ProcessBaseType;
			//typedef AdaptiveMeshRefinementProcess     AdaptiveMeshRefinementProcessType;
			//typedef MappingVariablesProcess           MappingVariablesProcessType;
			//typedef StressToNodesProcess              StressToNodesProcessType;


			// class_<FindElementalNeighboursProcess, Process>(m,"FindElementalNeighboursProcess")
			// 	.def(init<ModelPart&, int, unsigned int>())
			// 	.def("Execute", &FindElementalNeighboursProcess::Execute)
			// 	;

			// Adaptive Mesh Refinement Process
			// class_<AdaptiveMeshRefinementProcessType, Process>(m,"AdaptiveMeshRefinementProcess")
			// 	.def(init < ModelPart&, std::string, std::string, std::string, std::string, double, int >())
			// 	.def("Execute", &AdaptiveMeshRefinementProcessType::Execute)
			// 	;
	
				
			// Mapping Variables Process
			// class_<MappingVariablesProcessType, Process>(m,"MappingVariablesProcess")
			// 	.def(init < ModelPart&,ModelPart&, std::string, std::string >())
			// 	.def("Execute", &MappingVariablesProcessType::Execute)
			// 	;

			// Stress extrapolation to Nodes
			class_<StressToNodesProcess, StressToNodesProcess::Pointer,  Process>
				(m,"StressToNodesProcess")
				.def(init < ModelPart&, unsigned int >())
				.def("Execute", &StressToNodesProcess::Execute)
				;
			
			class_<DemAfterRemeshIdentificatorProcess, DemAfterRemeshIdentificatorProcess::Pointer,  Process>
				(m,"DemAfterRemeshIdentificatorProcess")
				.def(init < ModelPart&>())
				.def("Execute", &DemAfterRemeshIdentificatorProcess::Execute)
				;

		}
	}  // namespace Python.
} // Namespace Kratos