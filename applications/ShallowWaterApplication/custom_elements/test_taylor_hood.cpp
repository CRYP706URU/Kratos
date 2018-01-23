//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:         BSD License
//                   Kratos default license: kratos/license.txt
//
//  Main authors:    Miguel Maso Sotomayor
//

#include "test_taylor_hood.hpp"
#include "geometries/triangle_2d_3.h"
#include "geometries/quadrilateral_2d_4.h"
#include "geometries/tetrahedra_3d_4.h"
#include "geometries/hexahedra_3d_8.h"
#include "includes/checks.h"
#include "shallow_water_application.h"

namespace Kratos {

Element::Pointer TestTaylorHood::Create(IndexType NewId, NodesArrayType const& ThisNodes, PropertiesType::Pointer pProperties) const
{
    return Element::Pointer(new TestTaylorHood(NewId, this->GetGeometry().Create(ThisNodes), pProperties) );
}


int TestTaylorHood::Check(const ProcessInfo &rCurrentProcessInfo)
{
    KRATOS_TRY

    // Base class checks for positive Jacobian and Id > 0
    int result = Element::Check(rCurrentProcessInfo);
    if(result != 0) return result;

    // Check that all required variables have been registered
    KRATOS_CHECK_VARIABLE_KEY(VELOCITY)
    KRATOS_CHECK_VARIABLE_KEY(HEIGHT)
    KRATOS_CHECK_VARIABLE_KEY(DELTA_TIME)
    KRATOS_CHECK_VARIABLE_KEY(BATHYMETRY)
    KRATOS_CHECK_VARIABLE_KEY(GRAVITY)
    KRATOS_CHECK_VARIABLE_KEY(MANNING)
    KRATOS_CHECK_VARIABLE_KEY(RAIN)

    // Check that the element's nodes contain all required SolutionStepData and Degrees of freedom
    for( unsigned int i = 0; i < this->GetGeometry().size(); ++i )
    {
        Node<3> &rNode = this->GetGeometry()[i];
        KRATOS_CHECK_VARIABLE_IN_NODAL_DATA(VELOCITY, rNode)
        KRATOS_CHECK_VARIABLE_IN_NODAL_DATA(HEIGHT, rNode)
        KRATOS_CHECK_VARIABLE_IN_NODAL_DATA(BATHYMETRY, rNode)
        KRATOS_CHECK_VARIABLE_IN_NODAL_DATA(RAIN, rNode)
    }

    // If this is a 2D problem, check that nodes are in XY plane
    //~ if (this->GetGeometry().WorkingSpaceDimension() == 2)
    //~ {
        //~ for (unsigned int i=0; i<this->GetGeometry().size(); ++i)
        //~ {
            //~ if (this->GetGeometry()[i].Z() != 0.0)
                //~ KRATOS_ERROR << "Node with non-zero Z coordinate found. Id: " << this->GetGeometry()[i].Id() << std:endl;
        //~ }
    //~ }

    return result;

    KRATOS_CATCH("")
}


void TestTaylorHood::Initialize()
{
    KRATOS_TRY;

    const GeometryType& rGeom = this->GetGeometry();
    const SizeType Dim = rGeom.WorkingSpaceDimension();
    const SizeType NumVNodes = rGeom.PointsNumber();

    // Define a geometry container for water height nodes
    switch (NumVNodes)
    {
    case 3: // 2D P1P1, not div-stable !!
        mpHeightGeometry = this->pGetGeometry();
        break;
    case 4: // 2D Q1Q1, not div-stable !!
        mpHeightGeometry = this->pGetGeometry();
        break;
    case 6: // 2D P2P1
        mpHeightGeometry = GeometryType::Pointer( new Triangle2D3< Node<3> >(rGeom(0), rGeom(1), rGeom(2)) );
        break;
    case 9: // 2D Q2Q1
        mpHeightGeometry = GeometryType::Pointer( new Quadrilateral2D4< Node<3> >(rGeom(0), rGeom(1), rGeom(2), rGeom(3)) );
        break;
    case 10: // 3D P2P1
        mpHeightGeometry = GeometryType::Pointer( new Tetrahedra3D4< Node<3> >(rGeom(0), rGeom(1), rGeom(2), rGeom(3)) );
        break;
    case 27: // 3D Q2Q1
        mpHeightGeometry = GeometryType::Pointer( new Hexahedra3D8< Node<3> >(rGeom(0), rGeom(1), rGeom(2), rGeom(3), rGeom(4), rGeom(5), rGeom(6), rGeom(7)) );
        break;
    default:
        KRATOS_ERROR << "Unexpected geometry type for Primitive Variables Taylor-Hood elements" << std::endl;
    }
    const SizeType NumHNodes = mpHeightGeometry->PointsNumber();

    if (NumVNodes > 4)
        this->mIntegrationMethod = GeometryData::GI_GAUSS_4; // Quadratic velocities
    else
        this->mIntegrationMethod = GeometryData::GI_GAUSS_2; // Linear velocities

    const GeometryType::IntegrationPointsArrayType& IntegrationPoints = rGeom.IntegrationPoints( this->mIntegrationMethod );

    // Initialize member variables
    mDNv_DX.resize( IntegrationPoints.size() ); // Shape function derivatives container
    mDNh_DX.resize( 1 );                        // Shape function derivatives are constant for height
    mDetJ.resize( IntegrationPoints.size() );   // Determinant of Jacobian at each integration point

    // Geting the jacobian for the velocity geometry
    GeometryType::JacobiansType J;
    J = GetGeometry().Jacobian( J, mIntegrationMethod );

    const GeometryType::ShapeFunctionsGradientsType& DNv_De = rGeom.ShapeFunctionsLocalGradients( this->mIntegrationMethod );

    // Temporary container for inverse of J
    Matrix InvJ;

    // Calculating the inverse J
    for ( SizeType g = 0; g < IntegrationPoints.size(); g++ )
    {
        // Calculating and storing inverse of the jacobian and the parameters needed
        MathUtils<double>::InvertMatrix( J[g], InvJ, mDetJ[g] );

        // Calculating the shape function derivatives in global coordinates
        mDNv_DX[g].resize(NumVNodes,Dim);
        noalias( mDNv_DX[g] ) = prod( DNv_De[g], InvJ );
    }

    GeometryType::ShapeFunctionsGradientsType DNh_De;
    DNh_De = mpHeightGeometry->ShapeFunctionsIntegrationPointsGradients(DNh_De, GeometryData::GI_GAUSS_1 );
    mDNh_DX[0].resize(NumHNodes, Dim);
    noalias( mDNh_DX[0] ) = DNh_De[0];

    KRATOS_CATCH( "" )
}


void TestTaylorHood::CalculateLocalSystem(MatrixType &rLeftHandSideMatrix, VectorType &rRightHandSideVector, ProcessInfo &rCurrentProcessInfo)
{
    // Obtain required constants
    const SizeType Dim = this->GetGeometry().WorkingSpaceDimension();
    const SizeType NumVNodes = this->GetGeometry().PointsNumber();
    const SizeType NumHNodes = mpHeightGeometry->PointsNumber();
    const SizeType NumGauss = this->GetGeometry().IntegrationPoints(this->mIntegrationMethod).size();

    const SizeType LocalSize = Dim * NumVNodes + NumHNodes;

    const Matrix NvContainer = this->GetGeometry().ShapeFunctionsValues(this->mIntegrationMethod);
    const Matrix NhContainer = mpHeightGeometry->ShapeFunctionsValues(this->mIntegrationMethod);

    const GeometryType::IntegrationPointsArrayType& IntegrationPoints = this->GetGeometry().IntegrationPoints( this->mIntegrationMethod );

    // Getting gravity value and constants
    mGravity = rCurrentProcessInfo[GRAVITY_Z];
    const double delta_t = rCurrentProcessInfo[DELTA_TIME];
    const double dt_inv  = 1.0 / delta_t;

    // Initialize local contribution
    if (rLeftHandSideMatrix.size1() != LocalSize)
        rLeftHandSideMatrix.resize(LocalSize, LocalSize, false);

    rLeftHandSideMatrix = ZeroMatrix(LocalSize,LocalSize);

    if (rRightHandSideVector.size() != LocalSize)
        rRightHandSideVector.resize(LocalSize, false);

    rRightHandSideVector = ZeroVector(LocalSize);

    // Evaluate variables in the Height geometry: linear interpolation
    double Height = 0;
    double Depth = 0;
    for (SizeType i = 0; i < NumHNodes; i++) // so, nGauss = nNodes
    {
        Height += mpHeightGeometry->operator[](i).FastGetSolutionStepValue(HEIGHT);
        Depth  += mpHeightGeometry->operator[](i).FastGetSolutionStepValue(BATHYMETRY);
    }
    Height /= static_cast<double>(NumHNodes);
    Depth /= static_cast<double>(NumHNodes);

    double tau_v;
    double tau_h;
    double Ctau = rCurrentProcessInfo[DYNAMIC_TAU];
    double abs_height = std::abs(Height);
    double elem_size = mpHeightGeometry->Length();
    tau_v = Ctau * elem_size * std::sqrt(mGravity/abs_height);
    tau_h = Ctau * elem_size * std::sqrt(abs_height/mGravity);
    //~ ComputeStabilizationParameters(tau_v,tau_h,Height,mGravity,rCurrentProcessInfo);

    // Loop on integration points
    for (SizeType g = 0; g < NumGauss; g++)
    {
        const ShapeFunctionsType& Nv = row(NvContainer,g);
        const ShapeFunctionsType& Nh = row(NhContainer,g);
        const ShapeDerivativesType& DNv_DX = mDNv_DX[g];
        const ShapeDerivativesType& DNh_DX = mDNh_DX[0];
        const double GaussWeight = mDetJ[g] * IntegrationPoints[g].Weight();

        //~ double Height;
        array_1d<double,3> depth_grad(3,0.0);
        array_1d<double,3> Velocity(3,0.0);

        // Interpolation using height is linear
        //~ this->EvaluateInPoint(Height,HEIGHT,Nh,*mpHeightGeometry);
        this->EvaluateGradient(depth_grad,BATHYMETRY,DNh_DX,*mpHeightGeometry);
        this->EvaluateInPoint(Velocity,VELOCITY,Nv,this->GetGeometry());


        // Add inertia terms
        this->AddMassTerms(rLeftHandSideMatrix,rRightHandSideVector,dt_inv,Nv,Nh,GaussWeight);

        // Add mixed wave equation terms in mass and momentum equations
        this->AddWaveEquationTerms(rLeftHandSideMatrix,Height,Nv,Nh,DNv_DX,DNh_DX,GaussWeight);

        // Add convective terms
        this->AddConvectiveTerms(rLeftHandSideMatrix,Velocity,Nv,Nh,DNv_DX,DNh_DX,GaussWeight);

        this->AddStabTerms(rLeftHandSideMatrix,tau_v,tau_h,DNv_DX,DNh_DX,GaussWeight);

        // Add velocity-height terms
        this->AddSourceTerms(rRightHandSideVector,depth_grad,Nh,GaussWeight);
    }

    // Add residual of previous iteration to RHS
    VectorType LastValues = ZeroVector(LocalSize);
    this->GetValuesVector(LastValues);
    noalias(rRightHandSideVector) -= prod(rLeftHandSideMatrix,LastValues);
}


void TestTaylorHood::GetDofList(DofsVectorType &rElementalDofList,
                                 ProcessInfo &rCurrentProcessInfo)
{
    const SizeType Dim = this->GetGeometry().WorkingSpaceDimension();
    const SizeType NumVNodes = this->GetGeometry().PointsNumber();
    const SizeType NumHNodes = mpHeightGeometry->PointsNumber();

    const SizeType LocalSize = NumVNodes * Dim + NumHNodes;

    if (rElementalDofList.size() != LocalSize)
        rElementalDofList.resize(LocalSize);

    SizeType Index = 0;

    for (SizeType i = 0; i < NumVNodes; i++)
    {
        rElementalDofList[Index++] = GetGeometry()[i].pGetDof(VELOCITY_X);
        rElementalDofList[Index++] = GetGeometry()[i].pGetDof(VELOCITY_Y);
        if(Dim > 2) rElementalDofList[Index++] = GetGeometry()[i].pGetDof(VELOCITY_Z);
    }

    for (SizeType i = 0; i < NumHNodes; i++)
        rElementalDofList[Index++] = mpHeightGeometry->operator[](i).pGetDof(HEIGHT);
}


void TestTaylorHood::EquationIdVector(Element::EquationIdVectorType &rResult, ProcessInfo &rCurrentProcessInfo)
{
    const SizeType Dim = this->GetGeometry().WorkingSpaceDimension();
    const SizeType NumVNodes = this->GetGeometry().PointsNumber();
    const SizeType NumHNodes = mpHeightGeometry->PointsNumber();

    const SizeType LocalSize = NumVNodes * Dim + NumHNodes;

    if (rResult.size() != LocalSize)
        rResult.resize(LocalSize);

    SizeType Index = 0;

    for (SizeType i = 0; i < NumVNodes; i++)
    {
        rResult[Index++] = GetGeometry()[i].GetDof(VELOCITY_X).EquationId();
        rResult[Index++] = GetGeometry()[i].GetDof(VELOCITY_Y).EquationId();
        if(Dim > 2) rResult[Index++] = GetGeometry()[i].GetDof(VELOCITY_Z).EquationId();
    }

    for (SizeType i = 0; i < NumHNodes; i++)
        rResult[Index++] = mpHeightGeometry->operator[](i).GetDof(HEIGHT).EquationId();
}


void TestTaylorHood::GetValuesVector(Vector &rValues, int Step)
{
    const SizeType Dim = this->GetGeometry().WorkingSpaceDimension();
    const SizeType NumVNodes = this->GetGeometry().PointsNumber();
    const SizeType NumHNodes = mpHeightGeometry->PointsNumber();

    const SizeType LocalSize = NumVNodes * Dim + NumHNodes;

    if (rValues.size() != LocalSize)
        rValues.resize(LocalSize);

    SizeType Index = 0;

    for (SizeType i = 0; i < NumVNodes; i++)
    {
        rValues[Index++] = GetGeometry()[i].FastGetSolutionStepValue(VELOCITY_X,Step);
        rValues[Index++] = GetGeometry()[i].FastGetSolutionStepValue(VELOCITY_Y,Step);
        if(Dim > 2) rValues[Index++] = GetGeometry()[i].FastGetSolutionStepValue(VELOCITY_Z,Step);
    }

    for (SizeType i = 0; i < NumHNodes; i++)
        rValues[Index++] = mpHeightGeometry->operator[](i).FastGetSolutionStepValue(HEIGHT,Step);
}


void TestTaylorHood::FinalizeSolutionStep(ProcessInfo &rCurrentProcessInfo)
{
    KRATOS_TRY;

    const SizeType NumVNodes = this->GetGeometry().PointsNumber();
    GeometryType& rGeom = this->GetGeometry();
    switch (NumVNodes)
    {
    case 3: // P1P1, both geometries have the same nodes, do nothing
    {
        break;
    }
    case 4: // Q1Q1, both geometries have the same nodes, do nothing
    {
        break;
    }
    case 6: // triangle
    {
        const double p0 = rGeom[0].FastGetSolutionStepValue(HEIGHT);
        const double p1 = rGeom[1].FastGetSolutionStepValue(HEIGHT);
        const double p2 = rGeom[2].FastGetSolutionStepValue(HEIGHT);
        ThreadSafeNodeWrite(rGeom[3],HEIGHT, 0.5 * (p0 + p1) );
        ThreadSafeNodeWrite(rGeom[4],HEIGHT, 0.5 * (p1 + p2) );
        ThreadSafeNodeWrite(rGeom[5],HEIGHT, 0.5 * (p2 + p0) );
        break;
    }
    case 9: // quadrilateral
    {
        const double p0 = rGeom[0].FastGetSolutionStepValue(HEIGHT);
        const double p1 = rGeom[1].FastGetSolutionStepValue(HEIGHT);
        const double p2 = rGeom[2].FastGetSolutionStepValue(HEIGHT);
        const double p3 = rGeom[3].FastGetSolutionStepValue(HEIGHT);
        ThreadSafeNodeWrite(rGeom[4],HEIGHT, 0.5 * (p0 + p1) );
        ThreadSafeNodeWrite(rGeom[5],HEIGHT, 0.5 * (p1 + p2) );
        ThreadSafeNodeWrite(rGeom[6],HEIGHT, 0.5 * (p2 + p3) );
        ThreadSafeNodeWrite(rGeom[7],HEIGHT, 0.5 * (p3 + p0) );
        ThreadSafeNodeWrite(rGeom[8],HEIGHT, 0.25 * (p0 + p1 + p2 + p3) );
        break;
    }
    case 10: // tetrahedron
    {
        const double p0 = rGeom[0].FastGetSolutionStepValue(HEIGHT);
        const double p1 = rGeom[1].FastGetSolutionStepValue(HEIGHT);
        const double p2 = rGeom[2].FastGetSolutionStepValue(HEIGHT);
        const double p3 = rGeom[3].FastGetSolutionStepValue(HEIGHT);
        ThreadSafeNodeWrite(rGeom[4],HEIGHT, 0.5 * (p0 + p1) );
        ThreadSafeNodeWrite(rGeom[5],HEIGHT, 0.5 * (p1 + p2) );
        ThreadSafeNodeWrite(rGeom[6],HEIGHT, 0.5 * (p2 + p0) );
        ThreadSafeNodeWrite(rGeom[7],HEIGHT, 0.5 * (p0 + p3) );
        ThreadSafeNodeWrite(rGeom[8],HEIGHT, 0.5 * (p1 + p3) );
        ThreadSafeNodeWrite(rGeom[9],HEIGHT, 0.5 * (p2 + p3) );
        break;
    }
    case 27: // hexahedron
    {
        const double p0 = rGeom[0].FastGetSolutionStepValue(HEIGHT);
        const double p1 = rGeom[1].FastGetSolutionStepValue(HEIGHT);
        const double p2 = rGeom[2].FastGetSolutionStepValue(HEIGHT);
        const double p3 = rGeom[3].FastGetSolutionStepValue(HEIGHT);
        const double p4 = rGeom[4].FastGetSolutionStepValue(HEIGHT);
        const double p5 = rGeom[5].FastGetSolutionStepValue(HEIGHT);
        const double p6 = rGeom[6].FastGetSolutionStepValue(HEIGHT);
        const double p7 = rGeom[7].FastGetSolutionStepValue(HEIGHT);
        // edges -- bottom
        ThreadSafeNodeWrite(rGeom[8],HEIGHT, 0.5 * (p0 + p1) );
        ThreadSafeNodeWrite(rGeom[9],HEIGHT, 0.5 * (p1 + p2) );
        ThreadSafeNodeWrite(rGeom[10],HEIGHT, 0.5 * (p2 + p3) );
        ThreadSafeNodeWrite(rGeom[11],HEIGHT, 0.5 * (p3 + p0) );
        // edges -- middle
        ThreadSafeNodeWrite(rGeom[12],HEIGHT, 0.5 * (p4 + p0) );
        ThreadSafeNodeWrite(rGeom[13],HEIGHT, 0.5 * (p5 + p1) );
        ThreadSafeNodeWrite(rGeom[14],HEIGHT, 0.5 * (p6 + p2) );
        ThreadSafeNodeWrite(rGeom[15],HEIGHT, 0.5 * (p7 + p3) );
        // edges -- top
        ThreadSafeNodeWrite(rGeom[16],HEIGHT, 0.5 * (p4 + p5) );
        ThreadSafeNodeWrite(rGeom[17],HEIGHT, 0.5 * (p5 + p6) );
        ThreadSafeNodeWrite(rGeom[18],HEIGHT, 0.5 * (p6 + p7) );
        ThreadSafeNodeWrite(rGeom[19],HEIGHT, 0.5 * (p7 + p0) );
        // face centers
        ThreadSafeNodeWrite(rGeom[20],HEIGHT, 0.25 * (p0 + p1 + p2 + p3) );
        ThreadSafeNodeWrite(rGeom[21],HEIGHT, 0.25 * (p0 + p1 + p4 + p5) );
        ThreadSafeNodeWrite(rGeom[22],HEIGHT, 0.25 * (p1 + p2 + p5 + p6) );
        ThreadSafeNodeWrite(rGeom[23],HEIGHT, 0.25 * (p2 + p3 + p6 + p7) );
        ThreadSafeNodeWrite(rGeom[24],HEIGHT, 0.25 * (p3 + p0 + p7 + p4) );
        ThreadSafeNodeWrite(rGeom[25],HEIGHT, 0.25 * (p4 + p5 + p6 + p7) );
        // element center
        ThreadSafeNodeWrite(rGeom[26],HEIGHT, 0.125 * (p0+p1+p2+p3+p4+p5+p6+p7) );
        break;
    }
    default:
        KRATOS_ERROR << "Unexpected geometry type for Primitive Variables Taylor-Hood elements" << std::endl;
    }

    KRATOS_CATCH("");
}


//~ void TestTaylorHood::ComputeStabilizationParameters(double& rTauv, double& rTauh, const double rHeight, const double rGravity, ProcessInfo& rCurrentProcessInfo)
//~ {
    //~ // Initialize outputs
    //~ rTauv = 0;
    //~ rTauh = 0;
    //~ double Ctau = rCurrentProcessInfo[DYNAMIC_TAU];
    //~ double abs_height = std::abs(rHeight);
    //~ double elem_size = mpHeightGeometry->Length();
    //~ rTauv = Ctau * elem_size * std::sqrt(rGravity/abs_height);
    //~ rTauh = Ctau * elem_size * std::sqrt(abs_height/rGravity);
//~ }


void TestTaylorHood::AddMassTerms(MatrixType& rLHS,
                                  VectorType& rRHS,
                                  const double& rDeltaTInv,
                                  const ShapeFunctionsType& Nv,
                                  const ShapeFunctionsType& Nh,
                                  const double& rWeight)
{
    const SizeType Dim = this->GetGeometry().WorkingSpaceDimension();
    const SizeType NumVNodes = this->GetGeometry().PointsNumber();
    const SizeType NumHNodes = mpHeightGeometry->PointsNumber();
    const int local_size = Dim * NumVNodes + NumHNodes;

    SizeType FirstRow = 0;
    SizeType FirstCol = 0;
    double term_ij = 0.0;
    MatrixType mass_matrix = ZeroMatrix(local_size, local_size);

    // Add mass terms for velocity unknown
    for (unsigned int i = 0; i < NumVNodes; ++i)
    {
        for (unsigned int j = 0; j < NumVNodes; ++j)
        {
            term_ij = rWeight * Nv[i] * Nv[j];

            for (unsigned int d = 0; d < Dim; ++d)
                mass_matrix(FirstRow+d,FirstCol+d) += term_ij;
            FirstCol += Dim;
        }
        FirstCol = 0;
        FirstRow += Dim;
    }

    // Add mass terms for height unknown
    FirstRow = NumVNodes * Dim;
    FirstCol = NumVNodes * Dim;
    for (unsigned int i = 0; i < NumHNodes; i++)
    {
        for (unsigned int j = 0; j < NumHNodes; j++)
        {
            term_ij = rWeight * Nh[i] * Nh[j];
            mass_matrix(FirstRow+i,FirstCol+j) += term_ij;
        }
    }

    // Add mass contribution to LHS
    rLHS += rDeltaTInv * mass_matrix;

    // Add mass contribution to RHS
    VectorType old_values = ZeroVector(local_size);
    this->GetValuesVector(old_values,1);
    rRHS += rDeltaTInv * prod (mass_matrix, old_values);
}

void TestTaylorHood::AddWaveEquationTerms(MatrixType &rLHS,
                                          const double& rHeight,
                                          const ShapeFunctionsType &Nv,
                                          const ShapeFunctionsType &Nh,
                                          const ShapeDerivativesType &DNv_DX,
                                          const ShapeDerivativesType &DNh_DX,
                                          const double& rWeight)
{
    const SizeType Dim = this->GetGeometry().WorkingSpaceDimension();
    const SizeType NumVNodes = this->GetGeometry().PointsNumber();
    const SizeType NumHNodes = mpHeightGeometry->PointsNumber();

    SizeType FirstRow = 0;
    SizeType Col = NumVNodes * Dim;
    //~ double Term_ij = 0.0;

    for(SizeType i = 0; i < NumVNodes; ++i)
    {
        for(SizeType j = 0; j < NumHNodes; ++j)
        {
            for(SizeType d = 0; d < Dim; d++)
            {
                rLHS(FirstRow+d,Col) += rWeight * mGravity * Nv[i] * DNh_DX(j,d); // height gradient
                rLHS(Col,FirstRow+d) += rWeight * rHeight  * Nh[j] * DNv_DX(i,d); // velocity divergence
            }

            // Update column index
            Col += 1;
        }
        // Update matrix indices
        FirstRow += Dim;
        Col = NumVNodes * Dim;
    }
}

void TestTaylorHood::AddConvectiveTerms(MatrixType &rLHS,
                                        const array_1d<double,3>& rConvVel,
                                        const ShapeFunctionsType& rNv,
                                        const ShapeFunctionsType& rNh,
                                        const ShapeDerivativesType& rDNv_DX,
                                        const ShapeDerivativesType& rDNh_DX,
                                        const double& rWeight)
{
    const SizeType Dim = this->GetGeometry().WorkingSpaceDimension();
    const SizeType NumVNodes = this->GetGeometry().PointsNumber();
    const SizeType NumHNodes = mpHeightGeometry->PointsNumber();

    SizeType FirstRow = 0;
    SizeType FirstCol = 0;

    double term_ij;

    // Convecting velocity gradient
    for (unsigned int i = 0; i < NumVNodes; ++i)
    {
        for (unsigned int j = 0; j < NumVNodes; ++j)
        {
            term_ij = rWeight * rNv[i] * ( rConvVel[0] * rDNv_DX(j,0) + rConvVel[1] * rDNv_DX(j,1) );
            
            for (unsigned int d = 0; d < Dim; d++)
                rLHS(FirstRow+d,FirstCol+d) += term_ij;
            
            FirstCol += Dim;
        }
        FirstCol = 0;
        FirstRow += Dim;
    }

    FirstRow = NumVNodes * Dim;
    FirstCol = NumVNodes * Dim;
    // Convecting height gradient
    for (unsigned int i = 0; i < NumHNodes; i++)
    {
        for (unsigned int j = 0; j < NumHNodes; j++)
        {
            term_ij = rWeight * rNh[i] * ( rConvVel[0] * rDNh_DX(j,0) + rConvVel[1] * rDNh_DX(j,1) );
            
            rLHS(FirstRow,FirstCol) += term_ij;
            
            FirstCol += 1;
        }
        FirstCol = 0;
        FirstRow += 1;
    }
}

void TestTaylorHood::AddStabTerms(MatrixType &rLHS,
                                  const double& rTau_v,
                                  const double& rTau_h,
                                  const ShapeDerivativesType& rDNv_DX,
                                  const ShapeDerivativesType& rDNh_DX,
                                  const double& rWeight)
{
    const SizeType Dim = this->GetGeometry().WorkingSpaceDimension();
    const SizeType NumVNodes = this->GetGeometry().PointsNumber();
    const SizeType NumHNodes = mpHeightGeometry->PointsNumber();

    SizeType FirstRow = 0;
    SizeType FirstCol = 0;

    // Add diffusion terms for velocity unknown
    for (unsigned int i = 0; i < NumVNodes; ++i)
    {
        for (unsigned int j = 0; j < NumVNodes; ++j)
        {
            rLHS(FirstRow  ,FirstCol  ) += rWeight * rTau_v * rDNv_DX(i,0) * rDNv_DX(j,0);
            rLHS(FirstRow  ,FirstCol+1) += rWeight * rTau_v * rDNv_DX(i,0) * rDNv_DX(j,1);
            rLHS(FirstRow+1,FirstCol  ) += rWeight * rTau_v * rDNv_DX(i,1) * rDNv_DX(j,0);
            rLHS(FirstRow+1,FirstCol+1) += rWeight * rTau_v * rDNv_DX(i,1) * rDNv_DX(j,1);
            FirstCol += Dim;
        }
        FirstCol = 0;
        FirstRow += Dim;
    }

    // Add diffusion terms for height unknown
    FirstRow = NumVNodes * Dim;
    FirstCol = NumVNodes * Dim;
    for (unsigned int i = 0; i < NumHNodes; ++i)
    {
        for (unsigned int j = 0; j < NumHNodes; ++j)
        {
            rLHS(FirstRow+i,FirstCol+j) += rWeight * rTau_h * ( rDNh_DX(i,0) * rDNh_DX(j,0) + rDNh_DX(i,1) * rDNh_DX(j,1) );
        }
    }
}

void TestTaylorHood::AddSourceTerms(VectorType &rRHS,
                                         const array_1d<double,2>& rDepthGrad,
                                         const ShapeFunctionsType &Nv,
                                         const double& rWeight)
{
    const SizeType Dim = this->GetGeometry().WorkingSpaceDimension();
    const SizeType NumVNodes = this->GetGeometry().PointsNumber();
    //~ const SizeType NumHNodes = mpHeightGeometry->PointsNumber();

    SizeType FirstRow = 0;

    for (SizeType i = 0; i < NumVNodes; ++i)
    {
        for (SizeType d = 0; d < Dim; ++d)
        {
            rRHS(FirstRow + d) += rWeight * Nv[i] * rDepthGrad[d];
        }

        FirstRow += 1;
    }
}

} // namespace Kratos