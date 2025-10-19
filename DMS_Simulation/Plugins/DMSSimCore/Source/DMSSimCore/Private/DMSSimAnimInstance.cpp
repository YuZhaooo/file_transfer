
#include "DMSSimAnimInstance.h"



AActor *UDMSSimAnimInstance::GetRightHandAttachedObject() const
{
    if ( RightHandObject_.Num() == 0 )
        return nullptr;

    return RightHandObject_.CreateConstIterator().Value().Object_;
}

AActor* UDMSSimAnimInstance::GetLeftHandAttachedObject() const
{
    if ( LeftHandObject_.Num() == 0 )
        return nullptr;
    
    return LeftHandObject_.CreateConstIterator().Value().Object_;
}

FORCEINLINE void BlendIKParameter( float DeltaTime, float & InOut, UDMSSimAnimInstance::EDMSIKBlendOp & BlendOp, float Time )
{
    if ( Time < SMALL_NUMBER )
    {
          InOut   = BlendOp == UDMSSimAnimInstance::IKBlend_In ? 1.0f : 0.0f;
          BlendOp = UDMSSimAnimInstance::IKBlend_None;

          return;
    }

    const float Advance = DeltaTime * (1.0f / Time);

    if ( BlendOp == UDMSSimAnimInstance::IKBlend_In )
    {
        InOut += Advance;
        if ( InOut > 1.0f ) 
        {
            InOut   = 1.0f;
            BlendOp = UDMSSimAnimInstance::IKBlend_None;        
        }
    }
    else
    {
        InOut -= Advance;
        if ( InOut < 0.0f )
        {
            InOut   = 0.0f;
            BlendOp = UDMSSimAnimInstance::IKBlend_None;
        }    
    }
}

void UDMSSimAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
    Super::NativeUpdateAnimation(DeltaSeconds);

    if ( SteeringIKLeftHandBlendOp != IKBlend_None )    
        BlendIKParameter( DeltaSeconds, SteeringWeightLeftHand, SteeringIKLeftHandBlendOp, SteeringIKLeftHandBlendOp == IKBlend_In ? SteeringIKLeftHandBlendIn : SteeringIKLeftHandBlendOut );
    
    if ( SteeringIKRightHandBlendOp != IKBlend_None )
        BlendIKParameter( DeltaSeconds, SteeringWeightRightHand, SteeringIKRightHandBlendOp, SteeringIKRightHandBlendOp == IKBlend_In ? SteeringIKRightHandBlendIn : SteeringIKRightHandBlendOut );
}
