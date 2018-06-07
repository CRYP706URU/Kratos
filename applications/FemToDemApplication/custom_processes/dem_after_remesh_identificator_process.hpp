//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ \.
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:		 BSD License
//					 Kratos default license: kratos/license.txt
//
//  Main authors:    Alejandro Cornejo Velázquez
//

// System includes

// External includes

// Project includes

#if !defined(KRATOS_DEM_AFTER_REMESH_IDENTIFICATOR_PROCESS_HPP_INCLUDED )
#define  KRATOS_DEM_AFTER_REMESH_IDENTIFICATOR_PROCESS_HPP_INCLUDED


#include "processes/process.h"
#include "includes/model_part.h"
#include "processes/find_nodal_neighbours_process.h"

namespace Kratos
{

class  DemAfterRemeshIdentificatorProcess : public Process
{
public:

    KRATOS_CLASS_POINTER_DEFINITION(DemAfterRemeshIdentificatorProcess);

    // Constructor
    DemAfterRemeshIdentificatorProcess(ModelPart& rModelPart)
        : mrModelPart(rModelPart)
    {
    }
    // It will create a submodel part containing nodes to include DEM on them,
    // with the DEM radius assigned to each node to be created the DEM afterwards
    // The modelpart must include the skinModelpart and damage extrapolated to nodes
    void Execute()
    {
        const std::string& name_dem_model_part = "DemAfterRemeshingNodes";

        if (mrModelPart.HasSubModelPart(name_dem_model_part)) {
            mrModelPart.RemoveSubModelPart(name_dem_model_part);
        }

        mrModelPart.CreateSubModelPart(name_dem_model_part);
        ModelPart::Pointer p_auxiliar_model_part = mrModelPart.pGetSubModelPart(name_dem_model_part);
        ModelPart::Pointer p_skin_model_part = mrModelPart.pGetSubModelPart("SkinDEMModelPart");

        for (ModelPart::NodeIterator it = (*p_skin_model_part).NodesBegin(); it != (*p_skin_model_part).NodesEnd(); ++it) {
            const double nodal_damage = it->GetSolutionStepValue(NODAL_DAMAGE);
            if (nodal_damage > 0.94) {
                p_auxiliar_model_part->AddNode(*(it.base()));
            }
        } // DemAfterRemeshingNodes SubModelPart Filled with nodes

        // Let's assign the DEM radius to those nodes...
        Process& neighbour_finder = FindNodalNeighboursProcess(mrModelPart, 4, 4);
        neighbour_finder.Execute();

        for (ModelPart::NodeIterator it = (*p_auxiliar_model_part).NodesBegin(); it != (*p_auxiliar_model_part).NodesEnd(); ++it) {

            WeakPointerVector< Node<3> >& rneigh = (*it).GetValue(NEIGHBOUR_NODES);
            std::vector<double> radius_dems;
            double distance, radius_dem, min_radius;

            for (int i = 0; i < rneigh.size(); i++) {

                distance = this->CalculateDistanceBetweenNodes((*it), rneigh[i]);
                if (rneigh[i].GetValue(DEM_RADIUS) != 0.0) {
                    radius_dem = distance - rneigh[i].GetValue(DEM_RADIUS);
                } else {
                    radius_dem = 0.5 * distance;
                }
                radius_dems.push_back(radius_dem);
            }

            min_radius = this->GetMinimumValue(radius_dems);
            (*it).SetValue(DEM_RADIUS, min_radius);

            // KRATOS_WATCH((*it).Id())
            // KRATOS_WATCH((*it).GetValue(DEM_RADIUS))
        }

        // std::cout<< "******************"<<std::endl;
        // for (ModelPart::NodeIterator it = (mrModelPart).NodesBegin(); it != (mrModelPart).NodesEnd(); ++it) {
		// 	if ((*it).GetValue(DEM_RADIUS) != 0.0) {
		// 		KRATOS_WATCH((*it).Id())
		// 		KRATOS_WATCH((*it).GetValue(DEM_RADIUS))
		// 	}
        // }
    }

    double CalculateDistanceBetweenNodes(
        const Node<3>& Node1, 
        const Node<3>& Node2)
    {
        const double X1 = Node1.X();
		const double X2 = Node2.X();
        const double Y1 = Node1.Y();
		const double Y2 = Node2.Y();
        const double Z1 = Node1.Z();
		const double Z2 = Node2.Z();
        return std::sqrt(std::pow((X1-X2),2) + std::pow((Y1-Y2),2) + std::pow((Z1-Z2),2));
    }

    double GetMinimumValue(std::vector<double> array_values)
    {
        int size = array_values.size();
		double aux = array_values[0];
        for (int i = 1; i < size; i++) {
            if (array_values[i] < aux) aux = array_values[i];
        }
        return aux;
    }

protected:
        
    // Member Variables
    ModelPart& mrModelPart;

};
}
#endif