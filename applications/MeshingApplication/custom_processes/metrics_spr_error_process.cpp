// KRATOS  __  __ _____ ____  _   _ ___ _   _  ____ 
//        |  \/  | ____/ ___|| | | |_ _| \ | |/ ___|
//        | |\/| |  _| \___ \| |_| || ||  \| | |  _ 
//        | |  | | |___ ___) |  _  || || |\  | |_| |
//        |_|  |_|_____|____/|_| |_|___|_| \_|\____| APPLICATION
//
//  License:		 BSD License
//                       license: MeshingApplication/license.txt
//
//  Main authors:    Anna Rehr
//  Co-author   :    Vicente Mataix Ferrandiz
//

// Project includes
#include "custom_processes/metrics_spr_error_process.h"

namespace Kratos
{
template<SizeType TDim>
SPRMetricProcess<TDim>::SPRMetricProcess(
    ModelPart& rThisModelPart,
    Parameters ThisParameters
    )
    :mThisModelPart(rThisModelPart)
{               
    Parameters default_parameters = Parameters(R"(
    {
        "minimal_size"                        : 0.01,
        "maximal_size"                        : 10.0, 
        "error"                               : 0.1,
        "penalty_normal"                      : 10000.0,
        "penalty_tangential"                  : 10000.0,
        "echo_level"                          : 0,
        "set_number_of_elements"              : false,
        "number_of_elements"                  : 1000,
        "average_nodal_h"                     : false
    })" 
    );
    
    ThisParameters.ValidateAndAssignDefaults(default_parameters);
    
    mMinSize = ThisParameters["minimal_size"].GetDouble();
    mMaxSize = ThisParameters["maximal_size"].GetDouble();
    mPenaltyNormal = ThisParameters["penalty_normal"].GetDouble();
    mPenaltyTangent = ThisParameters["penalty_tangential"].GetDouble();
    mEchoLevel = ThisParameters["echo_level"].GetInt();
    mSigmaSize = (TDim == 2) ? 3 : 6;
    mSetElementNumber = ThisParameters["set_number_of_elements"].GetBool();
    mElementNumber = ThisParameters["number_of_elements"].GetInt();
    mTargetError = ThisParameters["error"].GetDouble();
    mAverageNodalH = ThisParameters["average_nodal_h"].GetBool();

//     SkylineLUFactorizationSolver< UblasSpace<double,CompressedMatrix,Vector>, UblasSpace<double,Matrix,Vector>> solver = SkylineLUFactorizationSolver< UblasSpace<double,CompressedMatrix,Vector>, UblasSpace<double,Matrix,Vector>>();
}
    
/***********************************************************************************/
/***********************************************************************************/
    
template<SizeType TDim>
void SPRMetricProcess<TDim>::Execute()
{
    /************************************************************************
    --1-- Calculate superconvergent stresses (at the nodes) --1--
    ************************************************************************/

    FindNodalNeighboursProcess find_neighbours(mThisModelPart);
    find_neighbours.Execute();

    // Iteration over all nodes -- construction of patches
    NodesArrayType& nodes_array = mThisModelPart.Nodes();
    SizeType num_nodes = nodes_array.size();
    
    for(IndexType i_node = 0; i_node < num_nodes; ++i_node) {
        auto it_node = nodes_array.begin() + i_node;
        
        const SizeType neighbour_size = it_node->GetValue(NEIGHBOUR_ELEMENTS).size();

        Vector sigma_recovered(mSigmaSize, 0.0);
        
        if(neighbour_size > TDim) {
            CalculatePatch(it_node, it_node, neighbour_size,sigma_recovered);
            it_node->SetValue(RECOVERED_STRESS, sigma_recovered);
            
            KRATOS_INFO_IF("SPRMetricProcess", mEchoLevel > 2) << "Recovered sigma: " << sigma_recovered << std::endl;
        } else {
            auto& neigh_nodes = it_node->GetValue(NEIGHBOUR_NODES);
            for(auto it_neighbour_nodes = neigh_nodes.begin(); it_neighbour_nodes != neigh_nodes.end(); it_neighbour_nodes++){
                
                Vector sigma_recovered_i(mSigmaSize,0);
                
                IndexType count_i = 0;
                for(IndexType i_node_loop = 0; i_node_loop < num_nodes; ++i_node_loop) { // FIXME: Avoid thsi double loop, extreamily expensive
                    auto it_node_loop = nodes_array.begin() + i_node_loop;
                    const SizeType size_elem_neigh = it_node_loop->GetValue(NEIGHBOUR_ELEMENTS).size();
                    if (it_node_loop->Id() == it_neighbour_nodes->Id() && size_elem_neigh > TDim){
                        CalculatePatch(it_node, it_node_loop, neighbour_size, sigma_recovered_i);
                        ++count_i;
                    }
                }
                
                // Average solution from different patches
                if(count_i != 0)
                    sigma_recovered = sigma_recovered*(count_i-1)/count_i + sigma_recovered_i/count_i;
            }
            
            it_node->SetValue(RECOVERED_STRESS,sigma_recovered);
            KRATOS_INFO_IF("SPRMetricProcess", mEchoLevel > 2) << "Recovered sigma: " << sigma_recovered << std::endl;
        }
    }
    /******************************************************************************
    --2-- calculate error estimation and new element size (for each element) --2--
    ******************************************************************************/
    //loop over all elements: 
    double error_overall = 0.0;
    double energy_norm_overall = 0.0;

    ElementsArrayType& elements_array = mThisModelPart.Elements();
    int num_elem = elements_array.end() - elements_array.begin();
    
    // Compute the error estimate per element
    #pragma omp parallel for
    for(int i_elem = 0; i_elem < num_elem; ++i_elem){
        auto it_elem = elements_array.begin() + i_elem;
        
        std::vector<double> error_integration_point;
        auto& process_info = mThisModelPart.GetProcessInfo();
        it_elem->GetValueOnIntegrationPoints(ERROR_INTEGRATION_POINT, error_integration_point, process_info);

        KRATOS_INFO_IF("SPRMetricProcess", mEchoLevel > 2) << "Error GP:" << error_integration_point << std::endl;

        double error_energy_norm = 0.0;
        for(IndexType i = 0;i < error_integration_point.size();++i)
            error_energy_norm += error_integration_point[i];
        error_overall += error_energy_norm;
        error_energy_norm = std::sqrt(error_energy_norm);
        it_elem->SetValue(ELEMENT_ERROR, error_energy_norm);
        
        KRATOS_INFO_IF("SPRMetricProcess", mEchoLevel > 2) << "Element error: " << error_energy_norm << std::endl;

        std::vector<double> strain_energy;
        it_elem->GetValueOnIntegrationPoints(STRAIN_ENERGY, strain_energy, process_info);
        
        double energy_norm = 0.0;
        for(IndexType i = 0;i < strain_energy.size(); ++i)
            energy_norm += 2.0 * strain_energy[i];
        energy_norm_overall += energy_norm;
        energy_norm= std::sqrt(energy_norm);
        
        KRATOS_INFO_IF("SPRMetricProcess", mEchoLevel > 2) << "Energy norm: " << energy_norm << std::endl;
    }
    
    error_overall = std::sqrt(error_overall);
    energy_norm_overall = std::sqrt(energy_norm_overall);
    double error_percentage = error_overall/std::sqrt((std::pow(error_overall, 2) + std::pow(energy_norm_overall, 2)));
    
    KRATOS_INFO_IF("SPRMetricProcess", mEchoLevel > 1)
        << "Overall error norm: " << error_overall << std::endl
        << "Overall energy norm: "<< energy_norm_overall << std::endl
        << "Error in percent: " << error_percentage << std::endl;
    
    // Compute new element size
    #pragma omp parallel for
    for(int i_elem = 0; i_elem < num_elem; ++i_elem){
        auto it_elem = elements_array.begin() + i_elem;
        
        //Compute the current element size h
        //it_elem->CalculateElementSize();
        ComputeElementSize(it_elem);

        // Compute new element size
        double new_element_size = it_elem->GetValue(ELEMENT_H)/it_elem->GetValue(ELEMENT_ERROR);

        // if a target number for elements is given: use this, else: use current element number
        //if(mSetElementNumber == true && mElementNumber<mThisModelPart.Elements().size())
        if(mSetElementNumber == true)
            new_element_size *= std::sqrt((std::pow(energy_norm_overall, 2)+ std::pow(error_overall, 2))/mElementNumber) * mTargetError;
        else
            new_element_size *= std::sqrt((energy_norm_overall*energy_norm_overall+error_overall*error_overall)/mThisModelPart.Elements().size())*mTargetError;
        
        
        // Check if element sizes are in specified limits. If not, set them to the limit case
        if(new_element_size < mMinSize)
            new_element_size = mMinSize;
        if(new_element_size > mMaxSize)
            new_element_size = mMaxSize;

        it_elem->SetValue(ELEMENT_H, new_element_size);
    }

    /******************************************************************************
    --3-- Calculate metric (for each node) --3--
    ******************************************************************************/

    #pragma omp parallel for
    for(int i_node = 0; i_node < num_nodes; ++i_node) {
        auto it_node = nodes_array.begin() + i_node;
        /**************************************************************************
        ** Determine nodal element size h:
        ** if average_nodal_h == true : the nodal element size is averaged from the element size of neighboring elements
        ** if average_nodal_h == false: the nodal element size is the minimum element size from neighboring elements
        */
        double h_min = 0.0;
        auto& neigh_elements = it_node->GetValue(NEIGHBOUR_ELEMENTS);
        for(WeakElementItType i_neighbour_elements = neigh_elements.begin(); i_neighbour_elements != neigh_elements.end(); i_neighbour_elements++){
            const double element_h = i_neighbour_elements->GetValue(ELEMENT_H);
            if(mAverageNodalH == false){
                if(h_min == 0.0 || h_min > element_h) h_min = element_h;
            } else {
                h_min += element_h;
            }
        }
        if(mAverageNodalH == true)
            h_min = h_min/static_cast<double>(neigh_elements.size());

        // Set metric
        Matrix metric_matrix(TDim, TDim, 0.0);
        for(IndexType i = 0;i < TDim; ++i)
            metric_matrix(i,i) = 1.0/std::pow(h_min, 2);

        // Transform metric matrix to a vector
        const Vector metric = MetricsMathUtils<TDim>::TensorToVector(metric_matrix);
        it_node->SetValue(MMG_METRIC, metric);

        KRATOS_INFO_IF("SPRMetricProcess", mEchoLevel > 2) << "Node "<<it_node->Id()<<" has metric: "<<it_node->GetValue(MMG_METRIC)<<std::endl;
    }
    
    mThisModelPart.GetProcessInfo()[ERROR_ESTIMATE] = error_overall/std::pow((error_overall*error_overall+energy_norm_overall*energy_norm_overall),0.5);
}

/***********************************************************************************/
/***********************************************************************************/

template<SizeType TDim>
void SPRMetricProcess<TDim>::CalculatePatch(
    NodeItType itNode,
    NodeItType itPatchNode,
    SizeType NeighbourSize,
    Vector& rSigmaRecovered
    )
{
    // Determine if contact BC has to be regarded
    // We take the geometry GP from the core
    const double tolerance = std::numeric_limits<double>::epsilon();
    const bool regard_contact = std::abs(itNode->GetValue(CONTACT_PRESSURE)) > tolerance ? true : false;
    
//     regard_contact = itPatchNode->Has(CONTACT_PRESSURE);
//     if(regard_contact == false) {
//         for( auto& i_neighbour_nodes : itPatchNode->GetValue(NEIGHBOUR_NODES)) {
//             if (i_neighbour_nodes.Has(CONTACT_PRESSURE)) {
//                 regard_contact = true;
//                 break;
//             }
//         }
//     }
    
    if (regard_contact == false)
        CalculatePatchStandard(itNode, itPatchNode, NeighbourSize, rSigmaRecovered);
    else
        CalculatePatchContact(itNode, itPatchNode, NeighbourSize, rSigmaRecovered);
}

/***********************************************************************************/
/***********************************************************************************/
    
template<SizeType TDim>
void SPRMetricProcess<TDim>::CalculatePatchStandard(
    NodeItType itNode,
    NodeItType itPatchNode,
    SizeType NeighbourSize,
    Vector& rSigmaRecovered
    )
{
    std::vector<Vector> stress_vector(1);
    std::vector<array_1d<double,3>> coordinates_vector(1);
    Variable<array_1d<double,3>> variable_coordinates = INTEGRATION_COORDINATES;
    Variable<Vector> variable_stress = CAUCHY_STRESS_VECTOR;
    Matrix A(TDim+1,TDim+1,0);
    Matrix b(TDim+1,mSigmaSize,0); 
    Matrix p_k(1,TDim+1,0);
    
    auto& neigh_elements = itPatchNode->GetValue(NEIGHBOUR_ELEMENTS);
    for( WeakElementItType it_elem = neigh_elements.begin(); it_elem != neigh_elements.end(); ++it_elem) {
        
        it_elem->GetValueOnIntegrationPoints(variable_stress,stress_vector,mThisModelPart.GetProcessInfo());
        it_elem->GetValueOnIntegrationPoints(variable_coordinates,coordinates_vector,mThisModelPart.GetProcessInfo());

        KRATOS_INFO_IF("SPRMetricProcess", mEchoLevel > 3)
        << "\tStress: " << stress_vector[0] << std::endl
        << "\tx: " << coordinates_vector[0][0] << "\ty: " << coordinates_vector[0][1] << "\tz_coordinate: " << coordinates_vector[0][2] << std::endl;
        
        Matrix sigma(1,mSigmaSize);
        for(IndexType j = 0; j < mSigmaSize; ++j)
            sigma(0,j)=stress_vector[0][j];
        p_k(0,0)=1;
        p_k(0,1)=coordinates_vector[0][0]-itPatchNode->X(); 
        p_k(0,2)=coordinates_vector[0][1]-itPatchNode->Y();
        if(TDim == 3)
            p_k(0,3)=coordinates_vector[0][2]-itPatchNode->Z();       
        
        A += prod(trans(p_k), p_k);
        b += prod(trans(p_k), sigma);
    }
    
    Matrix invA(TDim+1,TDim+1);
    double det;
    MathUtils<double>::InvertMatrix(A,invA,det);

    KRATOS_INFO_IF("SPRMetricProcess", mEchoLevel > 3) << A << std::endl << invA << std::endl << det<< std::endl;

    const double tolerance = std::numeric_limits<double>::epsilon();
    if(det < tolerance){
        KRATOS_WARNING_IF("SPRMetricProcess", mEchoLevel == 2) << A << std::endl;
        for( IndexType i=0; i<TDim+1;i++){
            for( IndexType j=0; j<TDim+1; j++)
                A(i,j)+= 0.001;
        }
        MathUtils<double>::InvertMatrix(A, invA, det);
        KRATOS_WARNING_IF("SPRMetricProcess", mEchoLevel > 0) << "det: " << det << std::endl;
    }

    Matrix coeff(TDim+1,mSigmaSize);
    coeff = prod(invA,b);
    
    if(NeighbourSize > TDim) {
        rSigmaRecovered = MatrixRow(coeff,0);
    } else {
        p_k(0,1)=itNode->X()-itPatchNode->X(); 
        p_k(0,2)=itNode->Y()-itPatchNode->Y();
        if(TDim ==3)
            p_k(0,3)=itNode->Z()-itPatchNode->Z();
        Matrix sigma(1,mSigmaSize);
        sigma = prod(p_k,coeff);
        rSigmaRecovered = MatrixRow(sigma,0);
    }
}

/***********************************************************************************/
/***********************************************************************************/

template<SizeType TDim>
void SPRMetricProcess<TDim>::CalculatePatchContact(
    NodeItType itNode,
    NodeItType itPatchNode,
    SizeType NeighbourSize,
    Vector& rSigmaRecovered)
{

    std::vector<Vector> stress_vector(1);
    std::vector<array_1d<double,3>> coordinates_vector(1);
    Variable<array_1d<double,3>> variable_coordinates = INTEGRATION_COORDINATES;
    Variable<Vector> variable_stress = CAUCHY_STRESS_VECTOR;

    CompressedMatrix A((mSigmaSize*(TDim+1)),(mSigmaSize*(TDim+1)),0);
    Matrix b((mSigmaSize*(TDim+1)),1,0); 
    Matrix p_k(mSigmaSize,(mSigmaSize*(TDim+1)),0);
    Matrix N_k(1,mSigmaSize,0);
    Matrix T_k(1,mSigmaSize,0);
    Matrix T_k2(1,mSigmaSize,0);  // in case of 3D: second tangential vector
    Matrix sigma(mSigmaSize,1);
    
    /* Computation A and b */
    // PART 1: contributions from the neighboring elements
    auto& neigh_elements = itPatchNode->GetValue(NEIGHBOUR_ELEMENTS);
    for( WeakElementItType it_elem = neigh_elements.begin(); it_elem != neigh_elements.end(); ++it_elem) {
        
        auto& process_info = mThisModelPart.GetProcessInfo();
        it_elem->GetValueOnIntegrationPoints(variable_stress,stress_vector, process_info);
        it_elem->GetValueOnIntegrationPoints(variable_coordinates,coordinates_vector, process_info);

        KRATOS_INFO_IF("SPRMetricProcess", mEchoLevel > 3)
        << "\tElement: " << it_elem->Id() << std::endl
        << "\tStress: " << stress_vector[0] << std::endl
        << "\tX: " << coordinates_vector[0][0] << "\tY: " << coordinates_vector[0][1] << "\tZ: " << coordinates_vector[0][2] << std::endl;
        
        for( IndexType j = 0; j < mSigmaSize; ++j)
            sigma(j,0) = stress_vector[0][j];
        
        for ( IndexType j = 0; j < mSigmaSize; ++j){
            p_k(j,j*(TDim+1))=1;
            p_k(j,j*(TDim+1)+1)=coordinates_vector[0][0]-itPatchNode->X(); 
            p_k(j,j*(TDim+1)+2)=coordinates_vector[0][1]-itPatchNode->Y();
            if(TDim == 3)
                p_k(j,j*(TDim+1)+3)=coordinates_vector[0][2]-itPatchNode->Z();
        }
        
        A += prod(trans(p_k),p_k);
        b += prod(trans(p_k),sigma);
    }
    
    // Computing A and b
    Matrix A1((mSigmaSize * (TDim + 1)),1,0), A2(1,(mSigmaSize *(TDim + 1)),0);
    for (IndexType j = 0; j < mSigmaSize; ++j){
        p_k(j,j * (TDim + 1) + 1)= itNode->X() - itPatchNode->X();
        p_k(j,j * (TDim + 1) + 2)= itNode->Y() - itPatchNode->Y();
        if(TDim == 3)
            p_k(j,j * (TDim + 1) + 3)= itNode->Z() - itPatchNode->Z();
    }
    
    // Set the normal and tangential vectors in Voigt Notation
    const array_1d<double, 3>& normal = itNode->GetValue(NORMAL);

    if(TDim == 2) {
        N_k(0,0) = normal[0]*normal[0];
        N_k(0,1) = normal[1]*normal[1];
        N_k(0,2) = 2.0*normal[0]*normal[1];
        
        T_k(0,0) = normal[0]*normal[1];
        T_k(0,1) = -normal[0]*normal[1];
        T_k(0,2) = normal[1]*normal[1]-normal[0]*normal[0];
    } else if (TDim == 3) {
        N_k(0,0) = normal[0]*normal[0];
        N_k(0,1) = normal[1]*normal[1];
        N_k(0,1) = normal[2]*normal[2];
        N_k(0,3) = 2*normal[1]*normal[2];
        N_k(0,4) = 2*normal[2]*normal[0];
        N_k(0,5) = 2*normal[0]*normal[1];

        // Set tangential vectors
        array_1d<double, 3> t1(3, 0.0);
        array_1d<double, 3> t2(3, 0.0);

        const double tolerance = std::numeric_limits<double>::epsilon();
        if(std::abs(normal[0]) > tolerance || std::abs(normal[1]) > tolerance) {
            const double norm = std::sqrt((t1[0]*t1[0]+t1[1]*t1[1]));
            t1[0] = normal[1]/norm;
            t1[1] = -normal[0]/norm;

            t2[0] = -normal[0]*normal[2]/norm;
            t2[1] = -normal[1]*normal[2]/norm;
            t2[2] = normal[0]*normal[0]+normal[1]*normal[1]/norm;
        } else{
            t1[0] = 1.0;
            t2[1] = 1.0;
        }

        T_k(0,0) = normal[0]*t1[0];
        T_k(0,1) = normal[1]*t1[1];
        T_k(0,2) = normal[2]*t1[2];
        T_k(0,3) = normal[1]*t1[2]+normal[2]*t1[1];
        T_k(0,4) = normal[2]*t1[0]+normal[0]*t1[2];
        T_k(0,5) = normal[0]*t1[1]+normal[1]*t1[0];
        
        T_k2(0,0) = normal[0]*t2[0];
        T_k2(0,1) = normal[1]*t2[1];
        T_k2(0,2) = normal[2]*t2[2];
        T_k2(0,3) = normal[1]*t2[2]+normal[2]*t2[1];
        T_k2(0,4) = normal[2]*t2[0]+normal[0]*t2[2];
        T_k2(0,5) = normal[0]*t2[1]+normal[1]*t2[0];
    }
    
    A1 = prod(trans(p_k),trans(N_k));
    A2 = prod(N_k,p_k);
    A += mPenaltyNormal*prod(A1, A2);

    A1 = prod(trans(p_k),trans(T_k));
    A2 = prod(T_k,p_k);
    A += mPenaltyTangent*prod(A1, A2);

    b += mPenaltyNormal*prod(trans(p_k),trans(N_k))*itNode->GetValue(CONTACT_PRESSURE);

//     //PART 2: contributions from contact nodes: regard all nodes from the patch which are in contact
//     //patch center node:
//     if (itPatchNode->Has(CONTACT_PRESSURE)){
//         const array_1d<double, 3>& normal_patch_node = itPatchNode->GetValue(NORMAL);
//         p_k(0,1)=0.0;
//         p_k(0,2)=0.0;
//         p_k(1,4)=0.0;
//         p_k(1,5)=0.0;
//         p_k(2,7)=0.0;
//         p_k(2,8)=0.0;
//         N_k(0,0) = normal_patch_node[0]*normal_patch_node[0];
//         N_k(0,1) = normal_patch_node[1]*normal_patch_node[1];
//         N_k(0,2) = 2*normal_patch_node[0]*normal_patch_node[1];
//         T_k(0,0) = normal_patch_node[0]*normal_patch_node[1];
//         T_k(0,1) = -normal_patch_node[0]*normal_patch_node[1];
//         T_k(0,2) = normal_patch_node[1]*normal_patch_node[1]-normal_patch_node[0]*normal_patch_node[0];
//
//         A1 = prod(trans(p_k),trans(N_k));
//         A2 = prod(N_k,p_k);
//         A+= mPenaltyNormal*prod(A1, A2);
//
//         A1 = prod(trans(p_k),trans(T_k));
//         A2 = prod(T_k,p_k);
//         A+= mPenaltyTangent*prod(A1, A2);
//         //A+= mPenaltyNormal*prod(prod(trans(p_k),trans(N_k)),prod(N_k,p_k));
//         //A+= mPenaltyTangent*prod(prod(prod(trans(p_k),trans(T_k)),T_k),p_k);
//
//         b-= mPenaltyNormal*prod(trans(p_k),trans(N_k))*itPatchNode->GetValue(CONTACT_PRESSURE);
//     }
//
//     // Neighboring nodes:
//     for( auto& i_neighbour_nodes : itPatchNode->GetValue(NEIGHBOUR_NODES)) {
//         if (i_neighbour_nodes.Has(CONTACT_PRESSURE)){
//             const array_1d<double, 3>& normal_neigh_node = i_neighbour_nodes.GetValue(NORMAL);
//             const double x_patch = itPatchNode->X();
//             const double y_patch = itPatchNode->Y();
//             const double x_neigh = i_neighbour_nodes.X();
//             const double y_neigh = i_neighbour_nodes.Y();
//             p_k(0,1)= x_neigh-x_patch;
//             p_k(0,2)= y_neigh-y_patch;
//             p_k(1,4)= x_neigh-x_patch;
//             p_k(1,5)= y_neigh-y_patch;
//             p_k(2,7)= x_neigh-x_patch;
//             p_k(2,8)= y_neigh-y_patch;
//             N_k(0,0) = normal_neigh_node[0]*normal_neigh_node[0];
//             N_k(0,1) = normal_neigh_node[1]*normal_neigh_node[1];
//             N_k(0,2) = 2*normal_neigh_node[0]*normal_neigh_node[1];
//             T_k(0,0) = normal_neigh_node[0]*normal_neigh_node[1];
//             T_k(0,1) = -normal_neigh_node[0]*normal_neigh_node[1];
//             T_k(0,2) = normal_neigh_node[1]*normal_neigh_node[1]-normal_neigh_node[0]*normal_neigh_node[0];
//
//             A1 = prod(trans(p_k),trans(N_k));
//             A2 = prod(N_k,p_k);
//             A+= mPenaltyNormal*prod(A1, A2);
//
//             A1 = prod(trans(p_k),trans(T_k));
//             A2 = prod(T_k,p_k);
//             A+= mPenaltyTangent*prod(A1, A2);
//
//             b+= mPenaltyNormal*prod(trans(p_k),trans(N_k))*i_neighbour_node->GetValue(CONTACT_PRESSURE);
//         }
//     }

    // Computing coefficients a: A*a=b
    SkylineLUFactorizationSolver< UblasSpace<double,CompressedMatrix,Vector>, UblasSpace<double,Matrix,Vector>> solver = SkylineLUFactorizationSolver< UblasSpace<double,CompressedMatrix,Vector>, UblasSpace<double,Matrix,Vector>>();

    KRATOS_INFO_IF("SPRMetricProcess", mEchoLevel > 3) << A << std::endl;
    
    Vector coeff(mSigmaSize*(TDim+1));
    Vector b_vector = MatrixColumn(b,0);
    solver.Solve(A,coeff,b_vector);

    for (IndexType j = 0; j < mSigmaSize;++j){
        p_k(j,j*(TDim + 1) + 1)= itNode->X() - itPatchNode->X();
        p_k(j,j*(TDim + 1) + 2)= itNode->Y() - itPatchNode->Y();
        if (TDim == 3)
            p_k(j,j*(TDim + 1) + 3)= itNode->Z() - itPatchNode->Z();
    }
    
    Matrix coeff_matrix(mSigmaSize*(TDim + 1), 1);
    for (IndexType i=0; i<mSigmaSize*(TDim + 1); ++i)
        coeff_matrix(i,0)=coeff(i);
    
    sigma = prod(p_k,coeff_matrix);

    rSigmaRecovered = MatrixColumn(sigma,0);
    
    KRATOS_INFO_IF("SPRMetricProcess", mEchoLevel > 1) <<" Recovered pressure: "<< prod(N_k,sigma) <<", LM: "<<itNode->GetValue(CONTACT_PRESSURE)<<std::endl;
}

/***********************************************************************************/
/***********************************************************************************/

template<SizeType TDim>
void SPRMetricProcess<TDim>::ComputeElementSize(ElementItType itElement)
{
    auto& this_geometry = itElement->GetGeometry(); 
    
    if (this_geometry.GetGeometryType() == GeometryData::KratosGeometryType::Kratos_Triangle2D3){ // Triangular elements
        itElement->SetValue(ELEMENT_H, 2.0 * this_geometry.Circumradius());
    } else if(this_geometry.GetGeometryType() == GeometryData::KratosGeometryType::Kratos_Tetrahedra3D4){ // Tetrahedral elements
        itElement->SetValue(ELEMENT_H,std::pow(12.0 * GeometryUtils::CalculateVolume3D(this_geometry)/std::sqrt(2.0), 1.0/3.0));
    }
}

/***********************************************************************************/
/***********************************************************************************/

template class SPRMetricProcess<2>;
template class SPRMetricProcess<3>;

};// namespace Kratos.