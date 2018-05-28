// KRATOS  ___|  |                   |                   |
//       \___ \  __|  __| |   |  __| __| |   |  __| _` | |
//             | |   |    |   | (    |   |   | |   (   | |
//       _____/ \__|_|   \__,_|\___|\__|\__,_|_|  \__,_|_| MECHANICS
//
//  License:         BSD License
//                   license: structural_mechanics_application/license.txt
//
//  Main authors:    Alejandro Cornejo & Lucia Barbu
//

#if !defined (KRATOS_GENERIC_SMALL_STRAIN_ISOTROPIC_DAMAGE_3D_H_INCLUDED)
#define  KRATOS_GENERIC_SMALL_STRAIN_ISOTROPIC_DAMAGE_3D_H_INCLUDED

// System includes
#include <string>
#include <iostream>

// Project includes
#include "includes/define.h"
#include "includes/serializer.h"
#include "includes/properties.h"
#include "utilities/math_utils.h"
#include "custom_processes/tangent_operator_calculator_process.h"

#include "includes/constitutive_law.h"
#include "structural_mechanics_application_variables.h"

namespace Kratos
{
///@name Kratos Globals
///@{

///@}
///@name Type Definitions
///@{

///@}
///@name  Enum's
///@{

///@}
///@name  Functions
///@{

///@}
///@name Kratos Classes
///@{
/**
 * @class GenericConstitutiveLawIntegrator
 * @ingroup StructuralMechanicsApplication
 * @brief
 * @details
 * @author Alejandro Cornejo & Lucia Barbu
 */
template <class ConstLawIntegratorType>
class KRATOS_API(STRUCTURAL_MECHANICS_APPLICATION) GenericSmallStrainIsotropicDamage3D
    : public ConstitutiveLaw
{
public:
    ///@name Type Definitions
    ///@{

    /// Counted pointer of GenericYieldSurface
    KRATOS_CLASS_POINTER_DEFINITION(GenericSmallStrainIsotropicDamage3D);

    ///@}
    ///@name Life Cycle
    ///@{

    /**
    * Default constructor.
    */
    GenericSmallStrainIsotropicDamage3D()
    {
    }

    /**
    * Clone.
    */
    ConstitutiveLaw::Pointer Clone() const override
    {
        GenericSmallStrainIsotropicDamage3D<ConstLawIntegratorType>::Pointer p_clone
            (new GenericSmallStrainIsotropicDamage3D<ConstLawIntegratorType>(*this));
        return p_clone;
    }

    /**
    * Copy constructor.
    */
    GenericSmallStrainIsotropicDamage3D (const GenericSmallStrainIsotropicDamage3D& rOther)
    : ConstitutiveLaw(rOther)
    {
    }
    /**
    * Destructor.
    */
    ~GenericSmallStrainIsotropicDamage3D() override
    {
    }

    ///@}
    ///@name Operators
    ///@{

    ///@}
    ///@name Operations
    ///@{

    int GetVoigtSize(){return 6;}
    int GetWorkingSpaceDimension() {return 3;}

    double GetThreshold() {return mThreshold;}
    double GetDamage() {return mDamage;}
    double GetNonConvThreshold() {return mNonConvThreshold;}
    double GetNonConvDamage() {return mNonConvDamage;}

    void SetThreshold(const double& toThreshold) {mThreshold = toThreshold;}
    void SetDamage(const double& toDamage) {mDamage = toDamage;}
    void SetNonConvThreshold(const double& toThreshold) {mNonConvThreshold = toThreshold;}
    void SetNonConvDamage(const double& toDamage) {mNonConvDamage = toDamage;}


    void CalculateMaterialResponsePK1(ConstitutiveLaw::Parameters& rValues)
    {
        this->CalculateMaterialResponseCauchy(rValues);
    }
    void CalculateMaterialResponsePK2(ConstitutiveLaw::Parameters& rValues)
    {
        this->CalculateMaterialResponseCauchy(rValues);
    }
    void CalculateMaterialResponseKirchhoff(ConstitutiveLaw::Parameters& rValues)
    {
        this->CalculateMaterialResponseCauchy(rValues);
    }

    void CalculateMaterialResponseCauchy(ConstitutiveLaw::Parameters& rValues)
    {
        // Integrate Stress Damage
        const Properties& rMaterialProperties = rValues.GetMaterialProperties();
        const int VoigtSize = this->GetVoigtSize();
        Vector& IntegratedStressVector = rValues.GetStressVector();
        Matrix& TangentTensor = rValues.GetConstitutiveMatrix(); // todo modify after integration

        // Elastic Matrix
        Matrix C;
        this->CalculateElasticMatrix(C, rMaterialProperties);

        double Threshold, Damage;
        // In the 1st step must be set
        if (this->GetThreshold() == 0.0) 
        {
            ConstLawIntegratorType::YieldSurfaceType::GetInitialUniaxialThreshold(rMaterialProperties, Threshold);
            this->SetThreshold(Threshold);
        }

        // Converged values
        Threshold = this->GetThreshold();
        Damage    = this->GetDamage();

        // S0 = C:(E-Ep)
        Vector PredictiveStressVector = prod(C, rValues.GetStrainVector());

        // Initialize Plastic Parameters
        double UniaxialStress;
        ConstLawIntegratorType::YieldSurfaceType::CalculateEquivalentStress(PredictiveStressVector, 
            rValues.GetStrainVector(), UniaxialStress, rMaterialProperties);

        const double F = UniaxialStress - Threshold; 

        if (F <= 0.0) 
        {   // Elastic case
            noalias(IntegratedStressVector) = PredictiveStressVector;
            this->SetNonConvDamage(Damage);
            this->SetNonConvThreshold(Threshold);
            
            noalias(TangentTensor) = (1 - Damage)*C;
        }
        else // Damage case
        {
            const double CharacteristicLength = rValues.GetGeometry().Length();

            // This routine updates the PredictiveStress to verify the yield surf
            ConstLawIntegratorType::IntegrateStressVector(PredictiveStressVector, UniaxialStress,
                Damage, Threshold, rMaterialProperties, CharacteristicLength);

            // Updated Values
            noalias(IntegratedStressVector) = PredictiveStressVector; 
            this->SetNonConvDamage(Damage);
            this->SetNonConvThreshold(Threshold);

            //noalias(TangentTensor) = (1 - Damage)*C; // Secant Tensor
            this->CalculateTangentTensor(rValues); 
            TangentTensor = rValues.GetConstitutiveMatrix();
        }
        
    } // End CalculateMaterialResponseCauchy

    void CalculateTangentTensor(ConstitutiveLaw::Parameters& rValues) 
    {
        TangentOperatorCalculatorProcess <GenericSmallStrainIsotropicDamage3D> (rValues).Execute();
    }

    void FinalizeSolutionStep(
        const Properties& rMaterialProperties,
        const GeometryType& rElementGeometry,
        const Vector& rShapeFunctionsValues,
        const ProcessInfo& rCurrentProcessInfo
    ) override
    {
        this->SetDamage(this->GetNonConvDamage());
        this->SetThreshold(this->GetNonConvThreshold());
    }

    void CalculateElasticMatrix(Matrix &rElasticityTensor,
        const Properties &rMaterialProperties
    )
    {
        const double E = rMaterialProperties[YOUNG_MODULUS];
        const double poisson_ratio = rMaterialProperties[POISSON_RATIO];
        const double lambda =
            E * poisson_ratio / ((1. + poisson_ratio) * (1.0 - 2.0 * poisson_ratio));
        const double mu = E / (2.0 + 2.0 * poisson_ratio);

        if (rElasticityTensor.size1() != 6 || rElasticityTensor.size2() != 6)
            rElasticityTensor.resize(6, 6, false);
        rElasticityTensor.clear();

        rElasticityTensor(0, 0) = lambda + 2.0 * mu;
        rElasticityTensor(0, 1) = lambda;
        rElasticityTensor(0, 2) = lambda;
        rElasticityTensor(1, 0) = lambda;
        rElasticityTensor(1, 1) = lambda + 2.0 * mu;
        rElasticityTensor(1, 2) = lambda;
        rElasticityTensor(2, 0) = lambda;
        rElasticityTensor(2, 1) = lambda;
        rElasticityTensor(2, 2) = lambda + 2.0 * mu;
        rElasticityTensor(3, 3) = mu;
        rElasticityTensor(4, 4) = mu;
        rElasticityTensor(5, 5) = mu;
    }

    ///@}
    ///@name Access
    ///@{

    ///@}
    ///@name Inquiry
    ///@{

    ///@}
    ///@name Input and output
    ///@{

    ///@}
    ///@name Friends
    ///@{

    ///@}

protected:
    ///@name Protected static Member Variables
    ///@{

    ///@}
    ///@name Protected member Variables
    ///@{

    ///@}
    ///@name Protected Operators
    ///@{

    ///@}
    ///@name Protected Operations
    ///@{

    ///@}
    ///@name Protected  Access
    ///@{

    ///@}
    ///@name Protected Inquiry
    ///@{

    ///@}
    ///@name Protected LifeCycle
    ///@{

    ///@}
private:
    ///@name Static Member Variables
    ///@{

    ///@}
    ///@name Member Variables
    ///@{

    // Converged values
    double mDamage = 0.0;
    double mThreshold = 0.0;

    // Non Converged values
    double mNonConvDamage = 0.0;
    double mNonConvThreshold = 0.0;

    ///@}
    ///@name Private Operators
    ///@{

    ///@}
    ///@name Private Operations
    ///@{

    ///@}
    ///@name Private  Access
    ///@{

    ///@}
    ///@name Private Inquiry
    ///@{

    ///@}
    ///@name Un accessible methods
    ///@{

    // Serialization

    friend class Serializer;

    void save(Serializer& rSerializer) const override
    {
        KRATOS_SERIALIZE_SAVE_BASE_CLASS( rSerializer, ConstitutiveLaw )
        rSerializer.save("Damage", mDamage);
        rSerializer.save("Threshold", mThreshold);
        rSerializer.save("NonConvDamage", mNonConvDamage);
        rSerializer.save("mNonConvThreshold", mNonConvThreshold);

    }

    void load(Serializer& rSerializer) override
    {
        KRATOS_SERIALIZE_LOAD_BASE_CLASS( rSerializer, ConstitutiveLaw)
        rSerializer.load("Damage", mDamage);
        rSerializer.load("Threshold", mThreshold);
        rSerializer.load("NonConvDamage", mDamage);
        rSerializer.load("mNonConvThreshold", mNonConvThreshold);
    }

    ///@}

}; // Class GenericYieldSurface

} // namespace kratos
#endif